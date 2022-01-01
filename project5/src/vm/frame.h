#ifndef VM_FRAME_H
#define VM_FRAME_H
#include <stdint.h>
#include "vm/page.h"

void frame_init (size_t);
struct frame_entry *get_frame (void);
struct frame_entry *frame_get_multiple (size_t);
void free_frame (struct frame_entry *);
static struct bitmap *free_frames;
static struct frame_entry *frame_table;
static unsigned clock_ptr, clock_max;
static struct lock frame_lock;
struct frame_entry {
    void *kpage;              
    uint32_t num;             
    struct page *page_occupant;     
};

#endif /* vm/frame.h */
