/*
name:Wang Yuhao
ID:2200017794
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
     int i, j;
      int t1,t2,t3,t4,t5,t6,t7,t8;
    if(N==32){
        for (i = 0; i < 32; i += 8) {      
            for (j = 0; j < 32; j += 8) {  
                for (int k = i; k < (i + 8); ++k) {  
                     t1 = A[k][j];                  
                     t2 = A[k][j + 1];
                    t3 = A[k][j + 2];
                     t4 = A[k][j + 3];
                     t5 = A[k][j + 4];
                     t6 = A[k][j + 5];
                     t7 = A[k][j + 6];
                     t8 = A[k][j + 7];
                    B[j][k] = t1;  
                    B[j + 1][k] = t2;
                    B[j + 2][k] = t3;
                    B[j + 3][k] = t4;
                    B[j + 4][k] = t5;
                    B[j + 5][k] = t6;
                    B[j + 6][k] = t7;
                    B[j + 7][k] = t8;
                }
            }
        }
    }
    else if(N==64){
        for (i = 0; i < 64; i += 8) {      
            for (j = 0; j < 64; j += 8) {  
                int t1,t2,t3,t4,t5,t6,t7,t8;
                for (int k = 0; k < 4; ++k,++i) {  
                    t1=A[i][j];                  
                    t2=A[i][j + 1];
                    t3=A[i][j + 2];
                    t4=A[i][j + 3];
                    t5=A[i][j + 4];
                    t6=A[i][j + 5];
                    t7=A[i][j + 6];
                    t8=A[i][j + 7];
                    B[j][i] = t1;  
                    B[j + 1][i] = t2;
                    B[j + 2][i] = t3;
                    B[j + 3][i] = t4;
                    B[j][i+4]=t5;
                    B[j+1][i+4]=t6;
                    B[j+2][i+4]=t7;
                    B[j+3][i+4]=t8;
                }
                i-=4;
                for(int k=0;k<4;++k,++j){
                    t5=A[i+4][j];
                    t6=A[i+5][j];
                    t7=A[i+6][j];
                    t1=B[j][i+4];
                    t8=A[i+7][j];
                    t2=B[j][i+5];
                    t3=B[j][i+6];
                    t4=B[j][i+7];
                    B[j][i+4]=t5;
                    B[j][i+5]=t6;
                    B[j][i+6]=t7;
                    B[j][i+7]=t8;
                    B[j+4][i]=t1;
                    B[j+4][i+1]=t2;
                    B[j+4][i+2]=t3;
                    B[j+4][i+3]=t4;
                }
                i+=4;
                for(int k=0;k<4;++k,++i){
                
                    t5=A[i][j];
                    t6=A[i][j+1];
                    t7=A[i][j+2];
                    t8=A[i][j+3];

                    B[j][i]=t5;
                    B[j+1][i]=t6;
                    B[j+2][i]=t7;
                    B[j+3][i]=t8;

                }
                    i-=8,j-=4;
            }
        }
    }
    else if (N==68){

        for (i = 0; i < 64; i += 8) {      
            for (j = 0; j < 60; j += 4) {  
                for (int k = i; k < (i + 8); ++k) {  
                    t1 = A[k][j];                  
                    t2 = A[k][j + 1];
                    t3 = A[k][j + 2];
                    t4 = A[k][j + 3];
                    
                    B[j][k] = t1;  
                    B[j + 1][k] = t2;
                    B[j + 2][k] = t3;
                    B[j + 3][k] = t4;
                    
                    
                }
                
            }
        }
        for (i = 64; i < 68; i +=4) {      
            for (j = 0; j < 60; j +=4) {  
                    for(int k=0;k<4;++k,++i){
                    t5=A[i][j];
                    t6=A[i][j+1];
                    t7=A[i][j+2];
                    t8=A[i][j+3];
                    B[j][i]=t5;
                    B[j+1][i]=t6;
                    B[j+2][i]=t7;
                    B[j+3][i]=t8;
                }
                
                i-=4;
            }
        }
   
     
    }
    ENSURES(is_transpose(M, N, A, B));
}


/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */



 /*
  * trans - A simple baseline transpose function, not optimized for the cache.
  */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);
    registerTransFunction(trans, trans_desc);
    /* Register any additional transpose functions */
  
    
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

