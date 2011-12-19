/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */
/*! \file kogmo_rtdb_rtmalloc.c
 * \brief Memory Allocation within the Real-time Vehicle Database
 *
 * Copyright (c) 2005-2006 Matthias Goebl <mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include "kogmo_rtdb_internal.h"


#if defined(RTMALLOC_tlsf)
extern int init_memory_pool (size_t, void *);
extern void destroy_memory_pool (void *);
extern void *malloc_ex (size_t, void *);
extern void free_ex (void *, void *);
extern void *realloc_ex (void *, size_t, void *);
extern void *calloc_ex (size_t, size_t, void *);
#elif defined(RTMALLOC_suba)
void *suba_init(void *mem, size_t size, int rst, size_t mincell);
void *suba_alloc(void *suba, size_t size, int zero);
void *suba_realloc(void *suba, void *ptr, size_t size);
int suba_free(void *suba, void *ptr);
#endif


/* ******************** HEAP MANAGEMENT ******************** */

inline static void
kogmo_rtdb_heap_lock (kogmo_rtdb_handle_t *db_h)
{
  kogmo_rtdb_ipc_mutex_lock(&db_h->localdata_p->heap_lock);
}
inline static void
kogmo_rtdb_heap_unlock (kogmo_rtdb_handle_t *db_h)
{
  kogmo_rtdb_ipc_mutex_unlock(&db_h->localdata_p->heap_lock);
}


/*! \brief Allocate Memory for Object Data with Object Data Heap.
 * For internal use only.
 * returns Index-Pointer relative to Heap begin.
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_mem_alloc (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objsize_t size )
{
  void *ptr = NULL;
  void *base = db_h->localdata_p -> heap;

  DBGL(DBGL_DB,"mem_alloc: heap pointers: base %p", 
       db_h->localdata_p -> heap);

  DBGL(DBGL_DB,"mem_alloc: %lli bytes (%lli used, %lli free)",
       (long long int) size,
       (long long int) db_h->localdata_p -> heap_used,
       (long long int) db_h->localdata_p -> heap_free);
  if ( size <= 0 ) return -1;

  // speedup: round size to page_size (achieved with tlsf parameters)
  kogmo_rtdb_heap_lock(db_h);
#if defined(RTMALLOC_tlsf)
  ptr = malloc_ex (size, base);
#elif defined(RTMALLOC_suba)
  ptr = suba_alloc(db_h->heapinfo, size, 0/* dont zero*/);
#else
  if ( size <= db_h->localdata_p -> heap_free )
    {
      ptr = base + db_h->localdata_p -> heap_used;
    }
#endif
  DBG("mem_malloc: at %p",ptr);
  if ( ptr == NULL )
    {
      kogmo_rtdb_heap_unlock(db_h);
      ERR("object mem_alloc failed for %i bytes, only %lli free", size,
          (long long int) db_h->localdata_p -> heap_free );
      return -1;
    }
  db_h->localdata_p -> heap_free -= size;
  db_h->localdata_p -> heap_used += size;
  kogmo_rtdb_heap_unlock(db_h);
  memset (ptr, 0, size);
  DBG("allocated mem cleared");
  return ptr-base;
}

void
kogmo_rtdb_obj_mem_free (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objsize_t idx,
                         kogmo_rtdb_objsize_t size )
{
#if defined(RTMALLOC_tlsf)
  void *base = db_h->localdata_p -> heap;
#endif
  void *ptr = db_h->localdata_p -> heap + idx;
  DBGL(DBGL_DB,"mem_free: %i bytes at %p", size, ptr);
  kogmo_rtdb_heap_lock(db_h);
#if defined(RTMALLOC_tlsf)
  free_ex (ptr, base);
  db_h->localdata_p -> heap_free += size;
  db_h->localdata_p -> heap_used -= size;
#elif defined(RTMALLOC_suba)
  suba_free (db_h-> heapinfo, ptr);
  db_h->localdata_p -> heap_free += size;
  db_h->localdata_p -> heap_used -= size;
#else
// do nothing
#endif
  kogmo_rtdb_heap_unlock(db_h);
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_mem_init (kogmo_rtdb_handle_t *db_h)
{
  kogmo_rtdb_objsize_t size = db_h->localdata_p -> heap_size;
  kogmo_rtdb_objsize_t free;
  void *base = db_h->localdata_p -> heap;
  DBGL(DBGL_DB,"mem_init: %i bytes at %p", size, base);
#if defined(RTMALLOC_tlsf)
  free = init_memory_pool (size, base);
#elif defined(RTMALLOC_suba)
  db_h->heapinfo =
    suba_init(base, size, 1/*rst*/, 1024/*mincell*/);
  free = size;
#else
  free = size;
#endif
  db_h->localdata_p -> heap_free = free;
  db_h->localdata_p -> heap_used = 0;
  return free;
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_mem_attach (kogmo_rtdb_handle_t *db_h)
{
#if !defined(RTMALLOC_tlsf)
  kogmo_rtdb_objsize_t size = db_h->localdata_p -> heap_size;
  void *base = db_h->localdata_p ->heap;
#endif
#if defined(RTMALLOC_tlsf)
  DBGL(DBGL_DB,"mem_attach(tlsf)");
// do nothing
#elif defined(RTMALLOC_suba)
  db_h->heapinfo =
    suba_init(base, size, 0/*rst*/, 0);
  DBGL(DBGL_DB,"mem_attach(suba) at %p with %p",base,db_h->heapinfo);
  if(db_h->heapinfo == NULL) return KOGMO_RTDB_ERR_INVALID;
#else
  DBGL(DBGL_DB,"mem_attach(static)");
// do nothing
#endif
  return 0;
}

void
kogmo_rtdb_obj_mem_destroy (kogmo_rtdb_handle_t *db_h)
{
  void *base = db_h->localdata_p -> heap;
  DBGL(DBGL_DB,"mem_destroy: release %p, %lli bytes were used, %lli free",
      base,
      (long long int) db_h->localdata_p -> heap_used,
      (long long int) db_h->localdata_p -> heap_free);
#if defined(RTMALLOC_tlsf)
  destroy_memory_pool (base);
#elif defined(RTMALLOC_suba)
// do nothing
#else
// do nothing
#endif
}


