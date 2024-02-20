#include "cachelab.h"
#include <getopt.h> // getopt
#include <stdlib.h> // atoi
#include<stdio.h>
#include<string.h>

FILE* fp; // 定义文件指针

 //行定义
int s,E,b,S;
char operation;
unsigned long  address;
int size;
int time=0;
int hit_cnt=0,miss_cnt=0,evi_cnt=0;
//cache部分构成
struct line {
    int valid; // 有效位
    int tag; // 标记
    int last_time; // 最后使用时间
};
//每组E行
typedef struct line* set;
//一共s组
set* cache; 
//初始化缓存
void init_cache(){
   
    cache=(set*)malloc(sizeof(set)*S);
    for(int i = 0; i < S; ++i)
	{
		cache[i] = (set)malloc(sizeof(struct line) * E);
		for(int j = 0; j < E; ++j)
		{
			cache[i][j].valid = 0;
			cache[i][j].tag = -1;
			cache[i][j].last_time = -1;
		}
	}
    
}
void update(unsigned long address){
     time++;
    int set_=(address>>b)&(0xffffffffffffffff>>(64-s)),tag_=address>>(s+b);
    for(int i=0;i<E;++i){//先看是否命中
        if(cache[set_][i].tag==tag_){
            
            cache[set_][i].last_time=time;
            hit_cnt++;
            return;
        }
    }
    for(int i=0;i<E;++i){//再看是否还有空行
        if(cache[set_][i].valid==0){
            miss_cnt++;
            cache[set_][i].tag=tag_;
            cache[set_][i].valid=1;
            cache[set_][i].last_time = time;//更新使用时间
            return;
        }
    }
    int earliest_time=0x7fffffff,earliest_val=0;
    miss_cnt++;
    evi_cnt++;
    for(int i=0;i<E;++i){//最后进行替换
        if(cache[set_][i].last_time<earliest_time){
            earliest_time=cache[set_][i].last_time;
            earliest_val=i;
        }
    }
    cache[set_][earliest_val].tag=tag_;
    cache[set_][earliest_val].last_time=time;
    return;
}
void operate_trace()
{
	char operation;         // 命令开头的 I L M S
	unsigned long int address;   // 地址参数
	int size;               // 大小
	while(fscanf(fp, " %c %lx,%d\n", &operation, &address, &size) > 0)
	{
       
		switch(operation)
		{

			case 'I': continue;	   
			case 'L':
				update(address);
				break;
			case 'M':
                update(address);     
			case 'S':
                update(address);
                break;
            default:break;
		}
		 //printf("%lx %d %d %d\n",address,hit_cnt,miss_cnt, evi_cnt);
	}
	fclose(fp);
}
void printUsage()
{
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
            "Options:\n"
            "  -h         Print this help message.\n"
            "  -v         Optional verbose flag.\n"
            "  -s <num>   Number of set index bits.\n"
            "  -E <num>   Number of lines per set.\n"
            "  -b <num>   Number of block offset bits.\n"
            "  -t <file>  Trace file.\n\n"
            "Examples:\n"
            "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
            "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}
int main(int argc, char* argv[]) {
    int option;
    while ((option = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (option) {
        case 'h':
            printUsage();
            break;
        case 'v':
           break;
        case 's':
			s = atoi(optarg);
            S=1<<s;
			break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            fp = fopen(optarg, "r");
        default:
        printUsage();
        break;
        }
    }
    init_cache();  // 初始化cache
    operate_trace();
    for(int i=0;i<S;i++){
        free(cache[i]);
    }
    free(cache);
   printSummary(hit_cnt, miss_cnt, evi_cnt);
   return 0;
}