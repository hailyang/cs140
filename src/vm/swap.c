#include "vm/swap.h"
#include <bitmap.h>
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <stdio.h>
#include <stdbool.h>
#define SCALE (PGSIZE / BLOCK_SECTOR_SIZE)

static struct bitmap *swap_bitmap;
static struct block *swap_block;


void 
swap_init (void)
{
  lock_init (&swap_lock);
  lock_init (&swap_disk_lock);

  swap_block = block_get_role (BLOCK_SWAP);
printf("swap block get\n");
  if (swap_block == NULL)
    PANIC ("No swap device found. cannot initialize file system\n");
  block_sector_t size = block_size (swap_block);
  swap_bitmap = bitmap_create (size / SCALE);
  printf ("right bitmap size is %d, block sector size is %d, SCALE is %d\n", size/SCALE, size, SCALE);
  printf ("bitmap size %d\n", bitmap_size (swap_bitmap));
  bitmap_set (swap_bitmap, 0, true);
  printf ("bit set\n");
}

size_t 
swap_find_free_slot (void)
{
  return bitmap_scan_and_flip (swap_bitmap, 0, 1, false); 
}

void 
swap_set_slot (size_t idx)
{
  bitmap_mark (swap_bitmap, idx);
}

void 
swap_reset_slot (size_t idx) 
{
  bitmap_reset (swap_bitmap, idx);
}

void 
swap_read_slot (size_t idx, void *buffer)
{
  int i;
  for (i = 0; i < SCALE; i++)
  {
    block_read (swap_block, SCALE * idx + i, buffer);
    buffer = (void *)((unsigned)buffer + BLOCK_SECTOR_SIZE);
  }
}

void 
swap_write_slot (size_t idx, void *buffer)
{
  int i;
  for (i = 0; i < SCALE; i++)
  {
    block_write (swap_block, SCALE * idx + i, buffer);
    buffer = (void *)((unsigned) buffer + BLOCK_SECTOR_SIZE);
  }
}
