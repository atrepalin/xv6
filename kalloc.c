// Physical memory allocator (изменённый для kalloc_npages)

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // defined in kernel.ld

struct run {
  struct run *next;
  int npages;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

static void
insert_and_coalesce(struct run *r)
{
  struct run **pp = &kmem.freelist;
  char *rbase = (char*)r;
  uint rsize = r->npages * PGSIZE;

  while (*pp && (char*)(*pp) < rbase) {
    pp = &(*pp)->next;
  }

  if (*pp) {
    struct run *next = *pp;
    char *nextbase = (char*)next;
    if (rbase + rsize == nextbase) {
      r->npages += next->npages;
      r->next = next->next;
      *pp = r;
    } else {
      r->next = next;
      *pp = r;
    }
  } else {
    r->next = 0;
    *pp = r;
  }

  struct run *prev = 0;
  struct run *cur = kmem.freelist;
  while (cur && cur->next && cur->next != r)
    cur = cur->next;
  if (cur && cur->next == r)
    prev = cur;

  if (prev) {
    char *prevbase = (char*)prev;
    uint prevsize = prev->npages * PGSIZE;
    if (prevbase + prevsize == (char*)r) {
      prev->npages += r->npages;
      prev->next = r->next;
    }
  }
}

void
freerange(void *vstart, void *vend)
{
  char *p = (char*)PGROUNDUP((uint)vstart);
  if (p >= (char*)vend) return;

  int np = ((char*)vend - p) / PGSIZE;
  if (np <= 0) return;

  struct run *r = (struct run*)p;
  r->npages = np;

  if (kmem.use_lock)
    acquire(&kmem.lock);
  if (!kmem.freelist) {
    r->next = 0;
    kmem.freelist = r;
  } else {
    insert_and_coalesce(r);
  }
  if (kmem.use_lock)
    release(&kmem.lock);
}

void
kfree_npages(void *v, int n)
{
  struct run *r;

  if((uint)v % PGSIZE || (char *)v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  memset(v, 1, PGSIZE);

  r = (struct run*)v;
  r->npages = n;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  insert_and_coalesce(r);
  if(kmem.use_lock)
    release(&kmem.lock);
}

void 
kfree(void *v) {
  kfree_npages(v, 1);
}

void*
kalloc_npages(int n)
{
  if (n <= 0)
    return 0;

  if(kmem.use_lock)
    acquire(&kmem.lock);

  struct run **pp = &kmem.freelist;
  struct run *r = kmem.freelist;
  while (r) {
    if (r->npages >= n) {
      if (r->npages == n) {
        *pp = r->next;
        if(kmem.use_lock)
          release(&kmem.lock);
        return (void*)r;
      } else {
        int remaining = r->npages - n;
        char *alloc_base = (char*)r + remaining * PGSIZE;
        r->npages = remaining;
        if(kmem.use_lock)
          release(&kmem.lock);
        return (void*)alloc_base;
      }
    }
    pp = &r->next;
    r = r->next;
  }

  if(kmem.use_lock)
    release(&kmem.lock);
  return 0;
}

void*
kalloc(void)
{
  return (char*)kalloc_npages(1);
}
