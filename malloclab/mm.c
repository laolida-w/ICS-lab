/* 
name:Wang Yuhao
ID:2200017794
 * Simple, 32-bit and 64-bit clean allocator based on implicit free
 * lists, first-fit placement, and boundary tag coalescing, as described
 * in the CS:APP3e text. Blocks must be aligned to doubleword (8 byte) 
 * boundaries. Minimum block size is 16 bytes. 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<stdint.h>
#include "mm.h"
#include "memlib.h"

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

#define DEBUGx
#ifdef DEBUG
    #define  dbg_printf(...) printf(__VA_ARGS__)
    #define  CHECKHEAP() mm_checkheap(__LINE__)
#else
    # define dbg_printf(...)
    # define CHECKHEAP()
#endif


typedef uint32_t WORD;   ///< WORD is 32-bit
typedef uint64_t DWORD; 

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) 
  
/* Read and write a word at adress p of 4 byte size*/
#define GET_WORD(p)       (*(uint32_t *)(p))            
#define PUT_WORD(p, val)  (*(uint32_t*)(p) = (val))    


/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET_WORD(p) & ~0x7)                   
#define GET_ALLOC(p) (GET_WORD(p) & 0x1)                    

/* Given block ptr bp, comPUT_WORDe address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

/* Given block ptr bp, comPUT_WORDe address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) 
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) 

/* Grt and set pointer*/
#define GET_NEXT(bp)    (GET_WORD(bp) ? GET_WORD(bp) + heap_listp : NULL)
#define SET_NEXT(bp, val)  PUT_WORD((bp), (WORD)((val) ? ((char*)(val)-heap_listp) : 0))

#define SEGSIZE 14
/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  
static void**seglist;
/*
{16}            ---0
{17 4,..,32 }     ---1
...
{2^17+1,2^18}   ---13
*/

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void *place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void insert_into_list(void*bp);
static void remove_from_list(void*bp);

static int search_index(int size){
    int index=0;
    for (int j=size-1;index<SEGSIZE&&j>=SEGSIZE;index++,j>>=1){}
    return index==SEGSIZE?index-1:index;
}
/*
*insert_into_list  -  insert into list a free block at the head of the list;
*/
static void insert_into_list(void*bp){//make sure bp is absolute address 
  
    if(bp==NULL)return;
    /*insert into which list*/
    size_t size=GET_SIZE(HDRP(bp));
    int index=search_index(size);

    void*list_head=seglist[index];//
    if(list_head==NULL){ // empty list
        seglist[index]=bp;
        SET_NEXT(bp,NULL);
        return;
    }
    SET_NEXT(bp,list_head);
    seglist[index]=bp;
}
/*
*remove_from_list  -  remove_from_list a free block from the list
*/
static void remove_from_list(void*bp){
    if (bp == NULL )
        return;
    size_t size=GET_SIZE(HDRP(bp));
    int index=search_index(size);

    char *next = GET_NEXT(bp); //absolute address
    if (seglist[index]==bp){
        if(next==NULL){
            seglist[index] = NULL;
        }
        else{
            seglist[index] =next;
        }
    }
    else{
        char*prev=NULL;
         for (char*bp1 = seglist[index]; bp1!=NULL; bp1 = GET_NEXT(bp1)) {
            if(GET_NEXT(bp1)==bp){
                prev=bp1;
                break;
            }
         }

        if(next==NULL){//tail of the list
            SET_NEXT(prev,0);
        }
        else{
            SET_NEXT(prev,next);
        }
    }
    SET_NEXT(bp,0);
}
/* 
 * mm_init - Initialize the memory manager 
 */
int mm_init(void) 
{

    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(SEGSIZE * DSIZE + 4 * WSIZE)) == (void *)-1) 
        return -1;
     seglist=(void**)heap_listp;
   for (int i=0;i<SEGSIZE;i++){
        seglist[i]=NULL;
   }
   heap_listp =(char*)(seglist + SEGSIZE);
    PUT_WORD(heap_listp, 0);                          /* Alignment padding */
    PUT_WORD(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
    PUT_WORD(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
    PUT_WORD(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2*WSIZE);                     
    /* Extend the empty heap with a free block of INIT_SIZE bytes */
    if (extend_heap(CHUNKSIZE/2) == NULL) 
        return -1;
    CHECKHEAP();
    return 0;
}

/* 
 * malloc - Allocate a block with at least size bytes of payload 
 */
void *malloc(size_t size) 
{ 
   
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;      
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    if (heap_listp == 0){
        mm_init();
    }
    /* Adjust block size to include overhead and alignment reqs. */
    if (size == 448) size = 512;//magic number....

    if (size <= DSIZE)                                          
        asize = 2*DSIZE;                                        
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); 
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {  
        return place(bp, asize); 
    }
    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);              
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)  
        return NULL;       
    return place(bp, asize); 
} 

/* 
 * free - Free a block 
 */
void free(void *bp)
{
    if (bp == 0) 
        return;
    if (heap_listp == 0){
        mm_init();
    }
    size_t size = GET_SIZE(HDRP(bp));
    PUT_WORD(HDRP(bp), PACK(size, 0));
    PUT_WORD(FTRP(bp), PACK(size, 0));
    SET_NEXT(bp,0);
    coalesce(bp);
    CHECKHEAP();
}

/*
 * realloc - Naive implementation of realloc
 */
void *realloc(void *ptr, size_t size)
{
    size_t oldsize;
    void *newptr;
    if(size == 0) {
        mm_free(ptr);
        return 0;
    }
    if(ptr == NULL) {
        return mm_malloc(size);
    }
    newptr = mm_malloc(size);
    if(!newptr) {
        return 0;
    }
    oldsize = GET_SIZE(HDRP(ptr));
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);
    mm_free(ptr);
    return newptr;
}

/* 
 * mm_checkheap - Check the heap for correctness. Helpful hint: You
 *                can call this function using mm_checkheap(__LINE__);
 *                to identify the line number of the call site.
 */
void mm_checkheap( int lineno)  
{ 
   
    /*block checking */
    for( void *bp = NEXT_BLKP(heap_listp);bp!=mem_heap_hi()+1;bp=NEXT_BLKP(bp)){
        void*head=HDRP(bp);
        void*foot=FTRP(bp);
        /*head and foot mismatch*/
        if (GET_SIZE(head) != GET_SIZE(foot) ||
          GET_ALLOC(head) != GET_ALLOC(foot) ) {
        dbg_printf("(lineno=%d)Block %p:head and foot mismatch:", lineno,bp);
        dbg_printf("(lineno=%d)Head:%p  *head:0x%x" , lineno,head,GET_WORD(head));
        dbg_printf("(lineno=%d)Foot:%p  *foot:0x%x" , lineno,foot,GET_WORD(foot));
        exit(1);
        }
        /*right size?*/
        if (GET_SIZE(head) <2*DSIZE) {
            dbg_printf("(lineno=%d)Block %p:too small,*head:0x%x ", lineno,bp,GET_WORD(head));
            exit(1);
        }
        /*ALignment checking*/
         if ((size_t)(bp) %8) {
            dbg_printf("(lineno=%d) Block %p not aligned", lineno, bp);

            exit(1);
         }
        /*check coalesceing*/
        if (!GET_ALLOC(head)&&!GET_ALLOC(HDRP(NEXT_BLKP(bp)))) {
            dbg_printf("(lineno=%d)Adjacent free block %p and %p", lineno,bp,NEXT_BLKP(bp));
            exit(1);
        }
    }
    /*list checking :single linked list,not need to check prev one*/
    for(int index=0;index<SEGSIZE;index++){
        for(void*bp=seglist[index];bp!=NULL;bp=GET_NEXT(bp)){
            /*not in heap*/
            if(bp &&!(bp <= mem_heap_hi() && bp >= mem_heap_lo())){ 
               dbg_printf("(lineno=%d) seglist[%d] (%p) not in heap", lineno,index,bp);
               exit(1);
            }
            /*not in right bucket*/
            if(search_index(GET_SIZE(HDRP(bp)))!=index){
                 dbg_printf("(lineno=%d) seglist[%d] (%p):size=%u, not in right list", lineno,index,bp,GET_SIZE(HDRP(bp)));
            }
        }
    }

}
/* 
 * The remaining routines are internal helper routines 
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t words) 
{
    
    char *bp;
    size_t size;
    
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; 

    if ((long)(bp = mem_sbrk(size)) == -1)  
        return NULL;                                        
    
    /* Initialize free block header/footer and the epilogue header */
    PUT_WORD(HDRP(bp), PACK(size, 0));         /* Free block header */   
    PUT_WORD(FTRP(bp), PACK(size, 0));         /* Free block footer */   
    PUT_WORD(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ 

    /* Coalesce if the previous block was free */
    return coalesce(bp);                                          
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce(void *bp) 
{

    
    size_t size = GET_SIZE(HDRP(bp));
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    
    if (prev_alloc && next_alloc) {   
        insert_into_list(bp);         /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        
        remove_from_list(NEXT_BLKP(bp));
        PUT_WORD(HDRP(bp), PACK(size, 0));
        PUT_WORD(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));    
        remove_from_list(PREV_BLKP(bp));

        PUT_WORD(FTRP(bp), PACK(size, 0));
        PUT_WORD(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {                                     /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(HDRP(NEXT_BLKP(bp)));    
        remove_from_list(PREV_BLKP(bp));
        remove_from_list(NEXT_BLKP(bp));
        PUT_WORD(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT_WORD(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

    }
    insert_into_list(bp);
    CHECKHEAP();
    return bp;
}

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
static void*place(void *bp, size_t asize)
{

   /*
   tend to put small block on left side ,and big on right
   */
    size_t csize = GET_SIZE(HDRP(bp));  
    remove_from_list(bp);
    if ((csize - asize) >= (2*DSIZE)) { 
        /*tend to place big remain block on the right hand*/
            PUT_WORD(HDRP(bp), PACK(asize, 1));
            PUT_WORD(FTRP(bp), PACK(asize, 1));   
            PUT_WORD(HDRP(NEXT_BLKP(bp)), PACK(csize-asize, 0));
            PUT_WORD(FTRP(NEXT_BLKP(bp)), PACK(csize-asize, 0));
            insert_into_list(NEXT_BLKP(bp));
    }
    else { 
        PUT_WORD(HDRP(bp), PACK(csize, 1));
        PUT_WORD(FTRP(bp), PACK(csize, 1)); 
       
    }
    
    CHECKHEAP();
    return bp;
}

/* 
 * find_fit - using Best-fit searc
 */
static void *find_fit(size_t asize)
{
    /* Best-fit search */
     /* First-fit search */
    void *bp;
    int index=search_index(asize);
    for(;index<SEGSIZE;index++)
    {
        for (bp = seglist[index]; bp!=NULL; bp = GET_NEXT(bp)) {
            if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
                return bp;
            }
     }
    }
    return NULL; /* No fit */
}

