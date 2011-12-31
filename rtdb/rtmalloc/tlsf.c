// modified version of tlsf with address independent offsets by matthias goebl

/* 
 * Two Levels Segregate Fit memory allocator (TLSF)
 * Version 2.2.0
 *
 * Written by Miguel Masmano Tello <mimastel@doctor.upv.es>
 *
 * Thanks to Ismael Ripoll for his suggestions and reviews
 *
 * Copyright (C) 2006, 2005, 2004
 *
 * This code is released using a dual license strategy: GPL/LGPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of the GNU General Public License Version 2.0
 * Released under the terms of the GNU Lesser General Public License Version 2.1
 *
 */
#define _IA32_ 1

#ifdef TLSF_USE_ADDRESS_INDEPENDET_OFFSETS
#define ADDOFF(addr,off) ((bhdr_t*)((void*)(addr)+(unsigned int)(off)))
#define DELOFF(addr,off) ((void*)(addr)-(unsigned int)(off))
#else
// no additional computation effort when not using address-independentness
#define ADDOFF(addr,off) (addr)
#define DELOFF(addr,off) (addr)
#endif

#include "tlsf.h"
#include "tlsf-arch_dep.h"

#warning TLSF uses architecture dependent bit-operations, so please ignore the next 2 warnings (and use the suba-allocator for non-x86 architectures)
#ifndef DONT_INCLUDE_BITOPS
#include <asm/bitops.h>
#define __clear_bit clear_bit
#else
static __inline__ void __set_bit(int nr, volatile void * addr)
{
	__asm__(
		"btsl %1,%0"
		:"=m" (addr)
		:"Ir" (nr));
}
static __inline__ void __clear_bit(int nr, volatile void * addr)
{
	__asm__( 
		"btrl %1,%0"
		:"=m" (addr)
		:"Ir" (nr));
}
#endif
#include <string.h>


///////////////////////////////////////////////////////////////////////////
// Definition of the structures used by TLSF


#define FLI_OFFSET 6		// tlsf structure just will manage blocks bigger
		     // than 128 bytes
#define SMALL_BLOCK 128
#define REAL_FLI (MAX_FLI - FLI_OFFSET)
#define MIN_BLOCK_SIZE 12	// the size of free_ptr_t + 4
#define TLSF_SIGNATURE 0x2A59FA59

typedef struct free_ptr_struct {
    struct bhdr_struct *prev;
    struct bhdr_struct *next;
} free_ptr_t;

#define BLOCK_SIZE 0xFFFFFFFC
#define BLOCK_INF 0x3
#define BLOCK_STATE 0x1
#define PREV_STATE 0x2

// bit 0 of the block size
#define FREE_BLOCK 0x1
#define USED_BLOCK 0

// bit 1 of the block size
#define PREV_FREE 0x2
#define PREV_USED 0

#define BHDR_OVERHEAD 4		// just four bytes

typedef struct bhdr_struct {
    // This pointer is just valid if the first bit of size is set
    struct bhdr_struct *prev_hdr;
    // The size is stored in bytes
    u32_t size;			// bit 0 indicates whether the block is used and
    // bit 1 allows to know whether the previous block is free
    union {
	struct free_ptr_struct free_ptr;
	u8_t buffer[sizeof(struct free_ptr_struct)];
    } ptr;
} bhdr_t;

typedef struct TLSF_struct {
    // the TLSF's structure signature
    u32_t tlsf_signature;


    // the first-level bitmap
    // This array should have a size of REAL_FLI bits
    u32_t fl_bitmap;

    // the second-level bitmap
    u32_t sl_bitmap[REAL_FLI];

    bhdr_t *matrix[REAL_FLI][MAX_SLI];
} tlsf_t;

/////////////////////////////////////////////////////////////////

#define GET_NEXT_BLOCK(_addr, _r) \
  ((bhdr_t *) ((unsigned long) _addr + (_r) - 4))

#define ROUNDUP_SIZE(_r) ((_r + 0x3) & ~0x3)
#define ROUNDDOWN_SIZE(_r) ((_r) & ~0x3)

static inline void MAPPING_SEARCH(size_t * _r, int *_fl, int *_sl)
{
    int _t;

    if (*_r < SMALL_BLOCK) {
	*_fl = 0;
	*_sl = *_r / (SMALL_BLOCK / MAX_SLI);
    } else {
	_t = (1 << (_fls(*_r) - MAX_LOG2_SLI)) - 1;
	*_r = *_r + _t;
	*_fl = _fls(*_r);
	*_sl = (*_r >> (*_fl - MAX_LOG2_SLI)) - MAX_SLI;
	*_fl -= FLI_OFFSET;
	/*if ((*_fl -= FLI_OFFSET) < 0) // FL wil be always >0!
	 *_fl = *_sl = 0;
	 */
	*_r &= ~_t;
    }
}

static inline void MAPPING_INSERT(size_t _r, int *_fl, int *_sl)
{
    if (_r < SMALL_BLOCK) {
	*_fl = 0;
	*_sl = _r / (SMALL_BLOCK / MAX_SLI);
    } else {
	*_fl = _fls(_r);
	*_sl = (_r >> (*_fl - MAX_LOG2_SLI)) - MAX_SLI;
	*_fl -= FLI_OFFSET;
    }
}


static inline bhdr_t *FIND_SUITABLE_BLOCK(tlsf_t * _tlsf, int *_fl,
					  int *_sl)
{
    u32_t _tmp = _tlsf->sl_bitmap[*_fl] & (~0 << *_sl);
    bhdr_t *_b = NULL;

    if (_tmp) {
	*_sl = _ffs(_tmp);
	_b = ADDOFF( _tlsf->matrix[*_fl][*_sl], _tlsf);
    } else {
	*_fl = _ffs(_tlsf->fl_bitmap & (~0 << (*_fl + 1)));
	if (*_fl > 0) {		// likely
	    *_sl = _ffs(_tlsf->sl_bitmap[*_fl]);
	    _b = ADDOFF( _tlsf->matrix[*_fl][*_sl], _tlsf);
	}
    }
    return _b;
}


#define EXTRACT_BLOCK_HDR(_b, _tlsf, _fl, _sl) { \
  _tlsf -> matrix [_fl] [_sl] = _b -> ptr.free_ptr.next; \
  if (_tlsf -> matrix[_fl][_sl]) \
    ADDOFF( _tlsf -> matrix[_fl][_sl], _tlsf) -> ptr.free_ptr.prev = NULL; \
  else {\
    __clear_bit (_sl, &_tlsf -> sl_bitmap [_fl]); \
    if (!_tlsf -> sl_bitmap [_fl]) \
      __clear_bit (_fl, &_tlsf -> fl_bitmap); \
  } \
  _b -> ptr.free_ptr = (free_ptr_t) {NULL, NULL}; \
}


#define EXTRACT_BLOCK(_b, _tlsf, _fl, _sl) { \
  if (_b -> ptr.free_ptr.next) \
    ADDOFF( _b -> ptr.free_ptr.next, _tlsf) -> ptr.free_ptr.prev = _b -> ptr.free_ptr.prev; \
  if (_b -> ptr.free_ptr.prev) \
    ADDOFF( _b -> ptr.free_ptr.prev, _tlsf) -> ptr.free_ptr.next = _b -> ptr.free_ptr.next; \
  if (_tlsf -> matrix [_fl][_sl] == DELOFF( _b, _tlsf)) {  \
    _tlsf -> matrix [_fl][_sl] = _b -> ptr.free_ptr.next; \
    if (!_tlsf -> matrix [_fl][_sl]) {  \
      __clear_bit (_sl, &_tlsf -> sl_bitmap[_fl]); \
      if (!_tlsf -> sl_bitmap [_fl]) \
      __clear_bit (_fl, &_tlsf -> fl_bitmap); \
    } \
  } \
  _b -> ptr.free_ptr = (free_ptr_t) {NULL, NULL}; \
}

#define INSERT_BLOCK(_b, _tlsf, _fl, _sl) { \
  _b -> ptr.free_ptr = (free_ptr_t) {NULL, _tlsf -> matrix [_fl][_sl]}; \
  if (_tlsf -> matrix [_fl][_sl]) \
    ADDOFF( _tlsf -> matrix [_fl][_sl], _tlsf) -> ptr.free_ptr.prev = DELOFF( _b, _tlsf); \
  _tlsf -> matrix [_fl][_sl] = DELOFF( _b, _tlsf); \
  __set_bit (_sl, &_tlsf -> sl_bitmap [_fl]); \
  __set_bit (_fl, &_tlsf -> fl_bitmap); \
}

/////////////////////////////////////////////////////////////////////7

static char *mp;

int init_memory_pool(size_t mem_pool_size, void *mem_pool)
{
    tlsf_t *tlsf;
    bhdr_t *b, *lb;
    int fl, sl;

    if (!mem_pool || !mem_pool_size
	|| mem_pool_size < sizeof(tlsf_t) + 128) {
	ERROR_MSG("init_memory_pool (): memory_pool invalid\n");
	return -1;
    }

    if (((unsigned long) mem_pool & 0x3)) {
	ERROR_MSG
	    ("init_memory_pool (): mem_pool must be aligned to a word\n");
	return -1;
    }
    // Zeroing the memory pool
    memset((char *) mem_pool, 0x0, mem_pool_size);
    tlsf = (tlsf_t *) mem_pool;
    tlsf->tlsf_signature = TLSF_SIGNATURE;
    mp = mem_pool;
    b = GET_NEXT_BLOCK(mem_pool, sizeof(tlsf_t));
    b->size = ROUNDDOWN_SIZE(mem_pool_size - sizeof(tlsf_t) - 2 *
			     BHDR_OVERHEAD) | FREE_BLOCK | PREV_USED;
    b->ptr.free_ptr.prev = b->ptr.free_ptr.next = 0;

    if (b->size > (1 << MAX_FLI)) {
	ERROR_MSG
	    ("init_memory_pool (): TLSF can't store a block of %lu bytes.\n",
	     b->size & BLOCK_SIZE);
	return -1;
    }

    MAPPING_INSERT(b->size, &fl, &sl);
    INSERT_BLOCK(b, tlsf, fl, sl);
    // The sentinel block, it allow us to know when we're in the last block
    lb = GET_NEXT_BLOCK(b->ptr.buffer, b->size & BLOCK_SIZE);
    lb->prev_hdr = DELOFF( b, tlsf);
    lb->size = 0 | USED_BLOCK | PREV_FREE;
    return b->size & BLOCK_SIZE;
}

void destroy_memory_pool(void *mem_pool)
{
    tlsf_t *tlsf = (tlsf_t *) mem_pool;

    tlsf->tlsf_signature = 0;
}


void *rtl_malloc(size_t size)
{
    return malloc_ex(size, mp);
}

void rtl_free(void *ptr)
{
    free_ex(ptr, mp);
}



/******************************************************/
void *malloc_ex(size_t size, void *mem_pool)
{
    tlsf_t *tlsf = (tlsf_t *) mem_pool;
    bhdr_t *b, *b2, *next_b;
    int fl, sl, tmp_size;

    size = (size < MIN_BLOCK_SIZE) ? MIN_BLOCK_SIZE : ROUNDUP_SIZE(size);
    // Rounding up the requested size and calculating fl and sl
    MAPPING_SEARCH(&size, &fl, &sl);

    // Searching a free block
    if (!(b = FIND_SUITABLE_BLOCK(tlsf, &fl, &sl)))
	return NULL;		// Not found

    EXTRACT_BLOCK_HDR(b, tlsf, fl, sl);

    //-- found:
    next_b = GET_NEXT_BLOCK(b->ptr.buffer, b->size & BLOCK_SIZE);
    // Should the block be split?
    tmp_size = (b->size & BLOCK_SIZE) - size - BHDR_OVERHEAD;
    if (tmp_size >= MIN_BLOCK_SIZE) {
	b2 = GET_NEXT_BLOCK(b->ptr.buffer, size);
	b2->size = tmp_size | FREE_BLOCK | PREV_USED;
	next_b->prev_hdr = DELOFF( b2, tlsf);

	MAPPING_INSERT(tmp_size, &fl, &sl);
	INSERT_BLOCK(b2, tlsf, fl, sl);

	b->size = size | (b->size & PREV_STATE);
    } else {
	next_b->size &= (~PREV_FREE);
    }

    b->size &= (~FREE_BLOCK);	// Now it's used

    return (void *) b->ptr.buffer;
}

void free_ex(void *ptr, void *mem_pool)
{
    tlsf_t *tlsf = (tlsf_t *) mem_pool;
    bhdr_t *b = (bhdr_t *) ((unsigned long) ptr - 8), *tmp_b;
    int fl = 0, sl = 0;

    b->size |= FREE_BLOCK;
    b->ptr.free_ptr = (free_ptr_t) {
    NULL, NULL};
    tmp_b = GET_NEXT_BLOCK(b->ptr.buffer, b->size & BLOCK_SIZE);
    if (tmp_b->size & FREE_BLOCK) {
	MAPPING_INSERT(tmp_b->size & BLOCK_SIZE, &fl, &sl);
	EXTRACT_BLOCK(tmp_b, tlsf, fl, sl);
	b->size += (tmp_b->size & BLOCK_SIZE) + BHDR_OVERHEAD;
    }
    if (b->size & PREV_FREE) {
	tmp_b = ADDOFF( b->prev_hdr, tlsf);
	MAPPING_INSERT(tmp_b->size & BLOCK_SIZE, &fl, &sl);
	EXTRACT_BLOCK(tmp_b, tlsf, fl, sl);
	tmp_b->size += (b->size & BLOCK_SIZE) + BHDR_OVERHEAD;
	b = tmp_b;
    }
    MAPPING_INSERT(b->size & BLOCK_SIZE, &fl, &sl);
    INSERT_BLOCK(b, tlsf, fl, sl);

    tmp_b = GET_NEXT_BLOCK(b->ptr.buffer, b->size & BLOCK_SIZE);
    tmp_b->size |= PREV_FREE;
    tmp_b->prev_hdr = DELOFF( b, tlsf);
}

void *realloc_ex(void *ptr, size_t new_size, void *mem_pool)
{
    unsigned char *ptr_aux;
    unsigned int cpsize;
    bhdr_t *b;

    if (!ptr) {
	if (new_size)
	    return (void *) malloc_ex(new_size, mem_pool);
	if (!new_size)
	    return NULL;
    } else if (!new_size) {
	free_ex(ptr, mem_pool);
	return NULL;
    }
    // A clever implementation can be done (check whether the next
    // neighbour is free and it is big enough), but it does not worth
    // the effort!
    ptr_aux = (unsigned char *) malloc_ex(new_size, mem_pool);

    b = (bhdr_t *) ((unsigned long) ptr - 8);

    cpsize = ((b->size & BLOCK_SIZE) > new_size) ?
	new_size : (b->size & BLOCK_SIZE);

    memcpy((unsigned char *) ptr_aux, (unsigned char *) ptr, cpsize);

    free_ex(ptr, mem_pool);
    return ((void *) ptr_aux);
}


void *calloc_ex(size_t nelem, size_t elem_size, void *mem_pool)
{
    unsigned char *ptr;

    if (nelem <= 0 || elem_size <= 0)
	return NULL;

    if (!(ptr = (unsigned char *) malloc_ex(nelem * elem_size, mem_pool)))
	return NULL;
    memset(ptr, 0, nelem * elem_size);

    return ((void *) ptr);
}



#ifdef _DEBUG_TLSF_

////////////////  DEBUG FUNCTIONS  

// The following functions have been designed to ease the debugging of
// the TLSF  structure.  For non-developing  purposes, it may  be they
// haven't too much worth.  To enable them, _DEBUG_TLSF_ must be set.

void dump_memory_region(unsigned char *mem_ptr, unsigned int size)
{

    unsigned int begin = (unsigned int) mem_ptr;
    unsigned int end = (unsigned int) mem_ptr + size;
    int column = 0;

    begin >>= 2;
    begin <<= 2;

    end >>= 2;
    end++;
    end <<= 2;

    PRINT_MSG("\nMemory region dumped: 0x%x - 0x%x\n\n", begin, end);

    column = 0;
    PRINT_MSG("0x%x ", begin);

    while (begin < end) {
	if (((unsigned char *) begin)[0] == 0)
	    PRINT_MSG("00");
	else
	    PRINT_MSG("%2x", ((unsigned char *) begin)[0]);
	if (((unsigned char *) begin)[1] == 0)
	    PRINT_MSG("00 ");
	else
	    PRINT_MSG("%2x ", ((unsigned char *) begin)[1]);
	begin += 2;
	column++;
	if (column == 8) {
	    PRINT_MSG("\n0x%x ", begin);
	    column = 0;
	}

    }
    PRINT_MSG("\n\n");
}

void print_block(bhdr_t * b)
{
    if (!b)
	return;
    PRINT_MSG(">> [%p] (", b);
    if ((b->size & BLOCK_SIZE))
	PRINT_MSG("%lu bytes, ", b->size & BLOCK_SIZE);
    else
	PRINT_MSG("sentinel, ");
    if ((b->size & BLOCK_STATE) == FREE_BLOCK)
	PRINT_MSG("free [%p, %p], ", b->ptr.free_ptr.prev,
		  b->ptr.free_ptr.next);
    else
	PRINT_MSG("used, ");
    if ((b->size & PREV_STATE) == PREV_FREE)
	PRINT_MSG("prev. free [%p])\n", b->prev_hdr);
    else
	PRINT_MSG("prev used)\n");
}

void print_tlsf(tlsf_t * tlsf)
{
    bhdr_t *next;
    int i, j;

    PRINT_MSG("\nTLSF at %p\n", tlsf);

    PRINT_MSG("FL bitmap: 0x%x\n\n", (unsigned) tlsf->fl_bitmap);

    for (i = 0; i < REAL_FLI; i++) {
	if (tlsf->sl_bitmap[i])
	    PRINT_MSG("SL bitmap 0x%x\n", (unsigned) tlsf->sl_bitmap[i]);
	for (j = 0; j < MAX_SLI; j++) {
	    next = tlsf->matrix[i][j];
	    if (next)
		PRINT_MSG("-> [%d][%d]\n", i, j);
	    while (next) {
		print_block(ADDOFF( next, tlsf));
		next = ADDOFF( next, tlsf) -> ptr.free_ptr.next;
	    }
	}
    }
}

void print_all_blocks(tlsf_t * tlsf)
{
    bhdr_t *next = (bhdr_t *) ((unsigned long) tlsf + sizeof(tlsf_t) - 4);

    PRINT_MSG("\nTLSF at %p\nALL BLOCKS\n\n", tlsf);
    while (next) {
	print_block(next);
	if ((next->size & BLOCK_SIZE))
	    next =
		GET_NEXT_BLOCK(next->ptr.buffer, next->size & BLOCK_SIZE);
	else
	    next = 0;
    }
}

#endif
