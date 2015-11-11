#include "vm/swap.h"
#include <bitmap.h>
#include "devices/block.h"

#define SCALE (BLOCK_SECTOR_SIZE / PGSIZE)

static struct bitmap *swap_bitmap;
static struct block *swap_block;


void 
swap_init (void)
{
  lock_init (&swap_lock);
  swap_block = block_get_role (BLOCK_SWAP);
  block_sector_t size = block_size (swap_block);
  swap_bitmap = bitmap_create (size * SCALE);
  printf ("bitmap size %d\n", bitmap_size (swap_bitmap));

}

size_t 
swap_find_free_slot (void)
{
  return bitmap_scan (swap_bitmap, 0, 1, false); 
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


