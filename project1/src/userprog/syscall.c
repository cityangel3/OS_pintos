#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "devices/input.h"
static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int i,p[4];
	switch(*(uint32_t*)(f->esp)){
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
			exit(p[0]);
			break;
		case SYS_EXEC:
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
			f->eax = exec(p[0]);
			break;
		case SYS_WAIT:
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
			f->eax = wait((pid_t)p[0]);
			break;
		case SYS_CREATE:
			break;
		case SYS_REMOVE:
			break;
		case SYS_OPEN:
			break;
		case SYS_FILESIZE:
			break;
		case SYS_READ:
			for(i=0;i<3;i++){
				p[i] = *(uint32_t *)(f->esp+(4*(i+1)));
				protect_user_memory((const void*)p[i]);
			}
			f->eax = read((int)p[0],(void*)p[1],(unsigned)p[2]);
			break;
		case SYS_WRITE:
			for(i=0;i<3;i++){
				p[i] = *(uint32_t *)(f->esp+(4*(i+1)));
				protect_user_memory((const void*)p[i]);
			}
			f->eax = write((int)p[0],(void*)p[1],(unsigned)p[2]);
			break;
		case SYS_SEEK:
			break;
		case SYS_TELL:
			break;
		case SYS_CLOSE:
			break;
		case SYS_FIBO:
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
			f->eax = fibonacci((int)p[0]);
			break;
		case SYS_MAXFOUR:
			for(i=0;i<4;i++){
				p[i] = *(uint32_t *)(f->esp+(4*(i+1)));
				protect_user_memory((const void*)p[i]);
			}
			f->eax = max_of_four_int((int)p[0],(int)p[1],(int)p[2],(int)p[3]);
			break;
	}
	/*	
	printf("system call number : %u,address: %x\n",*(uint32_t*)(f->esp),(uint32_t)(f->esp));
	printf("f->esp+4:%u\n",*(uint32_t*)(f->esp+4));
	hex_dump((uintptr_t)(f->esp),f->esp,(0xc0000000-(uintptr_t)(f->esp)),1);
	*/
}
void exit(int num){
	printf("%s: exit(%d)\n",thread_current()->name,num);
	thread_current()->exit_number = num;
	thread_exit();
}
void protect_user_memory(const void* add){
	if(is_user_vaddr(add)) //from threads/vaddr.h
		return;
	else
		exit(-1);
}
void halt(void){
	shutdown_power_off(); //from devices/shutdown.h
}
pid_t exec(const char *cmd_line){ 
	return process_execute(cmd_line);
}
int wait(pid_t pid){ 
	return process_wait(pid);
}
int read(int fd,void* buffer,unsigned size){//pj1 only for stdin(0)
	if(fd != 0)
		return -1;
	int cur=0;
	while(((uint8_t*)buffer)[cur]!='\0'){
		((uint8_t*)buffer)[cur++] = input_getc();
		if(size <= cur)
			break;
	}
	return --cur;
}
int write(int fd,const void* buffer,unsigned size){ //pj1 only for stdout(1)
	if(fd != 1)
		return -1;
	putbuf(buffer, size); //from lib/kernel/consol.c(stdio.h)
	return size;	
}
int fibonacci(int n){
	int i,f1=1,f2=1,res=2;
	if(n == 1)
		return f1;
	else if(n == 2)
		return f2;
	else{
		for(i=3;i<=n;i++){
			res = f1 + f2;
			f1 = f2; 
			f2 = res;
		}
	}
	return res;
}
int max_of_four_int(int a,int b,int c,int d){
	int c1,c2;
	c1 = (a > b) ? a : b;
	c2 = (c > d) ? c : d;
	c1 = (c1 > c2) ? c1 : c2;
	return c1;
}
