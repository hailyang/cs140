#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#define sys_arg(TYPE, INDEX) (*(TYPE *)(f->esp +INDEX))

static void syscall_handler (struct intr_frame *);

static void sys_halt (void);
static void sys_exit (int status);
/*
static pid_t sys_exec (const char *cmd_line);
static int sys_wait (pid_t pid);
static bool sys_create (const char *file_name, unsigned initial_size);
static bool sys_remove (const char *file_name);
static int sys_open (const char *file_name);
static int sys_filesize (int fd);
static int sys_read (int fd, void *buffer, unsigned size);
static int sys_write (int fd, const void *buffer, unsigned size);
static void sys_seek (int fd, unsigned position);
static unsigned sys_tell (int fd);
static void sys_close (int fd);
*/
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
//  hex_dump(f->esp, f->esp, 512, true);
  int syscall_no = *((int *)f->esp);  //sys_arg(int, 0);
  printf ("system call %d!\n", syscall_no); 
  switch(syscall_no) {
	case SYS_HALT: 
		sys_halt();
		break;
	case SYS_EXIT:
		sys_exit(sys_arg(int, 1));
		break;
/*	case SYS_EXEC:
		f->eax = (uint32_t) sys_exec(sys_arg(char *, 1));
		break;
	case SYS_CREATE:
		f->eax = (uint32_t) sys_create(sys_arg(char *, 1));
		break;
	case SYS_REMOVE:
		f->eax = (uint32_t) sys_remove(sys_arg(char *, 1));
		break;
	case SYS_WAIT: 
		f->eax = (uint32_t) sys_wait(sys_arg(pid_t, 1));
		break;
	case SYS_OPEN:
		f->eax = (uint32_t) sys_open(sys_ar(char *, 1));
		break;
	case SYS_FILESIZE:
		f->eax = (uint32_t) sys_fileszie(sys_arg(int,1));
		break;
	case SYS_READ:
		f->eax = (uint32_t) sys_read(sys_arg(int, 1), sys_arg(void *,2),
						sys_arg(unsigned, 3));
		break;
	case SYS_WRITE:
		f->eax = (uint32_t) sys_write(sys_arg(int, 1), sys_arg(void *,2),
                                                sys_arg(unsigned, 3));
		break;
	case SYS_SEEK:
		sys_seek (sys_arg(int, 1), sys_arg(unsigned, 2));
		break;
	case SYS_TELL:
		f->eax = (uint32_t)sys_tell(sys_arg(int, 1));
		break;
	case SYS_CLOSE:
		sys_close(sys_arg(int, 1));
		break;
*/	default:
		break;
  }

  thread_exit ();
}

static void 
sys_halt(void)
{
	shutdown_power_off();
}

static void 
sys_exit(int status) {}
