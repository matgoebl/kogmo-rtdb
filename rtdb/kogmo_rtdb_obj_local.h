/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file kogmo_rtdb_obj_local.h
 * \brief Interface to Manage the Real-time Vehicle Database Objects (internal)
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_OBJ_LOCAL_H
#define KOGMO_RTDB_OBJ_LOCAL_H

#ifdef __cplusplus
extern "C" {
namespace KogniMobil {
#endif


#ifndef KOGMO_RTDB_OBJ_MAX
#define KOGMO_RTDB_OBJ_MAX 1000
#endif

// this is database-global
struct kogmo_rtdb_obj_local_t {
 uint64_t objmeta_oid_next;

 kogmo_rtdb_obj_info_t objmeta[KOGMO_RTDB_OBJ_MAX];
 uint32_t objmeta_free;
 pthread_mutex_t objmeta_lock;
 pthread_cond_t  objmeta_changenotify;

 pthread_mutex_t obj_lock[KOGMO_RTDB_OBJ_MAX];
 pthread_cond_t  obj_changenotify[KOGMO_RTDB_OBJ_MAX];
 pthread_mutex_t obj_changenotify_lock[KOGMO_RTDB_OBJ_MAX];

 int32_t rtdb_trace;
 int32_t rtdb_tracebufsize;

 struct
  {
   uint32_t simmode : 1;
   uint32_t no_notifies : 1;
  } flags;
 kogmo_timestamp_t sim_ts;

 float default_history_interval;
 float default_max_cycletime;
 float default_keep_deleted_interval;

 size_t heap_size;
 size_t heap_free;
 size_t heap_used;
 pthread_mutex_t heap_lock;

 char heap[1]; // will be extended
};

#ifndef KOGMO_RTDB_DEFAULT_HEAP_SIZE
#define KOGMO_RTDB_DEFAULT_HEAP_SIZE (64*1024*1024)
#endif
#define KOGMO_RTDB_DEFAULT_DBHOST "local:system"
#define KOGMO_RTDB_MINIMUM_HEAP_SIZE (512*1024)


// this is process-local
typedef struct {
 kogmo_rtdb_obj_info_t procobjmeta;
 kogmo_rtdb_obj_c3_process_t procobj;
 struct kogmo_rtdb_obj_local_t *localdata_p;
 long int localdata_size;
 void *heapinfo;
 struct kogmo_rtdb_ipc_handle_t ipc_h;
} kogmo_rtdb_handle_t;


#include <stdio.h>
inline static int
kogmo_rtdb_obj_slotnum (kogmo_rtdb_handle_t *db_h,
                        kogmo_rtdb_obj_info_t *objmeta_p)
{
  return (int) ( ( (long int)objmeta_p - (long int) (&db_h->localdata_p->objmeta[0]))
          / sizeof( kogmo_rtdb_obj_info_t ) );
}



#ifdef __cplusplus
}; /* namespace KogniMobil */
}; /* extern "C" */
#endif

#endif /* KOGMO_RTDB_OBJ_LOCAL_H */
