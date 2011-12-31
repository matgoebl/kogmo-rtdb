/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file kogmo_rtdb_housekeeping.c
 * \brief Housekeeping Functions for the RTDB (used by manager process)
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */
 
#include "kogmo_rtdb_internal.h"

int
kogmo_rtdb_objmeta_purge_objs (kogmo_rtdb_handle_t *db_h)
{
  int i;
  kogmo_rtdb_obj_info_t *scan_objmeta_p;
  kogmo_rtdb_objid_t scan_oid;
  kogmo_timestamp_t ts,scan_delete_ts;
  int purge_keep_alloc = 0;
  int percent_free;

  CHK_DBH("kogmo_rtdb_objmeta_purge",db_h,0);

  percent_free = (float)db_h->localdata_p -> heap_free * 100 /
                 db_h->localdata_p -> heap_size;

#ifndef KOGMO_RTDB_HARDREALTIME
  if ( percent_free < KOGMO_RTDB_PURGE_KEEPALLOC_PERCFREE )
    {
      DBG("purging preallocated memory, because only %i%% (<%i%%) is free",
          percent_free, KOGMO_RTDB_PURGE_KEEPALLOC_PERCFREE);
      purge_keep_alloc = 1;
    }
#endif

  ts = kogmo_rtdb_timestamp_now(db_h);

  kogmo_rtdb_objmeta_lock(db_h);
  for(i=0;i<KOGMO_RTDB_OBJ_MAX;i++)
    {
      scan_objmeta_p = &db_h->localdata_p -> objmeta[i];
      scan_oid = scan_objmeta_p->oid;
      scan_delete_ts = scan_objmeta_p -> deleted_ts;
      if (scan_oid && scan_delete_ts)
        {
          scan_delete_ts = kogmo_timestamp_add_secs(
               scan_delete_ts,
               scan_objmeta_p -> history_interval > db_h->localdata_p->default_keep_deleted_interval ?
               scan_objmeta_p -> history_interval : db_h->localdata_p->default_keep_deleted_interval);
          if ( scan_delete_ts < ts )
            {
              if ( scan_objmeta_p->flags.keep_alloc &&
                   ( !purge_keep_alloc ||
                     kogmo_timestamp_diff_secs ( scan_delete_ts, ts ) < KOGMO_RTDB_PURGE_KEEPALLOC_MAXSECS) )
                continue; // this is kept allocated and alloc-purce time is not reached or we do not purge
              kogmo_rtdb_obj_purgeslot(db_h, i);
              kogmo_rtdb_objmeta_unlock_notify(db_h);
              // give priority inheritance protocols a point to kick in
              kogmo_rtdb_objmeta_lock(db_h);
            }
        }
    }
  kogmo_rtdb_objmeta_unlock(db_h);

  percent_free = (float)db_h->localdata_p -> heap_free * 100 /
                 db_h->localdata_p -> heap_size;
  DBG("purge: %i%% free (%lli/%lli)", percent_free,
      (long long int) db_h->localdata_p -> heap_free,
      (long long int) db_h->localdata_p -> heap_size);

  return 0;
}


int
kogmo_rtdb_objmeta_purge_procs (kogmo_rtdb_handle_t *db_h)
{
  int i;
  kogmo_rtdb_objid_t err;

  CHK_DBH("kogmo_rtdb_objmeta_purge_procs",db_h,0);

  for(i=0;i<KOGMO_RTDB_PROC_MAX;i++)
    {
      if(db_h->ipc_h.shm_p->proc[i].proc_oid != 0 )
        {
          err = kill ( db_h->ipc_h.shm_p->proc[i].pid , 0);
          if ( err == -1 && errno == ESRCH )
            {
              kogmo_rtdb_obj_info_t used_objmeta;
              kogmo_rtdb_objid_list_t objlist;
              int j, immediately_delete;
              kogmo_rtdb_objid_t err;

              immediately_delete = db_h->ipc_h.shm_p->proc[i].flags & KOGMO_RTDB_CONNECT_FLAGS_IMMEDIATELYDELETE;

              DBGL(DBGL_APP,"DEAD PROCESS: %s [OID %lli, PID %i]",
                      db_h->ipc_h.shm_p->proc[i].name,
                      (long long int)db_h->ipc_h.shm_p->proc[i].proc_oid,
                      db_h->ipc_h.shm_p->proc[i].pid);

              err = kogmo_rtdb_obj_searchinfo_deleted (db_h, "", 0, 0,
                        db_h->ipc_h.shm_p->proc[i].proc_oid, 0, objlist, 0);
              if (err < 0 )
                {
                  DBGL(DBGL_APP,"search for objectlist failed: %d",err);
                }
              else
                {
                  DBGL(DBGL_APP,"dead process has %i objects to%s delete",err,
                       immediately_delete ? " immediately":"");
                  for (j=0; objlist[j] != 0; j++ )
                    {
                      err = kogmo_rtdb_obj_readinfo (db_h, objlist[j], 0, &used_objmeta );
                      if ( err >= 0 && used_objmeta.flags.persistent )
                        {
                          DBGL(DBGL_APP,"keeping persistent object '%s'",used_objmeta.name);
                          continue;
                        }
                       else
                         {
                           used_objmeta.oid = objlist[j]; // enough for delete
                         }
                        kogmo_rtdb_obj_delete_imm(db_h, &used_objmeta, immediately_delete);
                    }
                }

              kogmo_rtdb_ipc_mutex_lock (&db_h->ipc_h.shm_p->proc_lock);
              memset(&db_h->ipc_h.shm_p->proc[i],0,sizeof(struct kogmo_rtdb_ipc_process_t));
              db_h->ipc_h.shm_p->proc_free++;
              kogmo_rtdb_ipc_mutex_unlock (&db_h->ipc_h.shm_p->proc_lock);
            }
        }
    }
  return 0;
}

int
kogmo_rtdb_kill_procs (kogmo_rtdb_handle_t *db_h, int sig)
{
  int i;
  CHK_DBH("kogmo_rtdb_kill_procs",db_h,0);
  DBGL(DBGL_APP,"killing connected processes with signal %i..", sig);
  for(i=0;i<KOGMO_RTDB_PROC_MAX;i++)
    {
      if(db_h->ipc_h.shm_p->proc[i].proc_oid != 0 )
        {
          DBGL(DBGL_APP,"killing process %s [OID %lli, PID %i]",
                  db_h->ipc_h.shm_p->proc[i].name,
                  (long long int)db_h->ipc_h.shm_p->proc[i].proc_oid,
                  db_h->ipc_h.shm_p->proc[i].pid);
          kill ( db_h->ipc_h.shm_p->proc[i].pid, sig);
        }
    }
  return 0;
}

int
kogmo_rtdb_objmeta_upd_stats (kogmo_rtdb_handle_t *db_h)
{
  kogmo_rtdb_objid_t oid;
  //kogmo_rtdb_obj_info_t rtdbobj_info;
  kogmo_rtdb_obj_c3_rtdb_t rtdbobj;
  kogmo_rtdb_objid_t err;
  CHK_DBH("kogmo_rtdb_objmeta_upd_stats",db_h,0);
  oid = kogmo_rtdb_obj_searchinfo (db_h, "rtdb", KOGMO_RTDB_OBJTYPE_C3_RTDB, 0, 0, 0, NULL, 1);
  if (oid<0) return oid;
  //err = kogmo_rtdb_obj_readinfo (db_h, oid, 0, &rtdbobj_info);
  //if (err<0) return err;
  err = kogmo_rtdb_obj_readdata (db_h, oid, 0, &rtdbobj, sizeof(rtdbobj));
  if (err<0) return err;
  rtdbobj.base.data_ts = kogmo_timestamp_now();
  rtdbobj.rtdb.objects_free=db_h->localdata_p->objmeta_free;
  rtdbobj.rtdb.processes_free=db_h->ipc_h.shm_p->proc_free;
  rtdbobj.rtdb.memory_free=db_h->localdata_p -> heap_free;
  err = kogmo_rtdb_obj_writedata (db_h, oid, &rtdbobj);
  if (err<0) return err;
  return 0;
}

