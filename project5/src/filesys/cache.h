#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H
#include <string.h>
#include "devices/block.h"
#include "filesys/cache.h"
#include "filesys/filesys.h"
#include "threads/synch.h"
struct buffer_cache_entry {
  bool valid_bit;  
  block_sector_t disk_sector;
  uint8_t buffer[BLOCK_SECTOR_SIZE];
  bool dirty_bit;    
  bool refer_bit;    
};
#define NUM_CACHE 64
static struct buffer_cache_entry cache[NUM_CACHE];
void buffer_cache_init();
void buffer_cache_terminate();
void buffer_cache_read (block_sector_t sector,void*cont);
void buffer_cache_write (block_sector_t sector,const void*cont);
struct buffer_cache_entry* buffer_cache_lookup(block_sector_t sector);
struct buffer_cache_entry* buffer_cache_select_victim();
void buffer_cache_flush_entry(struct buffer_cache_entry*ent);
#endif
