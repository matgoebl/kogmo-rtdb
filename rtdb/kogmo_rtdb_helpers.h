/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */

#ifndef KOGMO_RTDB_HELPERS_H
#define KOGMO_RTDB_HELPERS_H

#ifdef MACOSX
# include <machine/endian.h>
# define __BYTE_ORDER BYTE_ORDER
# define __LITTLE_ENDIAN LITTLE_ENDIAN
# define __BIG_ENDIAN BIG_ENDIAN
#else
# include <endian.h>
#endif

#ifndef __BYTE_ORDER
# error no byte order defined
#endif


// 32-bit functions to copy int64 values in a controlled word order

// note: better use pointer variables or volatiles for src+dest to suppress unwanted compiler optimizations
#define _COPY_INT64(dest,src,word) ( (int*) ((void*)&(dest)) )[word] = ( (int*) ((void*)&(src)) )[word]
//DEBUG: #define _COPY_INT64(dest,src,word) do { ( (int*) ((void*)&(dest)) )[word] = ( (int*) ((void*)&(src)) )[word]; printf("%lli %i %i\n",dest,(int)(dest>>32)&0xFFFFFFFF,(int)(dest&0xFF
#define _CMP_INT64(dest,src,word) ( (int*) ((void*)&(dest)) )[word] == ( (int*) ((void*)&(src)) )[word]

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COPY_INT64_HIGHFIRST(dest,src) do { _COPY_INT64(dest,src,1); _COPY_INT64(dest,src,0); } while (0)
#define COPY_INT64_LOWFIRST(dest,src)  do { _COPY_INT64(dest,src,0); _COPY_INT64(dest,src,1); } while (0)
#define CMP_INT64_HIGH(dest,src) ( _CMP_INT64(dest,src,1) )
#elif __BYTE_ORDER == __BIG_ENDIAN
#define COPY_INT64_HIGHFIRST(dest,src) do { _COPY_INT64(dest,src,0); _COPY_INT64(dest,src,1); } while (0)
#define COPY_INT64_LOWFIRST(dest,src)  do { _COPY_INT64(dest,src,1); _COPY_INT64(dest,src,0); } while (0)
#define CMP_INT64_HIGH(dest,src) ( _CMP_INT64(dest,src,0) )
#else
# error unknown byte order
#endif


kogmo_rtdb_obj_info_t *
kogmo_rtdb_obj_findmeta_byid (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid );

inline static int
this_process_is_manager(kogmo_rtdb_handle_t *db_h)
{
  return db_h->ipc_h.this_process.flags&KOGMO_RTDB_CONNECT_FLAGS_MANAGER?1:0;
}

inline static int
this_process_is_admin(kogmo_rtdb_handle_t *db_h)
{
  return db_h->ipc_h.this_process.flags & ( KOGMO_RTDB_CONNECT_FLAGS_MANAGER | KOGMO_RTDB_CONNECT_FLAGS_ADMIN ) ? 1 : 0;
}


// Simple error handling and debugging

#define DBGL_MSG 1
#define DBGL_API 2
#define DBGL_DB 4
#define DBGL_IPC 8
#define DBGL_APP 16
#define DBGL_LOCK 32
#define DBGL_VERBOSE 256
#define DBGL_VERYVERBOSE 512
#define DBGL_MAX 1023

#ifndef DEBUG
 //#define DEBUG (getenv("DEBUG")?atoi(getenv("DEBUG")):0)
 //#define DEBUG 255
 //#define DEBUG 0
 //#define DEBUG (getenv("KOGMO_RTDB_DEBUG")?atoi(getenv("KOGMO_RTDB_DEBUG"))|kogmo_rtdb_debug:kogmo_rtdb_debug)
 #define DEBUG (kogmo_rtdb_debug)
#endif
#define IFDBGL(level) \
 if( ( DEBUG & (level) ) == (level) )
#define STDERR (DEBUG & DBGL_VERBOSE ? stdout : stderr)
#define DBGL(level,msg...) do { \
 IFDBGL(level) { \
  fprintf(STDERR,"%d DBG: ",getpid()); \
  fprintf(STDERR,msg); \
  fprintf(STDERR,"\n"); \
 } } while (0)
#define DBG(msg...) do { \
 if(DEBUG & DBGL_VERBOSE) { \
  fprintf(STDERR,"%d DBG: ",getpid()); \
  fprintf(STDERR,msg); \
  fprintf(STDERR,"\n"); \
 } } while (0)
#define ERR(msg...) do { \
  if(DEBUG) { \
   fprintf(STDERR,"%d ERR: in %s line %i: ",getpid(),__FILE__,__LINE__); \
  } else { \
   fprintf(STDERR,"ERROR: "); \
  } \
 fprintf(STDERR,msg); \
 fprintf(STDERR,"\n"); \
 } while (0)
#define DIE(msg...) do { \
  if(DEBUG) { \
   fprintf(STDERR,"DIE %d: in %s line %i: ",getpid(),__FILE__,__LINE__); \
  } else { \
   fprintf(STDERR,"DIED: "); \
  } \
 fprintf(STDERR,msg); \
 fprintf(STDERR,"\n"); \
 exit(1); \
 } while (0)

#define XXx
#define XX do { \
 if(DEBUG) { \
  fprintf(STDERR,"%d STOP: in %s line %i. <return>\n",getpid(),__FILE__,__LINE__); \
  sleep(1); \
  getchar(); \
 } } while (0);

#define MSG(msg...) do { \
 IFDBGL( DBGL_MSG ) { \
  fprintf(STDERR,"%d MSG: ",getpid()); \
  fprintf(STDERR,msg); \
  fprintf(STDERR,"\n"); \
 } } while (0)

#define MSGN(msg...) do { \
 IFDBGL( DBGL_MSG ) { \
   fprintf(STDERR,msg); \
 } } while (0)



// pointer check at function entry

#define CHK_DBH(funcname,db_h,writing) \
do { \
  if ((db_h) == NULL || (db_h)->localdata_p == NULL) { \
      ERR("%s(): not connected (RTDB handle is NULL)",funcname); \
      return -KOGMO_RTDB_ERR_NOCONN; \
  } \
  DBGL(DBGL_API,"API: %s()",funcname); \
} while(0)

// read one byte to see if pointer is usable,
// if not, it segfaults early, before entering the function.
// problem: does not check full area and not write access.
#define CHK_PTR(ptr) \
  if ( (ptr) == NULL ) { \
      ERR("pointer passed to the function is NULL"); \
      return -KOGMO_RTDB_ERR_INVALID; \
  } else { \
      volatile char pointer_check __attribute__ ((__unused__)) = *((char*)(ptr)); \
  }

#endif /* KOGMO_RTDB_HELPERS_H */
