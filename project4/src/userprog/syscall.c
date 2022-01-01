#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "devices/input.h"

#include "filesys/filesys.h"
#include "filesys/file.h"
#include <stdbool.h>
#include "threads/synch.h"
static void syscall_handler (struct intr_frame *);
int readcnt;
void
syscall_init (void) 
{
  lock_init(&mut);
  lock_init(&w);
  readcnt = 0;
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
			for(i=0;i<2;i++){
				p[i] = *(uint32_t *)(f->esp+(4*(i+1)));
				protect_user_memory((const void*)p[i]);
			}
			if(p[0] == NULL)
				exit(-1);
			f->eax = create((const char*)p[0],(unsigned)p[1]);
			break;
		case SYS_REMOVE:
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
			if(p[0] == NULL)
				exit(-1);
			f->eax = remove((const char*)p[0]);
			break;
		case SYS_OPEN:
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
			if(p[0] == NULL)
				exit(-1);
			f->eax = open((const char*)p[0]);
			break;
		case SYS_FILESIZE:
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
			f->eax = filesize((int)p[0]);
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
			for(i=0;i<2;i++){
				p[i] = *(uint32_t *)(f->esp+(4*(i+1)));
				protect_user_memory((const void*)p[i]);
			}
			seek((int)p[0],(unsigned)p[1]);
			break;
		case SYS_TELL:
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
			f->eax = tell((int)p[0]);
			break;
		case SYS_CLOSE:
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
			close((int)p[0]);
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
	struct thread* cur = thread_current();
	printf("%s: exit(%d)\n",cur->name,num);
	cur->exit_number = num; 
	int fd = 2;
  	//close each file descriptor 
  	while(++fd <= 127){
  		if(cur->Fd[fd]){
  	  		close(fd);
		}
  	}
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
	int cur,rbytes;
	struct file* fcur = thread_current()->Fd[fd];
	if(!fcur)
		exit(-1);
	if(fd !=0 && fd <= 2)
		exit(-1);
	//first readers-writers lock
	lock_acquire(&mut);
	readcnt++;
	if(readcnt == 1)
		lock_acquire(&w);
	lock_release(&mut);

	if(fd > 2){
		rbytes = file_read(fcur,buffer,size);
	}
	else{
		cur = 0;
		while(((uint8_t*)buffer)[cur]!='\0'){
			((uint8_t*)buffer)[cur++] = input_getc();
			if(size <= cur)
				break;
		}
		rbytes = --cur;
	}

	lock_acquire(&mut);
	readcnt--;
	if(readcnt == 0)
		lock_release(&w);
	lock_release(&mut);

	return rbytes;
}
int write(int fd,const void* buffer,unsigned size){ //pj1 only for stdout(1)
	int wbytes;
	struct file* fcur = thread_current()->Fd[fd];
	if(fd != 1){
		if(fd <= 2)
			exit(-1);
		else if(!fcur)
			exit(-1);
		lock_acquire(&w);
		wbytes = file_write(fcur,buffer,size);
	}
	else{
		lock_acquire(&w);
		putbuf(buffer, size); //from lib/kernel/consol.c(stdio.h)
		wbytes = (int)size;
	}
	lock_release(&w);
	return wbytes;	
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
bool create(const char*file,unsigned initial_size){/*creates a new file*/
	return filesys_create(file,initial_size); //function from filesys/filesys.h
}
bool remove(const char*file){ /*deletes the file called file*/
	return filesys_remove(file); //filesys/filesys.h
}
int open(const char*file){/*opens the file called file*/
	struct file *new;
	int count;
	struct thread* cur = thread_current();
	lock_acquire(&w);
	count = 2;
	if((new = filesys_open(file)) == NULL){
		lock_release(&w);
		return -1;
	}
	while((cur->Fd[++count]) != NULL){
		if(count >= 127){
			lock_release(&w);
			return -1;
		}
	}
	//pintos doesn't want to delete executable file of running program.
	if(!(strcmp(file, cur->name)))
		file_deny_write(new);
	cur->Fd[count] = new;
	
	lock_release(&w);
	return count;
}
int filesize(int fd){/*returns the size*/
	struct file* fcur = thread_current()->Fd[fd];
	if(fcur)
		return file_length(fcur); //filesys/file.h
	else
		exit(-1);
	return 0;
}
void seek(int fd,unsigned position){/*changes the next byte to be read or written*/
	struct file* fcur = thread_current()->Fd[fd];
	if(fcur)
		file_seek(fcur,position); //filesys/file.h
	else
		exit(-1);
}
unsigned tell(int fd){/*returns the position of the next byte to be read or written*/
	struct file* fcur = thread_current()->Fd[fd];
	if(fcur)
	       	return file_tell(fcur); //filesys/file.h	
	else
		exit(-1);
}
void close(int fd){/*close file descriptor fd*/
	struct file* fcur = thread_current()->Fd[fd];
	if(fcur){
		file_close(fcur); //filesys/file.h->cause doesn't init fd there
		thread_current()->Fd[fd] = NULL; //we should init fd here.
	}
	else{
		exit(-1);
	}
}
