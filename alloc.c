#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

#define MAX_KM_POOLS 32
#define MAX_KM_SIZE PGSIZE / 2

struct kmblock {
  struct kmblock *next;
};

struct kmpage {
  struct kmpage *next;     
  struct kmblock *free;    
  int blocksize;          
  int nblocks;             
  int nfree;               
};

static int pools;

static struct {
  struct spinlock lock;
  struct kmpage *pages;
  int blocksize;
} kmem[MAX_KM_POOLS];

static int sizes[MAX_KM_POOLS];

void
kminit(void)
{
  int sz;

  for (sz = 8, pools = 0; sz <= MAX_KM_SIZE; sz *= 2, pools++) {
    kmem[pools].blocksize = sizes[pools] = sz;
    kmem[pools].pages = 0;

    initlock(&kmem[pools].lock, "kmem");
  }
}

static int
choose_pool(int size)
{
  for(int i = 0; i < pools; i++) {
    if(size <= sizes[i])
      return i;
  }
  return -1;
}

static struct kmpage*
new_page(int blocksize)
{
  char *page = kalloc();
  if(page == 0)
    return 0;

  struct kmpage *hdr = (struct kmpage*)page;
  hdr->next = 0;
  hdr->blocksize = blocksize;
  hdr->free = 0;

  int offset = sizeof(struct kmpage);
  int nblocks = (PGSIZE - offset) / blocksize;
  hdr->nblocks = nblocks;
  hdr->nfree = nblocks;

  char *p = page + offset;
  
  for(int i = 0; i < nblocks; i++) {
    struct kmblock *b = (struct kmblock*)(p + i * blocksize);
    b->next = hdr->free;
    hdr->free = b;
  }

  return hdr;
}

void*
kmalloc(int size)
{
  if(size <= 0)
    return 0;

  if(size <= MAX_KM_SIZE) {
    int idx = choose_pool(size);
    if(idx < 0)
        return 0;

    acquire(&kmem[idx].lock);

    struct kmpage *pg = kmem[idx].pages;
    while(pg && pg->free == 0)
        pg = pg->next;

    if(pg == 0) {
        struct kmpage *newpg = new_page(kmem[idx].blocksize);
        if(newpg == 0) {
        release(&kmem[idx].lock);
        return 0;
        }

        newpg->next = kmem[idx].pages;
        kmem[idx].pages = newpg;
        pg = newpg;
    }

    struct kmblock *b = pg->free;
    pg->free = b->next;
    pg->nfree--;

    release(&kmem[idx].lock);
    return (void*)b;
  }
  else {
    return 0;
  }
}

void
kmfree(void *ptr)
{
  if(ptr == 0)
    return;

  struct kmpage *hdr = (struct kmpage*)PGROUNDDOWN((uint)ptr);

  if(hdr->blocksize > 0) {
    int idx = choose_pool(hdr->blocksize);
    if(idx < 0)
      return;

    acquire(&kmem[idx].lock);

    struct kmblock *b = (struct kmblock*)ptr;
    b->next = hdr->free;
    hdr->free = b;

    if(++hdr->nfree == hdr->nblocks) {
      struct kmpage **pp = &kmem[idx].pages;

      while(*pp && *pp != hdr)
        pp = &(*pp)->next;

      if(*pp == hdr) {
        *pp = hdr->next;
      }

      kfree((char *)hdr);
    }

    release(&kmem[idx].lock);
  }
}