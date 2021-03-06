#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "vm/spage.h"
#include "threads/thread.h"


struct list frame_list;

struct frame_entry
  {
     void *paddr;
     bool pinned;
     struct list_elem elem;
     struct thread *t;
     struct spage_entry *spte;
  };

void frame_init (void);
struct frame_entry *frame_get_frame_pinned (bool zero);
void frame_free_frame (void *paddr);
void frame_unpin_frame (void *paddr);
bool frame_check_dirty (void *uaddr, void *paddr);
void frame_clean_dirty (void *uaddr, void *paddr);

bool frame_exist_and_free (struct spage_entry *spte);
bool frame_exist_and_pin (struct spage_entry *spte);

static bool frame_thread_check_access (struct thread *t, void *uaddr, void *paddr);
static void frame_thread_clean_access (struct thread *t, void *uaddr, void *paddr);
static bool frame_thread_check_dirty (struct thread *t, void *uaddr, void *paddr);
#endif
