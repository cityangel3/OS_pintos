#include<stdio.h>
#include<stdlib.h>
#include<syscall.h>
int main(int argc,char **argv){
	int i,p[4];
	for(i=1;i<=4;i++){
		p[i-1] = atoi(argv[i]);
	}
	printf("%d %d %d %d\n",p[0],p[1],p[2],p[3]);
	printf("%d ",fibonacci(p[0]));
	printf("%d\n",max_of_four_int(p[0],p[1],p[2],p[3]));
	return EXIT_SUCCESS;
}
