#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "filesys/cache.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t d_indir_block;
    block_sector_t indir_block;
    block_sector_t dir_blocks[DIRECT];
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    bool is_dir;
  };

struct indir_inode {
  block_sector_t block[INDIRECT];
};

/* In-memory inode. */
struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
  };

bool inode_reserve (struct inode_disk *page, int len);
bool inode_delete (struct inode *id);
bool inode_new (struct inode_disk *page);
block_sector_t sector_number(struct inode_disk *idisk, off_t index);
/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);
  if (pos < inode->data.length) {
    return sector_number(&inode->data, pos/BLOCK_SECTOR_SIZE);
  }
  else
    return -1;
}
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}
bool reserve_indir (block_sector_t* page, size_t num, int level){
  static char free[BLOCK_SECTOR_SIZE];
  struct indir_inode indir_block;
  int chunk,max,nmax; 
  bool res = true;
  if(level > 2)
	  exit(-1);
  if (level == 0) {
    if (*page == 0) {
      if(! free_map_allocate (1, page))
        return res == false;
      buffer_cache_write (*page,free);
    }
    return res;
  }
  if(level > 1)
	  chunk = INDIRECT;
  else
	  chunk = 1;
  if(*page == 0) {
    free_map_allocate (1, page);
    buffer_cache_write (*page, free);
  }
  buffer_cache_read(*page, &indir_block);
  max = DIV_ROUND_UP (num, chunk);
  for (int i = 0; i < max; i++) {
    nmax = num < chunk ? num : chunk;
    if(!reserve_indir(&indir_block.block[i], nmax, level - 1))
      return res == false;
    num -= nmax;
  }
  if(num != 0)
	exit(-1);
  buffer_cache_write (*page, &indir_block);
  return res;
}

bool inode_reserve (struct inode_disk *page, int len)
{
  static char free[BLOCK_SECTOR_SIZE];
  if (len < 0) 
	return false;
  int num_sec=bytes_to_sectors(len);
  int max;
  bool res = true;
  // direct blocks
  max = num_sec < DIRECT ? num_sec: DIRECT;
  for (int i = 0; i < max;i++) {
    if (page->dir_blocks[i] == 0) { 
      if(! free_map_allocate(1, &page->dir_blocks[i]))
        return res == false;
      buffer_cache_write (page->dir_blocks[i],free);
    }
  }
  num_sec -= max;
  if(num_sec == 0) 
	  return res;
  //indirect block
  max = num_sec < INDIRECT ? num_sec : INDIRECT;
  if(!reserve_indir(&page->indir_block,max, 1))
    return res == false;
  num_sec -= max;
  if(num_sec == 0) 
	  return res;
  //double indirect block
  max = num_sec <  INDIRECT * INDIRECT ? num_sec : INDIRECT*INDIRECT;
  if(!reserve_indir(&page->d_indir_block,max, 2))
    return res ==false;
  num_sec -= max;
  if(num_sec == 0) 
	  return res;
  if(num_sec != 0)
	  exit(-1);
  return res == false;
}

void delete_indir(block_sector_t ent, size_t num_sec, int level)
{
  int chunk;
  if(level > 2)
	  exit(-1);
  else if(level == 0) {
    free_map_release(ent, 1);
    return;
  }
  else if(level == 1)
	  chunk = 1;
  else
	  chunk = INDIRECT;
  int nmax,max = DIV_ROUND_UP (num_sec, chunk);
  struct indir_inode indir_block;
  buffer_cache_read(ent, &indir_block);
  for (int i = 0; i < max;i++) {
    nmax = num_sec < chunk ? num_sec : chunk;
    delete_indir(indir_block.block[i],nmax, level - 1);
    num_sec -= nmax;
  }
  if(num_sec != 0)
	  exit(-1);
  free_map_release (ent, 1);
}

bool inode_delete(struct inode *id)
{
  bool res = true;
  if(id->data.length < 0) 
	  return res == false;
  int num_sec = bytes_to_sectors(id->data.length), max;
  max = num_sec < DIRECT ? num_sec: DIRECT;
  for (int i = 0; i < max;i++) {
    free_map_release (id->data.dir_blocks[i], 1);
  }
  num_sec -= max;
  max = num_sec <  INDIRECT ? num_sec : INDIRECT;
  if(max > 0) {
    delete_indir(id->data.indir_block,max, 1);
    num_sec -= max;
  }
  max = num_sec < INDIRECT * INDIRECT ? num_sec : INDIRECT*INDIRECT;
  if(max > 0) {
    delete_indir(id->data.d_indir_block,max, 2);
    num_sec -= max;
  }
  if(num_sec != 0)
	  exit(-1);
  return res;
}

block_sector_t sector_number (struct inode_disk *idisk, off_t index)
{
  struct indir_inode *indir_idisk;
  int ibase = 0, imax = 0;  
  block_sector_t res;
  imax = imax + DIRECT;
  if (index < imax) 
    return idisk->dir_blocks[index];
  ibase = imax;
  imax = imax + INDIRECT;
  if (index < imax) {
    indir_idisk = calloc(1, sizeof(struct indir_inode));
    buffer_cache_read(idisk->indir_block, indir_idisk);
    res = indir_idisk->block[(index - ibase)];
    free(indir_idisk);
    return res;
  }
  ibase = imax;
  imax = imax + INDIRECT * INDIRECT;
  if (index < imax) {
    indir_idisk = calloc(1, sizeof(struct indir_inode));
    buffer_cache_read(idisk->d_indir_block, indir_idisk);
    buffer_cache_read(indir_idisk->block[(index-ibase)/INDIRECT], indir_idisk);
    res = indir_idisk->block[(index-ibase)%INDIRECT];
    free(indir_idisk);
    return res;
  }
  return NULL;
}
/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (block_sector_t sector, off_t length, bool is_dir)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      disk_inode->is_dir = is_dir;
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      if (inode_new(disk_inode))
        {
          buffer_cache_write (sector, disk_inode);
          success = true;
        }
      free (disk_inode);
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  buffer_cache_read (inode->sector, &inode->data);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          free_map_release (inode->sector, 1);
          inode_delete(inode);
        }

      free (inode);
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          buffer_cache_read (sector_idx, buffer + bytes_read);
        }
      else
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL)
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          buffer_cache_read (sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode).
   */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;
  bool success;
  int len;
  if (inode->deny_write_cnt)
    return 0;
  len = offset+size;
  if(byte_to_sector(inode, len-1)==-1u){
    if((inode_reserve(& inode->data,len)) == false)
    	return 0;  
    inode->data.length = len;
    buffer_cache_write(inode->sector, &inode->data);
  }

  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == BLOCK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          buffer_cache_write (sector_idx, buffer + bytes_written);
        }
      else
        {
          /* We need a bounce buffer. */
          if (bounce == NULL)
            {
              bounce = malloc (BLOCK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left)
            buffer_cache_read (sector_idx, bounce);
          else
            memset (bounce, 0, BLOCK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          buffer_cache_write (sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

/* Returns whether the file is directory or not. */
bool
is_inode_dir(const struct inode *inode)
{
  return inode->data.is_dir;
}

/* Returns whether the file is removed or not. */
bool
is_inode_rm(const struct inode *inode)
{
  return inode->removed;
}

bool inode_new(struct inode_disk *page)
{
  return inode_reserve(page,page->length);
}


