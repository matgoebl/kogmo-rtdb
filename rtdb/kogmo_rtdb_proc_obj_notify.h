/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */
/*! \file kogmo_rtdb_obj_local.h
 * \brief Interface to Manage the Real-time Vehicle Database Objects (internal)
 *
 * Copyright (c) 2003-2006 Matthias Goebl <mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_PROC_OBJ_NOTIFY_H
#define KOGMO_RTDB_PROC_OBJ_NOTIFY_H

inline static void
kogmo_rtdb_objmeta_lock (kogmo_rtdb_handle_t *db_h)
{
  DBGL(DBGL_LOCK,"kogmo_rtdb_objmeta_lock");
  kogmo_rtdb_ipc_mutex_lock(&db_h->localdata_p->objmeta_lock);
}
inline static void
kogmo_rtdb_objmeta_unlock (kogmo_rtdb_handle_t *db_h)
{
  DBGL(DBGL_LOCK,"kogmo_rtdb_objmeta_unlock");
  kogmo_rtdb_ipc_mutex_unlock(&db_h->localdata_p->objmeta_lock);
}


inline static void
kogmo_rtdb_obj_lock (kogmo_rtdb_handle_t *db_h,
                     kogmo_rtdb_obj_info_t *objmeta_p)
{
  DBGL(DBGL_LOCK,"kogmo_rtdb_obj_lock(objslot %i)",kogmo_rtdb_obj_slotnum (db_h, objmeta_p));
  kogmo_rtdb_ipc_mutex_lock(
   &db_h->localdata_p -> obj_lock[ kogmo_rtdb_obj_slotnum (db_h, objmeta_p) ] );
}
inline static void
kogmo_rtdb_obj_unlock (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_obj_info_t *objmeta_p)
{
  DBGL(DBGL_LOCK,"kogmo_rtdb_obj_unlock(objslot %i)",kogmo_rtdb_obj_slotnum (db_h, objmeta_p));
  kogmo_rtdb_ipc_mutex_unlock(
   &db_h->localdata_p -> obj_lock[ kogmo_rtdb_obj_slotnum (db_h, objmeta_p) ] );
}


/* ***** NOTIFICATIONS ***** */



inline static void
kogmo_rtdb_objmeta_unlock_notify (kogmo_rtdb_handle_t *db_h)
{
  DBGL(DBGL_LOCK,"kogmo_rtdb_objmeta_unlock_notify");
  kogmo_rtdb_ipc_condvar_signalall( &db_h->localdata_p -> objmeta_changenotify );
  kogmo_rtdb_ipc_mutex_unlock(&db_h->localdata_p->objmeta_lock);
  // NON-working fix against futex freeze: broadcast twice
  // kogmo_rtdb_ipc_condvar_signalall( &db_h->localdata_p -> objmeta_changenotify );
}


// passing meta notifications
inline static void
kogmo_rtdb_obj_wait_metanotify_prepare (kogmo_rtdb_handle_t *db_h)
{
  DBGL(DBGL_LOCK,"kogmo_rtdb_obj_wait_metanotify_prepare");
  kogmo_rtdb_ipc_mutex_lock( &db_h->localdata_p -> objmeta_lock );
}
inline static void
kogmo_rtdb_obj_wait_metanotify_done (kogmo_rtdb_handle_t *db_h)
{
  DBGL(DBGL_LOCK,"kogmo_rtdb_obj_wait_metanotify_done");
  kogmo_rtdb_ipc_mutex_unlock(&db_h->localdata_p -> objmeta_lock );
}
inline static int
kogmo_rtdb_obj_wait_metanotify (kogmo_rtdb_handle_t *db_h, kogmo_timestamp_t wakeup_ts)
{
  int ret;
  DBGL(DBGL_LOCK,"kogmo_rtdb_obj_wait_metanotify");
  ret = kogmo_rtdb_ipc_condvar_wait( &db_h->localdata_p -> objmeta_changenotify,
                                     &db_h->localdata_p -> objmeta_lock,
                                     wakeup_ts);
  if ( ret == -KOGMO_RTDB_ERR_TIMEOUT )
    DBGL(DBGL_LOCK,"kogmo_rtdb_obj_wait_metanotify() returns timeout %i",ret);
  //else
  //  DBGL(DBGL_LOCK,"kogmo_rtdb_obj_wait_metanotify() returns NO timeout %i",ret);
  kogmo_rtdb_ipc_mutex_unlock( &db_h->localdata_p -> objmeta_lock);
  return ret;
}

// passing notifications
inline static void
kogmo_rtdb_obj_do_notify_prepare (kogmo_rtdb_handle_t *db_h,
                     kogmo_rtdb_obj_info_t *objmeta_p)
{
  DBGL(DBGL_LOCK,"kogmo_rtdb_obj_do_notify_prepare(objslot %i)",kogmo_rtdb_obj_slotnum (db_h, objmeta_p));
  kogmo_rtdb_ipc_mutex_lock(
   &db_h->localdata_p -> obj_changenotify_lock[ kogmo_rtdb_obj_slotnum (db_h, objmeta_p) ] );
}
inline static void
kogmo_rtdb_obj_do_notify (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_obj_info_t *objmeta_p)
{
  DBGL(DBGL_LOCK,"kogmo_rtdb_obj_do_notify(objslot %i)",kogmo_rtdb_obj_slotnum (db_h, objmeta_p));
  kogmo_rtdb_ipc_condvar_signalall(
   &db_h->localdata_p -> obj_changenotify[ kogmo_rtdb_obj_slotnum (db_h, objmeta_p) ] );
  kogmo_rtdb_ipc_mutex_unlock(
   &db_h->localdata_p -> obj_changenotify_lock[ kogmo_rtdb_obj_slotnum (db_h, objmeta_p) ] );
}
inline static void
kogmo_rtdb_obj_wait_notify_prepare (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_obj_info_t *objmeta_p)
{
  DBGL(DBGL_LOCK,"kogmo_rtdb_obj_wait_notify_prepare(objslot %i)",kogmo_rtdb_obj_slotnum (db_h, objmeta_p));
  kogmo_rtdb_ipc_mutex_lock(
   &db_h->localdata_p -> obj_changenotify_lock[ kogmo_rtdb_obj_slotnum (db_h, objmeta_p) ] );
}
inline static void
kogmo_rtdb_obj_wait_notify_done (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_obj_info_t *objmeta_p)
{
  DBGL(DBGL_LOCK,"kogmo_rtdb_obj_wait_notify_done(objslot %i)",kogmo_rtdb_obj_slotnum (db_h, objmeta_p));
  kogmo_rtdb_ipc_mutex_unlock(
   &db_h->localdata_p -> obj_changenotify_lock[ kogmo_rtdb_obj_slotnum (db_h, objmeta_p) ] );
}
inline static int
kogmo_rtdb_obj_wait_notify (kogmo_rtdb_handle_t *db_h,
                            kogmo_rtdb_obj_info_t *objmeta_p, kogmo_timestamp_t wakeup_ts)
{
  int ret;
  DBGL(DBGL_LOCK,"kogmo_rtdb_obj_wait_notify(objslot %i)",kogmo_rtdb_obj_slotnum (db_h, objmeta_p));
  ret = kogmo_rtdb_ipc_condvar_wait(
         &db_h->localdata_p ->
           obj_changenotify[ kogmo_rtdb_obj_slotnum (db_h, objmeta_p) ],
         &db_h->localdata_p ->
           obj_changenotify_lock[ kogmo_rtdb_obj_slotnum (db_h, objmeta_p) ],
         wakeup_ts);
  kogmo_rtdb_ipc_mutex_unlock(
   &db_h->localdata_p -> obj_changenotify_lock[ kogmo_rtdb_obj_slotnum (db_h, objmeta_p) ] );
  return ret;
}






#endif /* KOGMO_RTDB_PROC_OBJ_NOTIFY_H */
