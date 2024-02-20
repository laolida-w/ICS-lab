#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE 10
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

typedef struct{
    int time;
    char content[MAX_OBJECT_SIZE];
    char request[MAXLINE];
    int valid;
    int size;
}Block;
typedef struct{
    
    Block blocks[MAX_CACHE];
    int cache_time;
   
}Cache;//直相连

Cache cache;
sem_t  ww;
    sem_t mutex;
    int readcnt=0;
void *thread(void *vargp);
void doit(int clientfd);
void parse_uri(char *uri,char *hostname,char *path,char *port);
void send_request(rio_t *rp,int fd,char*hostname,char*port);
void response(int serverfd, int clientfd,char*request);

void cache_init(Cache*cache);
void cache_write(Cache*cache,char*content,char*request,int size);
int cache_find(Cache*cache,char*request,int clientfd);

void sigchld_handler(int sig) { // reap all children
    int bkp_errno = errno;
    while(waitpid(-1, NULL, WNOHANG)>0);
    errno=bkp_errno;
}
/* main */
int main(int argc, char **argv) 
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, sigchld_handler);
    
    cache_init(&cache);
    Sem_init(&mutex, 0, 1);
    Sem_init(&ww, 0, 1);
    int listenfd;
    int*connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;//client长度
    struct sockaddr_storage clientaddr;
    pthread_t tid; 
    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }

    listenfd = open_listenfd(argv[1]);//代理创建监听描述符
    while (1) {

	clientlen = sizeof(clientaddr);
    connfd = Malloc(sizeof(int));
	*connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //简单地避免竞争
    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                port, MAXLINE, 0);// 
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    Pthread_create(&tid, NULL, thread, connfd);
   
    }
}
/* Thread routine */
void *thread(void *vargp) 
{  
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self()); //line:conc:echoservert:detach
    Free(vargp);                    //line:conc:echoservert:free
    doit(connfd);
    Close(connfd);
    return NULL;
}
/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int clientfd) 
{
    
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE],path[MAXLINE],port[MAXLINE],request_head[2*MAXLINE+24];
    rio_t rio;
    /* Read request line and headers */
    Rio_readinitb(&rio, clientfd);
    if (!rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
        return;
    printf("%s", buf);
   
    sscanf(buf, "%s %s %s", method, uri, version);     
    //uri:http://www.cmu.edu/hub/index.html

    if (strcasecmp(method, "GET")) {                   
       printf("Not Implemented" );
        return;
    } 
                    
    if(cache_find(&cache,uri,clientfd)!=0){
        return ;
        
    }
    else{
    char uri_[MAXLINE];
    strcpy(uri_,uri);
    parse_uri(uri_,hostname,path,port);
    //hostname:www.cmu.end   port(optional):8080   path:/hub/index.html
    sprintf(request_head,"GET %s HTTP/1.0\r\nHost: %s\r\n",path,hostname);
    //request_head : GET /hub/index.html HTTP/1.0
    int serverfd = Open_clientfd(hostname,port);   //与服务器建立连接    
    if (serverfd < 0)
    {
        printf("Error: connect to server failed\n");
        return;
    }
    Rio_writen(serverfd,request_head,strlen(request_head));    //向服务器发送请求行

    send_request(&rio, serverfd,hostname,port);   //向服务器发送请求报头        

    response(clientfd,serverfd,uri);  //将服务器要发送的信息发送给客户端

    Close(serverfd);
    }
    
}

/*parse_uri*/
void parse_uri(char *uri,char *hostname,char *path,char *port){
   
    strcpy(port,"80");     //默认值
      char*bg;
    if (strstr(uri, "http://") == uri) {//must begin with "http://"
		bg=uri+strlen("http://");
	}
    else
        bg=uri;
    char*port_bg = strchr(bg,':');
    char*path_bg= strchr(bg,'/');
    if(path_bg!=NULL){
        strcpy(path,path_bg);
        *path_bg = '\0';
    }
    else
        strcpy(path, "/");

    if(port_bg!=NULL){
        strcpy(port,port_bg+1);
        *port_bg = '\0';
    }
    strcpy(hostname, bg);
}
/*
*读取请求报头
*sample:Host:www.cmu.edu
*忽略除了Host之外的报头
*/
void send_request(rio_t *rp,int fd,char*hostname,char*port){ 
    
    char buf[MAXLINE];

    sprintf(buf, "Host: %s:%s\r\n", hostname, port);
	Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "%s", user_agent_hdr);
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Connection: close\r\n");
    Rio_writen(fd, buf, strlen(buf));

    sprintf(buf, "Proxy-Connection: close\r\n");
    Rio_writen(fd, buf, strlen(buf));

    for(Rio_readlineb(rp,buf,MAXLINE);strcmp(buf,"\r\n");Rio_readlineb(rp,buf,MAXLINE)){
        if(strncmp("Host",buf,4) == 0 
        || strncmp("User-Agent",buf,10) == 0
        || strncmp("Connection",buf,10) == 0 
        || strncmp("Proxy-Connection",buf,16) == 0)
                continue;
        Rio_writen(fd,buf,strlen(buf));
    }
    Rio_writen(fd,buf,strlen(buf));
    return;
}
void response(int clientfd,int serverfd,char*request){
    char buf[MAXLINE],content[MAX_OBJECT_SIZE];
    int n,size=0;
    rio_t rio;

    Rio_readinitb(&rio,serverfd);
    while((n = Rio_readlineb(&rio,buf,MAXLINE)) != 0){
        Rio_writen(clientfd,buf,n);
        if(n+size<MAX_OBJECT_SIZE){
            memcpy(content+size,buf,n);
            size+=n;
        }
    }
    if(size<MAX_OBJECT_SIZE)
    cache_write(&cache,content,request,size);
}
/*cache*/
void cache_init(Cache*cache){
    cache->cache_time=0;
    
    for(int i = 0;i<MAX_CACHE;i++){
        cache->blocks[i].time=0;
          cache->blocks[i].valid=0;
          cache->blocks[i].size=0;
    }
    
}
int maxlrucache(Cache*cache){
    int i;
    int max=0;
    for(i = 0;i<MAX_CACHE;i++){
        if(cache->blocks[i].time > max){
            max = cache->blocks[i].time;
        }
    }
    return max;
}

void cache_write(Cache*cache,char*content,char*request,int size){
   
   int index=0;
   int minlru = cache->blocks[0].time;
   P(&ww);
   for(int i=0;i<MAX_CACHE;i++){
        if(cache->blocks[i].valid==0){
            index=i;
           cache->blocks[i].valid=1;
            break;
        }
        if(cache->blocks[i].time < minlru){
                minlru = cache->blocks[i].time;
                index = i;
         }
   }
    cache->blocks[index].time = ++cache->cache_time;
    cache->blocks[index].size=size;
    strcpy(cache->blocks[index].request,request);
    memcpy(cache->blocks[index].content,content,size);
    V(&ww);
}

int cache_find(Cache*cache,char*request,int clientfd){
    printf("find_called\n");
    printf("[]%s\n",request);
    P(&mutex);
    readcnt++;
    if(readcnt==1)
        P(&ww);
    V(&mutex);
    for(int i=0;i<MAX_CACHE;i++){
        printf("[%d]%s\n",i,cache->blocks[i].request);
        if(!strcmp(request,cache->blocks[i].request)){
            Rio_writen(clientfd,cache->blocks[i].content,cache->blocks[i].size);
            cache->blocks[i].time=++cache->cache_time;
           printf("finded\n");
            break;
        }
    }
    P(&mutex);
    readcnt--;
    if(readcnt==0)
        V(&ww);
    V(&mutex);
    return 0;
}