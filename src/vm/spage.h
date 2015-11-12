#ifndef VM_SPAGE_H
#define VM_SPAGE_H

#include <hash.h>
#include <debug.h>
#include "vm/frame.h"
#include "filesys/file.h"
#include "filesys/off_t.h"
#include "devices/block.h"
#include <list.h>
#include <stddef.h>

enum page_type
{
   TYPE_STACK,
   TYPE_LOADING,
   TYPE_LOADING_WRITABLE,
   TYPE_ZERO,
   TYPE_LOADED_WRITABLE,
   TYPE_FILE
};

struct spage_entry
{
   void *uaddr;
   struct frame_entry *fte;
   enum page_type type;
   size_t swap_sector;
   struct file *file;
   off_t ofs;
   uint32_t length;
   struct hash_elem elem;
   struct list_elem list_elem;  
};

void spage_init (struct hash *spage_hash, struct lock *lock);
struct spage_entry *spage_lookup (struct hash *spage_hash, void *uaddr);
hash_action_func print_spte;

#endif
