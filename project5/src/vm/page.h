#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "threads/palloc.h"
#include "filesys/file.h"
#include "threads/synch.h"
#include "vm/frame.h"
#include <stdint.h>
#include <hash.h>
/* I'll use hash tables (we can choose among array,list,bitmap,and hash table)
 * cause it is efficient for wide range of table sizes*/
unsigned page_hash (const struct hash_elem *, void *);
bool page_less (const struct hash_elem *, const struct hash_elem *, void *);
void page_destructor (struct hash_elem *, void *);
void sup_page_init (void);
struct page *page_lookup (void *);
bool page_do_not_remove (void *);
bool page_allow_remove (void *);
enum page_status
  {
    ALL_ZERO,         
    IN_FRAME_TABLE,   
    IN_SWAP_TABLE,
    IN_FILESYS
  };
struct page
  {
    struct hash_elem hash_elem; 
    void *addr;                 
    off_t offset;               
    uint32_t *pagedir;          
    struct frame_entry *frame;  
    size_t read_bytes;         
    int block_sector;      
    enum page_status status;    
    bool is_stack_page;         
    bool writable;              
    struct file *file;          
  };


#endif /* vm/page.h */
