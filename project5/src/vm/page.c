#include "vm/page.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include <stdio.h>
#include <stdint.h>
#include "vm/swap.h"


unsigned page_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  const struct page *p = hash_entry (p_, struct page, hash_elem);
  return hash_bytes (&p->addr, sizeof p->addr);
}

bool page_less (const struct hash_elem *a_, const struct hash_elem *b_,void *aux UNUSED)
{
  const struct page *a = hash_entry (a_, struct page, hash_elem);
  const struct page *b = hash_entry (b_, struct page, hash_elem);

  return pg_no(a->addr) < pg_no(b->addr);
}
struct page *page_lookup (void *address)
{
  struct thread *t = thread_current ();
  struct page p;
  struct hash_elem *e;

  p.addr = (void *) (pg_no(address) << PGBITS);
  e = hash_find (&t->sup_pages, &p.hash_elem);
  return e != NULL ? hash_entry (e, struct page, hash_elem) : NULL;
}

void page_destructor (struct hash_elem *e, void *aux UNUSED)
{
  struct page *p = hash_entry (e, struct page, hash_elem);
  if (p->frame != NULL) {
    struct frame_entry *f = p->frame;
    p->frame = NULL;
    free_frame (f);
  }
  if (p->block_sector != -1)
    swap_free (p);
  free (p);
}

