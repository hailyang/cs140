#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
struct child_process
{
	tid_t tid;
	int status;
	struct thread *thread;
	struct semaphore wait_sema;
	struct list_elem child_elem;
};

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void setup_process_children(void);
#endif /* userprog/process.h */
