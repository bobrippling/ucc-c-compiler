#include "stdlib.h"
#include "sys/mman.h"
#include "unistd.h"

/* getpagesize() */
#define PAGE_SIZE 4096
#define MALLOC_CHUNK_SIZE PAGE_SIZE

// sbrk
//#define MALLOC_MMAP
#define MALLOC_SBRK
#define K_AND_R_MALLOC


static void *malloc_chunk_mmap(void)
{
	void *p;

	p = mmap(NULL, MALLOC_CHUNK_SIZE,
			PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if(p == MAP_FAILED)
		return NULL;

	return p;
}

#ifndef __DARWIN__
static void *malloc_chunk_sbrk(void)
{
	void *p = sbrk(MALLOC_CHUNK_SIZE);
	if(p == (void *)-1)
		return NULL;
	return p;
}
#endif

static void *malloc_chunk_static(void)
{
	static char malloc_buf[MALLOC_CHUNK_SIZE];
	static int used;
	if(used)
		return NULL;
	return malloc_buf;
}

static void *malloc_chunk(void)
{
#ifdef MALLOC_MMAP
	return malloc_chunk_mmap();
#else
#  ifdef MALLOC_SBRK
	return malloc_chunk_sbrk();
#  else
	return malloc_chunk_static();
#  endif
#endif
}

#ifdef K_AND_R_MALLOC
#define NALLOC 1024

#ifdef __GOT_SHORT_LONG
typedef long Align;    /* for alignment to long boundary */
#else
typedef int  Align;    /* for alignment to long boundary */
#endif

union header /* block header */
{
	struct
	{
		union header *ptr; /* next block if on free list */
		unsigned size;     /* size of this block */
	} s;
	Align x;           /* force alignment of blocks */
};

typedef union header Header;

static Header base;           /* empty list to get started */
static Header *freep = NULL;  /* start of free list */

/* morecore:  ask system for more memory */
static Header *morecore(unsigned nu)
{
	char *cp, *sbrk(int);
	Header *up;

	if (nu < NALLOC)
		nu = NALLOC;
	cp = sbrk(nu * sizeof(Header));
	if (cp == (char *) -1)   /* no space at all */
		return NULL;
	up = (Header *) cp;
	up->s.size = nu;
	free((void *)(up+1));
	return freep;
}

void *malloc(unsigned nbytes)
{
	Header *p, *prevp;
	Header *moreroce(unsigned);
	unsigned nunits;

	nunits = (nbytes+sizeof(Header) - 1) / sizeof(Header) + 1;
	if ((prevp = freep) == NULL) {   /* no free list yet */
		base.s.ptr = freep = prevp = &base;
		base.s.size = 0;
	}
	for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {
		if (p->s.size >= nunits) {  /* big enough */
			if (p->s.size == nunits)  /* exactly */
				prevp->s.ptr = p->s.ptr;
			else {              /* allocate tail end */
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}
			freep = prevp;
			return (void *)(p+1);
		}
		if (p == freep)  /* wrapped around free list */
			if ((p = morecore(nunits)) == NULL)
				return NULL;    /* none left */
	}
}

void free(void *ap)
{
	Header *bp, *p;

	bp = (Header *)ap - 1;    /* point to  block header */
	for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
		if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
			break;  /* freed block at start or end of arena */

	if (bp + bp->s.size == p->s.ptr) {    /* join to upper nbr */
		bp->s.size += p->s.ptr->s.size;
		bp->s.ptr = p->s.ptr->s.ptr;
	} else
		bp->s.ptr = p->s.ptr;
	if (p + p->s.size == bp) {            /* join to lower nbr */
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	} else
		p->s.ptr = bp;
	freep = p;
}
#else
typedef struct malloc_header malloc_header;

struct malloc_header
{
	malloc_header *next;
	size_t sz;
};

static malloc_header *free_list;

void *malloc(size_t sz)
{
	malloc_header *it, *prev;
	void *ret = NULL;

	if(!free_list){
		free_list = malloc_chunk();
		if(!free_list)
			return NULL;
		free_list->next = free_list;
		free_list->sz = MALLOC_CHUNK_SIZE;
	}

	for(prev = free_list; prev->next > free_list; prev = prev->next);

	for(it = free_list; it->next > it; it = it->next){
		if(it->sz > sz){
			size_t new = it->sz - sz;

			ret = it + sizeof(malloc_header);

			it->sz = sz;
			it += sz;

			it->sz = new;
			prev->next = it;

			break;
		}
	}

	return ret; /* TODO: grab more memory */
}

void free(void *free_this)
{
	/* TODO */
}
#endif
