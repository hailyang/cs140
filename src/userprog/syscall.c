#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <user/syscall.h>
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "userprog/process.h"
#include "filesys/filesys.h"

#define FD_MAX 32767 // Max of int.
#define END_OF_TEXT 3
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
static void cmd_validator(const char *str, int max_len);
static bool args_validator(struct intr_frame *f, int nr);
static bool args_validator_range(struct intr_frame *f, int beg, int nr);
static void file_validator(const char *file, int max);

static int get_next_fd(void);
static struct file* lookup_fd(int fd);

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
	dst++;
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
  return args_validator_range(f, 1, nr);
}

static bool
args_validator_range(struct intr_frame *f, int beg, int nr)
{
  int i;
  int readin;
  bool ret = true;
  for (i = beg; i<=nr; i++)
  {
     uint32_t uaddr = f->esp + 4*i;
     if (uaddr >= (unsigned) PHYS_BASE) {
        ret = false;
        thread_exit();
     }
     int j;
     for (j = 0; j<4; j++)
        {
           readin = get_user((uint8_t *)(uaddr + j));
           if (readin == -1) {
                ret = false;
                thread_exit();
           }
        }
  }
  return ret;
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

static void 
file_validator(const char *file, int max)
{
  int i;
  int ret;
  for (i = 0; i < max; file++)
    {
	if ((unsigned)file >= (unsigned)PHYS_BASE)
	   thread_exit();
	ret = get_user((uint8_t *)file);
	if (ret == -1)
	   thread_exit();
	if (ret == '\0')
	   return;
    }
  thread_exit();
}

static int
get_next_fd ()
{
  struct thread* cur = thread_current();
  int fd = cur->next_fd;
  if (fd == FD_MAX)
    return -1;
  else
    {
	cur->next_fd++;
  	return fd;
    }  
}

static struct file *
lookup_fd (int fd)
{
  struct thread *cur = thread_current();
  struct list_elem *e;
  for (e = list_begin(&cur->open_files); e != list_end(&cur->open_files);
	e = list_next (e))
    {
	struct fd_file *fd_f = list_entry(e, struct fd_file, file_elem);
	if (fd_f->fd == fd)
	  return fd_f->file;
    }
  return NULL;
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  bool ret = args_validator_range(f, 0, 0);
  int syscall_no = sys_arg(int, 0);
  switch(syscall_no) {
	case SYS_HALT: 
		sys_halt();
		break;
	case SYS_EXIT:
		ret = args_validator(f, 1);
		if (ret)
		  sys_exit(sys_arg(int, 1));
		else
		  sys_exit(-1);
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
  file_validator(file, PGSIZE);
  bool success;
  lock_acquire (&filesys_lock);
  success = filesys_create (file, initial_size);
  lock_release (&filesys_lock);
  return success;
}

static bool
sys_remove (const char *file)
{
  file_validator(file, PGSIZE);
  bool success;
  lock_acquire (&filesys_lock);
  success = filesys_remove (file);
  lock_release (&filesys_lock);
  return success;
}

static int
sys_open (const char *file)
{
  file_validator(file, PGSIZE);
  struct file *file_f;
  int fd;
  struct thread *cur = thread_current();
  lock_acquire (&filesys_lock);
  file_f = filesys_open (file);
  fd = get_next_fd();
  if ((file_f != NULL) && (fd != -1))
    {
	struct fd_file *fd_f = 
		(struct fd_file *) malloc (sizeof (struct fd_file));
	if (fd_f == NULL)
	  {
	     file_close (file_f);
	     lock_release (&filesys_lock);
	     return -1;
	  }
   	fd_f->fd = fd;
	fd_f->file = file_f;
	list_push_front(&cur->open_files, &fd_f->file_elem);
    }
  else
    {
         file_close (file_f);
         lock_release (&filesys_lock);
         return -1;
    }
  lock_release (&filesys_lock);
  return fd;
}

static int
sys_filesize (int fd)
{
  struct file *file = lookup_fd (fd);
  if (file == NULL)
    return -1;
  int file_size;
  lock_acquire (&filesys_lock);
  file_size = file_length (file);
  lock_release (&filesys_lock);
  return file_size;
}

static int
sys_read (int fd, void *buffer, unsigned size)
{
  int read_count = 0;
  struct file *file;
  uint8_t *readin_buf = NULL;
  
  if (size > 0) 
    {
	readin_buf = (uint8_t *)malloc (size);
	if (readin_buf == NULL)
	   return -1;
    } 
  // Read from console
  if (fd == STDIN_FILENO)
    {
	uint8_t in_char;
	for (read_count = 0; (unsigned) read_count < size; 
		read_count++)
   	  {
	     in_char = input_getc();
	     if (in_char == (uint8_t) END_OF_TEXT)
		break;
	     else
		readin_buf[read_count] = in_char;
	  }
    }
  else
    {
	file = lookup_fd (fd);
	if (file != NULL)
	  {
	     lock_acquire (&filesys_lock);
	     read_count = file_read (file, readin_buf, size);
	     lock_release (&filesys_lock);
	  }
 	else
  	  {
	     read_count = -1;
	  }
    }

  bool write_success = buffer_write (buffer, readin_buf, read_count);
  if (write_success)
    {
	if (readin_buf != NULL)
	   free (readin_buf);
	return read_count;
    }
  else
    {
	if (readin_buf != NULL)
	   free (readin_buf);
	thread_exit();
    }

}

static int
sys_write (int fd, const void *buffer, unsigned size)
{
  struct file *file;
  int write_count = 0;
  uint8_t *readin_buf = NULL;
  if (size > 0) 
    {
	readin_buf =(uint8_t *)malloc(size);
	if (readin_buf == NULL)
	  return -1;
    }

  if (buffer_read(readin_buf, buffer, size) == false)
    {
	if (readin_buf != NULL)
	   free(readin_buf);
	thread_exit();
    }
  if (fd == STDOUT_FILENO) 
    {
	while (size > 128)
	  {
	    putbuf(buffer, 128);
	    size -= 128;
	    write_count += 128;
	    buffer += 128;
    	  }
	putbuf (buffer, size);
	write_count += size;
    }
  else
    {
	file = lookup_fd (fd);
	if (file != NULL)
	  {
	    lock_acquire (&filesys_lock);
	    write_count = file_write (file, readin_buf, size);
	    lock_release (&filesys_lock);
	  }
	else 
	  write_count = -1;
    }	  
  
  if (readin_buf != NULL)
	free (readin_buf);
  return write_count;
}

static void
sys_seek (int fd, unsigned position)
{
  struct file *file = lookup_fd (fd);
  if (file != NULL)
    file_seek (file, position);

}

static unsigned
sys_tell (int fd)
{
  struct file *file = lookup_fd (fd);
  if (file != NULL)
    return file_tell (file);
  return -1;
}

static void
sys_close (int fd)
{
  struct thread *cur = thread_current ();
  struct list_elem *e;
  for (e = list_begin (&cur->open_files); e != list_end (&cur->open_files);
	 e = list_next (e))
    {
	struct fd_file *fd_file = list_entry (e, struct fd_file, file_elem);
	if (fd_file->fd == fd)
          {
	     list_remove (e);
	     file_close (fd_file->file);
	     free (fd_file);
	     break;
	  }
//	break;
    }
}

