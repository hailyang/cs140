#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <user/syscall.h>
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "userprog/process.h"

#define sys_arg(TYPE, INDEX) (*((TYPE *)(f->esp +INDEX*4)))

static void syscall_handler (struct intr_frame *);

static void sys_halt (void);
static void sys_exit (int status);
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
static bool buffer_read(uint8_t *dst, uint8_t *src, int size);
static bool buffer_write(uint8_t *dst, uint8_t *src, int size);
static void  cmd_validator(const char *str, int max_len);
static bool args_validator(struct intr_frame *f, int nr);

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}
 
/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}

static bool
buffer_read(uint8_t *dst, uint8_t *src, int size)
{
  if (((unsigned) src + size) > (unsigned) PHYS_BASE)
	return false;
  int i;
  int readin;
  for (i = 0; i<size; i++) 
   {
	readin = get_user(src++);
	if (readin == -1)
	  return false;
	*dst = (uint8_t) readin;
   }  
  return true;
}

static bool
buffer_write(uint8_t *dst, uint8_t *src, int size)
{
  if (((unsigned)dst + size) > (unsigned)PHYS_BASE)
       return false;
  int i;
  bool ret;
  for (i = 0; i<size; i++)
    {
	ret = put_user(dst++, (uint8_t)(*src++));
	if (ret == false)
	   return false;
    }
  return true;
}

static bool
args_validator(struct intr_frame *f, int nr)
{
  int i;
  int readin;
  for (i = 1; i<=nr; i++)
  {
     uint32_t uaddr = f->esp + 4*i;
     //printf ("args_validator buffer = 0x%.8x, with nr %d\n", uaddr, nr);
     if (uaddr > (unsigned) PHYS_BASE) {
        thread_exit();
     }
     int j;
     for (j = 0; j<4; j++)
        {
           readin = get_user((uint8_t *)(uaddr + j));
           if (readin == -1) {
		thread_exit();
	   }
        }
  }
}

static void
cmd_validator (const char *str, int max_len)
{
	int i;
	int result;
	for (i = 0; i < max_len; str++)
		{
			if (str >= PHYS_BASE)
				thread_exit ();
			result = get_user (str);
			if (result == -1)
				thread_exit ();
			if (result == '\0')
				return;
		}
	thread_exit ();
}


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  //hex_dump(f->esp, f->esp, 512, true);
  int syscall_no = sys_arg(int, 0);
  //printf("syscall no is %d\n", syscall_no);
  switch(syscall_no) {
	case SYS_HALT: 
		sys_halt();
		break;
	case SYS_EXIT:
		args_validator(f, 1);
		sys_exit(sys_arg(int, 1));
		break;
	case SYS_EXEC:
		args_validator(f, 1);
		f->eax = (uint32_t) sys_exec(sys_arg(char *, 1));
		break;
	case SYS_CREATE:
		args_validator(f, 2);
		f->eax = (uint32_t) sys_create(sys_arg(char *, 1),
					sys_arg(unsigned, 2));
		break;
	case SYS_REMOVE:
		args_validator(f, 1);
		f->eax = (uint32_t) sys_remove(sys_arg(char *, 1));
		break;
	case SYS_WAIT: 
		args_validator(f, 1);
		f->eax = (uint32_t) sys_wait(sys_arg(pid_t, 1));
		break;
	case SYS_OPEN:
		args_validator(f, 1);
		f->eax = (uint32_t) sys_open(sys_arg(char *, 1));
		break;
	case SYS_FILESIZE:
		args_validator(f, 1);
		f->eax = (uint32_t) sys_filesize(sys_arg(int,1));
		break;
	case SYS_READ:
		args_validator(f, 3);
		f->eax = (uint32_t) sys_read(sys_arg(int, 1), sys_arg(void *,2),
						sys_arg(unsigned, 3));
		break;
	case SYS_WRITE:
		args_validator(f, 3);
		f->eax = (uint32_t) sys_write(sys_arg(int, 1), sys_arg(void *,2),
                                                sys_arg(unsigned, 3));

		break;
	case SYS_SEEK:
		args_validator(f, 2);
		sys_seek (sys_arg(int, 1), sys_arg(unsigned, 2));
		break;
	case SYS_TELL:
		args_validator(f, 1);
		f->eax = (uint32_t)sys_tell(sys_arg(int, 1));
		break;
	case SYS_CLOSE:
		args_validator(f, 1);
		sys_close(sys_arg(int, 1));
		break;
	default:
		break;
  } 
  return;
}

static void 
sys_halt(void)
{
	shutdown_power_off();
}

static void 
sys_exit(int status) 
{ 
	struct thread *cur;
	enum intr_level old_level;
	cur = thread_current();
	cur->process_status = status;

	old_level = intr_disable();
	if (cur->parent != NULL) 
	  {
		cur->child_process_elem->status = status;
	  }
	thread_exit();
}

static pid_t
sys_exec (const char *cmd_line)
{
	cmd_validator(cmd_line, PGSIZE);
	tid_t t = process_execute(cmd_line);
	return t;
}

static int
sys_wait (pid_t pid)
{
	return process_wait((tid_t) pid);
}

static bool
sys_create (const char *file, unsigned initial_size)
{
	printf ("sys_create: file = %s, initial_size = %d\n", file, initial_size);
}

static bool
sys_remove (const char *file)
{
	printf ("sys_remove: file = %s\n", file);
}

static int
sys_open (const char *file)
{
	printf ("sys_open: file = %s\n", file);
}

static int
sys_filesize (int fd)
{
	printf ("sys_filesize: fd = %d\n", fd);
}

static int
sys_read (int fd, void *buffer, unsigned size)
{
	printf ("sys_read: fd = %d, buffer = 0x%.8x, size = %d\n", fd, (unsigned) buffer, size);
}

static int
sys_write (int fd, const void *buffer, unsigned size)
{
  uint8_t *readin_buf = (uint8_t *)malloc(size);
  if (readin_buf == NULL)
	return -1;

  if (buffer_read(readin_buf, buffer, size) == false)
    {
	free(readin_buf);
	thread_exit();
    }
  else 
    {
	if (fd == 1) 
   	  {
	    putbuf(buffer, size);
	  }
	
	free(readin_buf);
    }	  
}

static void
sys_seek (int fd, unsigned position)
{
	printf ("sys_seek: fd = %d, position = %d\n", fd, position);
}

static unsigned
sys_tell (int fd)
{
	printf ("sys_tell: fd = %d\n", fd);
}

static void
sys_close (int fd)
{
	printf ("sys_close: fd = %d\n", fd);
}


