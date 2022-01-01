#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "devices/input.h"
#include "lib/kernel/list.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include <stdbool.h>
#include "threads/synch.h"

enum file_or_dir { F=1, D=2 };
static struct Fd* get_file(int dnum, enum file_or_dir flag)
{
  struct thread* tcur = thread_current();
  struct list_elem *ecur;
  if(tcur != NULL && dnum >=3){
  if (! list_empty(&tcur->fd_head)) {
    for(ecur = list_begin(&tcur->fd_head);
        ecur != list_end(&tcur->fd_head); ecur = list_next(ecur))
    {
      struct Fd *fcur = list_entry(ecur, struct Fd, elem);
      if(fcur->num == dnum) {
        if (fcur->dir != NULL && (flag & D) )
          return fcur;
        else if (fcur->dir == NULL && (flag & F) )
          return fcur;
      }
    }
  }
  }
  return NULL; 
}

static void syscall_handler (struct intr_frame *);
int readcnt;
void
syscall_init (void) 
{
  lock_init(&mut);
  lock_init(&w);
  lock_init(&filesys_lock);
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
#ifdef FILESYS
 		case SYS_CHDIR: 
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
	     	 	f->eax = chdir((const char*)p[0]);
		     	break;
  		case SYS_MKDIR: 
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
     			 f->eax = mkdir((const char*)p[0]);
      			break;
 		case SYS_READDIR: 
     		 	for(i=0;i<2;i++){
			p[i] = *(uint32_t *)(f->esp+(4*(i+1)));
			protect_user_memory((const void*)p[i]);
			}
    		  	f->eax = readdir((int)p[0],(char*)p[1]);
     			 break;
  		case SYS_ISDIR: 
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
      			f->eax = isdir((int)p[0]);
      			break;
  		case SYS_INUMBER: 
			p[0] = *(uint32_t *)(f->esp+4);
			protect_user_memory((const void*)p[0]);
     			f->eax = inumber((int)p[0]);
    			  break;
#endif
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
#ifdef FILESYS
bool chdir(const char *filename)
{
  return filesys_chdir(filename);
}

bool mkdir(const char *filename)
{
  return filesys_create(filename, 0, true);
}

bool readdir(int fd, char *name)
{
  struct Fd* fcur = get_file(fd, D);
  struct inode *id;
  if (fcur == NULL) 
	  return -1;
  id = file_get_inode(fcur->file); 
  if(id == NULL || !is_inode_dir(id))
	  return -1;
  ASSERT (fcur->dir != NULL); 
  return dir_readdir (fcur->dir, name);
}
bool isdir(int fd)
{
  struct Fd* fcur = get_file(fd, F | D);
  return is_inode_dir(file_get_inode(fcur->file));
}
int inumber(int fd)
{
  struct Fd* fcur = get_file(fd, F | D);
  return (int) inode_get_inumber (file_get_inode(fcur->file));
}
#endif
int read(int fd,void* buffer,unsigned size){//pj1 only for stdin(0)
  struct Fd* fcur;
  int rbytes,cur;
  lock_acquire (&w);
  if(fd != 0) { 
    fcur = get_file(fd, F);
    if(fcur != NULL && fcur->file != NULL)
      rbytes = file_read(fcur->file, buffer, size);
    else 
      rbytes = -1;
  }
  else {
    cur = 0;
    while(((uint8_t*)buffer)[cur]!='\0'){
	((uint8_t*)buffer)[cur++] = input_getc();
	if(size <= cur)
		break;
    }
    rbytes = --cur;
  }
  lock_release (&w);
  return rbytes;
}
int write(int fd,const void* buffer,unsigned size){ //pj1 only for stdout(1)
 struct Fd* fcur; 
 int wbytes;
 lock_acquire (&w);
  if(fd != 1) { 
    fcur = get_file( fd, F);
    if(fcur && fcur->file) 
      wbytes = file_write(fcur->file, buffer, size);
    else 
      wbytes = -1;
  }
  else {
    putbuf(buffer, size);
    wbytes = (int)size;
  }
  lock_release (&w);
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
	return filesys_create(file,initial_size,false); //function from filesys/filesys.h
}
bool remove(const char*file){ /*deletes the file called file*/
	return filesys_remove(file); //filesys/filesys.h
}
int open(const char*file){/*opens the file called file*/
  struct thread* cur = thread_current();
  struct file* fnew;
  struct list* fhead;
  struct inode* id;
  struct Fd* fcur = palloc_get_page(0);
  if (fcur == NULL)
    return -1;
  lock_acquire (&w);
  fnew = filesys_open(file);
  if (fnew == NULL) {
    palloc_free_page(fcur);
    lock_release (&w);
    return -1;
  }
  if(!(strcmp(file, cur->name)))
	file_deny_write(fnew);
  fcur->file = fnew; 
  id = file_get_inode(fcur->file);
  if(id != NULL && is_inode_dir(id))
    fcur->dir = dir_open(inode_reopen(id));
  else 
    fcur->dir = NULL;
  fhead = &cur->fd_head;
  if (list_empty(fhead))
    fcur->num = 3;
  else
    fcur->num = (list_entry(list_back(fhead),struct Fd,elem)->num) + 1;
  list_push_back(fhead, &(fcur->elem));
  lock_release (&w);
  return fcur->num;
}
int filesize(int fd){/*returns the size*/
  struct Fd* fcur = get_file(fd,F);
  if(fcur== NULL)
    return -1;
  return file_length(fcur->file);
}
void seek(int fd,unsigned position){/*changes the next byte to be read or written*/
  struct Fd* fcur = get_file(fd, F);
  if(fcur != NULL && fcur->file != NULL)
    file_seek(fcur->file, position);
  return; 
}
unsigned tell(int fd){/*returns the position of the next byte to be read or written*/
  struct Fd* fcur = get_file(fd, F);
  if(fcur != NULL && fcur->file != NULL)
    return file_tell(fcur->file);
  else
    return -1; 
}
void close(int fd){/*close file descriptor fd*/
  struct Fd* fcur = get_file(fd, F | D);
  if(fcur != NULL && fcur->file != NULL) {
    file_close(fcur->file);
    if(fcur->dir) 
	    dir_close(fcur->dir);
    list_remove(&(fcur->elem));
    palloc_free_page(fcur);
  }
  else
	  exit(-1);
}

