#include <stdio.h>
#include <stdlib.h>

int main() {
    char c;
    printf("nihao\n");
    
    while ((c = getchar()) != EOF) {
	putchar(c);
    }
    exit(0);
}
