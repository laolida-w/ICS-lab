/* 
 * myhup.c - Sends a SIGHUP to itself, terminates when restarted.
 */ 
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>

int main() 
{
    printf("hello0\n");
    if (kill(getpid(), SIGHUP) < 0) {
    printf("hello1\n");
	perror("kill");
	exit(1);
    }
    printf("hello2\n");
    exit(0);
}
