#ifndef VM_MMAP_H
#define VM_MMAP_H

#include "filesys/file.h"
#include <list.h>

typedef int mapid_t;
#define MAP_FAILED ((mapid_t) - 1)
#define MAP_MAX 32767

struct mmap_entry
{
  mapid_t mid;
  struct file *file;
  struct list spte_list;
  struct list_elem elem;
};

#endif
