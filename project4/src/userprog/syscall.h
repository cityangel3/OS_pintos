#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/user/syscall.h"
void syscall_init (void);

void protect_user_memory(const void* add);
void exit(int num);
void halt(void);
pid_t exec(const char *cmd_line);
int wait(pid_t pid);
int read(int fd,void* buffer,unsigned size);
int write(int fd,const void* buffer,unsigned size);
int fibonacci(int n);
int max_of_four_int(int a,int b,int c,int d);
/*---belows are pj2---*/
bool create(const char*file,unsigned initial_size);
bool remove(const char*file);
int open(const char*file);
int filesize(int fd);
void seek(int fd,unsigned position);
unsigned tell(int fd);
void close(int fd);

struct lock mut,w;

#endif /* userprog/syscall.h */
