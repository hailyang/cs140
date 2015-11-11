#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "vm/mmap.h"
void syscall_init (void);
void syscall_munmap (struct mmap_entry *me);

#endif /* userprog/syscall.h */
