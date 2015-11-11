#include <list.h>
#include <stdint.h>
#include <stdio.h>
#include "vm/frame.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "threads/malloc.h"

void
frame_init (void)
{
  list_init (&frame_list);
  lock_init (&frame_lock);
}

struct frame_entry *
frame_get_frame (bool zero)
{
  void *addr = NULL;
  if (zero)
    addr = palloc_get_page (PAL_USER | PAL_ZERO | PAL_ASSERT);
  else
    addr = palloc_get_page (PAL_USER | PAL_ASSERT);

  if (addr == NULL)
    return NULL;
  struct frame_entry *fe = (struct frame_entry*)
                        malloc (sizeof (struct frame_entry));
     // kernel should not run out of memory
  ASSERT (fe != NULL);

  fe->paddr = addr;
  fe->pinned = true;
  fe->spte = NULL;
  list_push_back (&frame_list, &fe->elem);
  return fe;
}

struct frame_entry *
frame_get_frame_pinned (bool zero) {
  void *addr = NULL;
  if (zero)
    addr = palloc_get_page (PAL_USER | PAL_ZERO | PAL_ASSERT);
  else
    addr = palloc_get_page (PAL_USER | PAL_ASSERT);

  struct frame_entry *fe = (struct frame_entry*)
                        malloc (sizeof (struct frame_entry));
     // kernel should not run out of memory
  ASSERT (fe != NULL);

  fe->paddr = addr;
  fe->pinned = false;
  fe->spte = NULL;
  list_push_back (&frame_list, &fe->elem);
  return fe;

}
void 
frame_free_frame (void *paddr)
{
  struct list_elem *e;
  ASSERT (pg_ofs (paddr) == 0);
  for (e = list_begin (&frame_list); e != list_end (&frame_list);
	e = list_next (e))
  {
     struct frame_entry *fe = list_entry (e, struct frame_entry, elem);
     if (fe->paddr == paddr)
     {
	palloc_free_page (paddr);
	list_remove (e);
	free (fe);
	return;
     }
  }
}

void 
frame_unpin_frame (void *paddr) 
{
  struct list_elem *e;
  ASSERT (pg_ofs (paddr) == 0);
  for (e = list_begin (&frame_list); e != list_end (&frame_list);
		e = list_next (e))
  {
      struct frame_entry *fte = 
		list_entry (e, struct frame_entry, elem);
      if (fte->paddr == paddr)
      {
	fte->pinned = false;
	break;
       }
  }
}

bool 
frame_check_dirty (void *uaddr, void *paddr)
{
  struct thread *t = thread_current();
  bool u_dirty = pagedir_is_dirty (t->pagedir, uaddr);
  bool p_dirty = pagedir_is_dirty (t->pagedir, paddr);
  return u_dirty || p_dirty;
}

void 
frame_clean_dirty (void *uaddr, void *paddr) 
{
  struct thread *t = thread_current();
  pagedir_set_dirty (t->pagedir, uaddr);
  pagedir_set_dirty (t->pagedir, paddr);
}

bool 
frame_exist_and_free (struct spage_entry *s)
{
  bool exist = false;
  if (s->fte != NULL)
  {
    exist = true;
    palloc_free_page (s->fte->paddr);
    list_remove (&(s->fte->elem));
    free (s->fte);
    s->fte = NULL;
  }
  return exist;
}

bool 
frame_exist_and_pin (struct spage_entry *s)
{
  bool exist = false;
  if (s->fte != NULL)
  {
    exist = true;
    s->fte->pinned = true;
  }
  return exist;
}
