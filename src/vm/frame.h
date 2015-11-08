#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "vm/spage.h"
#include "threads/thread.h"
#include "threads/synch.h"


struct list frame_list;
struct lock frame_lock;

struct frame_entry
  {
     void *paddr;
     bool pinned;
     struct list_elem elem;
     struct thread *t;
     struct spage_entry *spte;
  };

void frame_init (void);
struct frame_entry *frame_get_frame (bool zero);
struct frame_entry *frame_get_frame_with_pin (bool zero, bool pin);
void frame_free_frame (void *paddr);
void frame_unpin_frame (void *paddr);

#endif