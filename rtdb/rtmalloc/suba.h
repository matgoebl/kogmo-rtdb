// source: http://www.ioplex.com/~miallen/libmba/dl/src/suba.c
// http://www.ioplex.com/~miallen/libmba/dl/docs/ref/suba.html

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>

#define MMSG
#define MMNO
#define MMNF
#define PMSG
#define PMNO
#define PMNF
#define AMSG
#define AMNO
#define AMNF
#define MCLR
#define NULL_POINTER_ERR


#define ALAL(a) ((a) && (a) != stdlib_allocator ? (a) : (global_allocator ? global_allocator : 0))
#define ALREF(a,p) ((ref_t)((p) ? (char *)(p) - (char *)ALAL(a) : 0))
#define ALADR(a,r) ((void *)((r) ? (char *)ALAL(a) + (r) : NULL))

typedef size_t ref_t;  /* suba offset from start of memory to object */

struct allocator;

typedef void *(*alloc_fn)(struct allocator *al, size_t size, int flags);
typedef void *(*realloc_fn)(struct allocator *al, void *obj, size_t size);
typedef int (*free_fn)(void *al, void *obj);
typedef int (*reclaim_fn)(struct allocator *al, void *arg, int attempt);
typedef void *(*new_fn)(void *context, size_t size, int flags);
typedef int (*del_fn)(void *context, void *object);

struct allocator {
	unsigned char magic[8];                /* suba header identifier */
	ref_t tail;                 /* offset to first cell in free list */
	size_t mincell;    /* min cell size must be at least sizeof cell */
	size_t size;                        /* total size of memory area */
	size_t alloc_total;  /* total bytes utilized from this allocator */
	size_t free_total;   /* total bytes released from this allocator */
	size_t size_total;  /* total bytes requested from this allocator */
					/* utilization = size_total / alloc_total * 100
					 * e.g. 50000.0 / 50911.0 * 100.0 = 98.2%
					 */
	size_t max_free;   /* for debugging - any cell larger throws err */
	alloc_fn alloc;
	realloc_fn realloc;
	free_fn free;
						/* for reaping memory from pool, varray, etc */
	reclaim_fn reclaim;
	void *reclaim_arg;
	int reclaim_depth;
	ref_t userref;
};

//struct allocator *global_allocator;
//struct allocator *stdlib_allocator;

void *allocator_alloc(struct allocator *al, size_t size, int flags);
void *allocator_realloc(struct allocator *al, void *obj, size_t size);
int allocator_free(void *al, void *obj);
void allocator_set_reclaim(struct allocator *al, reclaim_fn recl, void *arg);

#define SUBA_PTR_SIZE(ptr) ((ptr) ? (*((size_t *)(ptr) - 1)) : 0)

struct allocator *suba_init(void *mem, size_t size, int rst, size_t mincell);
void *suba_alloc(struct allocator *suba, size_t size, int zero);
void *suba_realloc(struct allocator *suba, void *ptr, size_t size);
int suba_free(void *suba, void *ptr);

void *suba_addr(const struct allocator *suba, const ref_t ref);
ref_t suba_ref(const struct allocator *suba, const void *ptr);
