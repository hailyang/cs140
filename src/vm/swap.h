#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "threads/synch.h"

struct lock swap_lock;

void swap_init (void);
void swap_set_slot (size_t idx);
void swap_reset_slot (size_t idx);
//void swap_read_slot (size_t idx, void *buffer);
//void swap_write_slot (size_t idx, void *buffer);
size_t swap_fine_free_slot (void);

#endif
