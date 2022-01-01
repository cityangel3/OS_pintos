#include <debug.h>
#include <string.h>
#include "filesys/cache.h"
#include "filesys/filesys.h"
#include "threads/synch.h"
struct lock bc_lock;
void buffer_cache_init (void)
{
  lock_init (&bc_lock);
  for (int i = 0; i < NUM_CACHE;i++)
    cache[i].valid_bit = false;
}
struct buffer_cache_entry* buffer_cache_select_victim (void)
{
  if(lock_held_by_current_thread(&bc_lock) == false)
	  exit(-1);
  struct buffer_cache_entry *ent;
  int cindex = 0;
  for(;;) {
    if (cache[cindex].valid_bit == false)
      return &(cache[cindex]);
    if (cache[cindex].refer_bit == true)
      cache[cindex].refer_bit = false;
    else break;
    cindex = (cindex + 1)% NUM_CACHE;
  }
  ent = &cache[cindex];
  if (ent->dirty_bit == true)
    buffer_cache_flush_entry (ent);
  ent->valid_bit = false;
  return ent;
}
void buffer_cache_flush_entry (struct buffer_cache_entry *ent)
{
  if(lock_held_by_current_thread(&bc_lock) == false)
	  exit(-1);
  if(!(ent != NULL && ent->valid_bit == true))
	  exit(-1);
  if (ent->dirty_bit) {
    block_write (fs_device, ent->disk_sector, ent->buffer);
    ent->dirty_bit = false;
  }
}
void buffer_cache_terminate()
{
  lock_acquire (&bc_lock);
  for (int i = 0; i < NUM_CACHE;i++)
  {
    if (cache[i].valid_bit == true) 
    	buffer_cache_flush_entry(&(cache[i]));
  }
  lock_release (&bc_lock);
}
void buffer_cache_read (block_sector_t sector, void *target)
{
  lock_acquire (&bc_lock);
  struct buffer_cache_entry *ent = buffer_cache_lookup (sector);
  if (ent == NULL) {
    ent = buffer_cache_select_victim ();
    ASSERT (ent != NULL && ent->valid_bit == false);
    ent->dirty_bit = false;
    ent->valid_bit = true;
    ent->disk_sector = sector;
    block_read (fs_device, sector, ent->buffer);
  }
  ent->refer_bit = true;
  memcpy (target, ent->buffer, BLOCK_SECTOR_SIZE);
  lock_release (&bc_lock);
}
void buffer_cache_write (block_sector_t sector, const void *source)
{
  lock_acquire(&bc_lock);
  struct buffer_cache_entry *ent = buffer_cache_lookup (sector);
  if (ent == NULL) {
    ent = buffer_cache_select_victim ();
    ASSERT (ent != NULL && ent->valid_bit == false);
    ent->dirty_bit = false;
    ent->valid_bit = true;
    ent->disk_sector = sector;
    block_read (fs_device, sector, ent->buffer);
  }
  ent->refer_bit = true;
  ent->dirty_bit = true;
  memcpy (ent->buffer, source, BLOCK_SECTOR_SIZE);
  lock_release (&bc_lock);
}
struct buffer_cache_entry* buffer_cache_lookup (block_sector_t sector)
{
  for (int i = 0; i < NUM_CACHE;i++)
  {
    if (cache[i].valid_bit && cache[i].disk_sector == sector) {
      return &(cache[i]);
    }
  }
  return NULL; // cache miss
}
