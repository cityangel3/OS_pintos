#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "vm/page.h"
static struct lock block_lock;
static struct bitmap *used_blocks;
struct block *swap_table;
void swap_init (void);
void swap_get (struct page *);
void swap_free (struct page *);
void swap_insert (struct page *);

#endif /* vm/swap.h */
