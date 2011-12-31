/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */

/*! \file kogmo_rtdb_obj_local.c
 * \brief Functions to Manage the Real-time Vehicle Database Objects
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include "kogmo_rtdb_internal.h"
#include <ctype.h>

int kogmo_rtdb_debug = 0;


/* ******************** INITIALISATION & EXIT ******************** */
static void
exit_handler(int code, void *db_h)
{
 DBGL(DBGL_API,"kogmo_rtdb_exit_handler: exiting..");
 DBG("kogmo_rtdb exit-handler: exiting..");
 // evtl. pthread_kill_other_threads_np(); usleep(300000);
 kogmo_rtdb_disconnect (db_h, (void*)1);
 exit(code);
}



void
kogmo_rtdb_obj_local_init (kogmo_rtdb_handle_t *db_h,
                           kogmo_rtdb_objsize_t heap_size)
{
  int i;
  DBGL(DBGL_DB,"local_init()");

  // already done by ipc-init
  //memset(db_h->localdata_p, 0, sizeof(struct kogmo_rtdb_obj_local_t) +
  //                             db_h->localdata_p->heap_size);

  // objmeta[]=[0..0] initialized by memset(..,0,..) 
  db_h->localdata_p->objmeta_oid_next=1;

  db_h->localdata_p->default_history_interval =
    KOGMO_RTDB_DEFAULT_HISTORY_INTERVAL;
  db_h->localdata_p->default_max_cycletime =
    KOGMO_RTDB_DEFAULT_MAX_CYCLETIME;
  db_h->localdata_p->default_keep_deleted_interval =
    KOGMO_RTDB_KEEP_DELETED_INTERVAL;

  db_h->localdata_p->heap_size = heap_size;

  kogmo_rtdb_ipc_mutex_init(&db_h->localdata_p->objmeta_lock);
  kogmo_rtdb_ipc_condvar_init(&db_h->localdata_p->objmeta_changenotify);
  db_h->localdata_p->objmeta_free=KOGMO_RTDB_OBJ_MAX;

  for ( i=0; i < KOGMO_RTDB_OBJ_MAX; i++)
    {
      kogmo_rtdb_ipc_mutex_init(&db_h->localdata_p->obj_lock[i]);
      kogmo_rtdb_ipc_mutex_init(&db_h->localdata_p->obj_changenotify_lock[i]);
      kogmo_rtdb_ipc_condvar_init(&db_h->localdata_p->obj_changenotify[i]);
    }
  kogmo_rtdb_ipc_mutex_init(&db_h->localdata_p->heap_lock);
  kogmo_rtdb_obj_mem_init (db_h);
  kogmo_rtdb_obj_mem_alloc (db_h, 2); // so there will be no index 0 in future requests
}


void
kogmo_rtdb_obj_local_destroy (kogmo_rtdb_handle_t *db_h)
{
  int i;
  DBGL(DBGL_DB,"local_destroy()");
  kogmo_rtdb_obj_mem_destroy (db_h);
  DBGL(DBGL_DB,"local_destroy() mutexes");
  kogmo_rtdb_ipc_mutex_destroy(&db_h->localdata_p->heap_lock);
  for ( i=0; i < KOGMO_RTDB_OBJ_MAX; i++)
    {
      kogmo_rtdb_ipc_mutex_destroy(&db_h->localdata_p->obj_lock[i]);
      kogmo_rtdb_ipc_mutex_destroy(&db_h->localdata_p->
                                   obj_changenotify_lock[i]);
      kogmo_rtdb_ipc_condvar_destroy(&db_h->localdata_p->
                                     obj_changenotify[i]);
    }
  kogmo_rtdb_ipc_mutex_destroy(&db_h->localdata_p->objmeta_lock);
  kogmo_rtdb_ipc_condvar_destroy(&db_h->localdata_p->
                                 objmeta_changenotify);
  DBGL(DBGL_DB,"local_destroy() mutexes done");
}


int
kogmo_rtdb_connect_initinfo (kogmo_rtdb_connect_info_t *conninfo,
                             _const char *dbhost,
                             _const char *procname, float cycletime)
{
  // main reason for this function, because defaults are always 0
  DBGL(DBGL_API,"kogmo_rtdb_connect_initinfo()");

  memset(conninfo,0,sizeof(kogmo_rtdb_connect_info_t));

  conninfo->procname = procname;
  conninfo->dbhost = dbhost;
  conninfo->cycletime = cycletime;
  conninfo->version = getenv("KOGMO_REV") ? atoi(getenv("KOGMO_REV")) : 0;
  return 0;
}

kogmo_rtdb_objid_t
kogmo_rtdb_connect (kogmo_rtdb_handle_t **db_hp,
                    kogmo_rtdb_connect_info_t *conninfo)
{
  kogmo_rtdb_objid_t connoid,rootoid,procoid,proclistoid;
  kogmo_rtdb_obj_info_t objmeta;
  long int localdata_size=0;
  int ret;
  char *local_dbhost=NULL;
  kogmo_rtdb_handle_t *db_h;

  if ( getenv("KOGMO_RTDB_DEBUG") )
    kogmo_rtdb_debug |= atoi(getenv("KOGMO_RTDB_DEBUG"));

  DBGL(DBGL_API,"kogmo_rtdb_connect()");
  CHK_PTR(db_hp);

  db_h = malloc ( sizeof ( kogmo_rtdb_handle_t ) );
  if ( db_h == NULL )
   {
     if ( conninfo->flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
       return ( -KOGMO_RTDB_ERR_NOMEMORY );
     else
       DIE("cannot allocate memory for handle");
   }
  *db_hp=db_h;

  DBG("db handle at %p, points to %p",&db_h,db_h);
  db_h->localdata_p = NULL; // still not connected

  if ( ! conninfo->cycletime )
    conninfo->cycletime = KOGMO_RTDB_DEFAULT_MAX_CYCLETIME;

  // only relevant for manager, will be overwritten for normal processes
  localdata_size = KOGMO_RTDB_DEFAULT_HEAP_SIZE;
  if ( getenv ("KOGMO_RTDB_HEAPSIZE") )
    {
      char *suffix;
      localdata_size = strtol ( getenv ("KOGMO_RTDB_HEAPSIZE"), &suffix, 0);
      if ( toupper(*suffix) == 'K' ) localdata_size *= 1024;
      if ( toupper(*suffix) == 'M' ) localdata_size *= 1024*1024;
      if ( toupper(*suffix) == 'G' ) localdata_size *= 1024*1024*1024;
    }

  if ( localdata_size < KOGMO_RTDB_MINIMUM_HEAP_SIZE )
    DIE("heapsize %lli too small, minimum is %lli bytes",
        (long long int) localdata_size, (long long int) KOGMO_RTDB_MINIMUM_HEAP_SIZE);

  if (conninfo->dbhost==NULL || (conninfo->dbhost!=NULL && conninfo->dbhost[0]=='\0'))
    {
      conninfo->dbhost = KOGMO_RTDB_DEFAULT_DBHOST;
      if (getenv("KOGMO_RTDB_DBHOST"))
        conninfo->dbhost = getenv("KOGMO_RTDB_DBHOST");
    }

  if ( conninfo->dbhost!=NULL && conninfo->dbhost[0] != '\0' )
    {
      if ( strncmp ( conninfo->dbhost, "local:", 6 ) != 0 )
      {
        if ( conninfo->flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
          return -KOGMO_RTDB_ERR_INVALID;
        else
          DIE("illegal dbhost specification '%s', only 'local:' implemented",conninfo->dbhost);
      }
      local_dbhost = &conninfo->dbhost[6];
    }

  DBGL(DBGL_API,"kogmo_rtdb_connect to dbhost:%s",conninfo->dbhost);
  connoid = kogmo_rtdb_ipc_connect (&db_h->ipc_h, local_dbhost,
                                    conninfo->procname,
                                    conninfo->cycletime, conninfo->flags,
                                    (char**)&db_h->localdata_p,
                                    &localdata_size,
                                    exit_handler, (void *)db_h);
  if ( connoid < 0 )
    {
      if ( conninfo->flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
        return connoid;
      else
        DIE("error connecting to local database, error %d",connoid);
    }

  //if ( conninfo->dbhost && strncmp ( conninfo->dbhost, "local:", 6 ) != 0 )
  kogmo_rtdb_obj_mem_attach (db_h);

  DBG("local data %li bytes at %p", localdata_size, db_h->localdata_p);

  if ( this_process_is_manager (db_h) )
    {
      db_h->localdata_p->flags.simmode = conninfo->flags & KOGMO_RTDB_CONNECT_FLAGS_SIMULATION ? 1 : 0;
      db_h->localdata_p->flags.no_notifies = conninfo->flags & KOGMO_RTDB_CONNECT_FLAGS_DO_POLLING ? 1 : 0;
      db_h->localdata_p->rtdb_trace = 0;
      db_h->localdata_p->rtdb_tracebufsize=KOGMO_RTDB_TRACE_BUFSIZE_DESIRED;
      ret = kogmo_rtdb_ipc_mq_init(&db_h->ipc_h.tracefd, db_h->ipc_h.shmid, this_process_is_manager (db_h) ? 1:0,
                                   sizeof(struct kogmo_rtdb_trace_msg), KOGMO_RTDB_TRACE_BUFSIZE_DESIRED);
      if ( ret < 0 )
        {
          db_h->localdata_p->rtdb_tracebufsize=KOGMO_RTDB_TRACE_BUFSIZE;
          ret = kogmo_rtdb_ipc_mq_init(&db_h->ipc_h.tracefd, db_h->ipc_h.shmid, this_process_is_manager (db_h) ? 1:0,
                                 sizeof(struct kogmo_rtdb_trace_msg), KOGMO_RTDB_TRACE_BUFSIZE);
          if ( ret < 0 )
            {
              db_h->localdata_p->rtdb_tracebufsize=KOGMO_RTDB_TRACE_BUFSIZE_MIN;
              ret = kogmo_rtdb_ipc_mq_init(&db_h->ipc_h.tracefd, db_h->ipc_h.shmid, this_process_is_manager (db_h) ? 1:0,
                                     sizeof(struct kogmo_rtdb_trace_msg), KOGMO_RTDB_TRACE_BUFSIZE_MIN);
              //xERR("Cannot allocate %d record buffers, only allocating %i.\nPlease enter 'echo 300 > /proc/sys/fs/mqueue/msg_max' as root to fix this.",KOGMO_RTDB_TRACE_BUFSIZE,KOGMO_RTDB_TRACE_BUFSIZE_MIN);
              if (ret<0)
                db_h->localdata_p->rtdb_tracebufsize=0;
                //xDIE("mq_open and init failed");
            }
        }

    }
  else
    {
       if ( db_h->localdata_p->rtdb_tracebufsize != 0 )
         kogmo_rtdb_ipc_mq_init(&db_h->ipc_h.tracefd, db_h->ipc_h.shmid, this_process_is_manager (db_h) ? 1:0,
                               sizeof(struct kogmo_rtdb_trace_msg), 0);
    }

  db_h->ipc_h.rxtracefd = 0;

  if (this_process_is_manager (db_h) )
    {
      kogmo_rtdb_obj_local_init(db_h, localdata_size -
                                      sizeof(struct kogmo_rtdb_obj_local_t) );
      // init root node
      kogmo_rtdb_obj_initinfo (db_h, &objmeta, "root",
                               KOGMO_RTDB_OBJTYPE_C3_ROOT, 0);
      objmeta.max_cycletime = objmeta.avg_cycletime = 1;
      objmeta.history_interval = 1;
      objmeta.flags.unique=1;
      rootoid = kogmo_rtdb_obj_insert (db_h, &objmeta);
      if ( rootoid < 0 )
        {
          if ( conninfo->flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
            return rootoid;
          else
            DIE("error committing init data %d",rootoid);
        }

      // init process list
      kogmo_rtdb_obj_initinfo (db_h, &objmeta, "processes",
                               KOGMO_RTDB_OBJTYPE_C3_PROCESSLIST, 0);
      objmeta.parent_oid = rootoid;
      objmeta.max_cycletime = 1;
      objmeta.history_interval = 1;
      objmeta.flags.unique=1;
      proclistoid = kogmo_rtdb_obj_insert (db_h, &objmeta);
      if ( proclistoid < 0 )
        {
          if ( conninfo->flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
            return proclistoid;
          else
            DIE("error committing init data %d",proclistoid);
        }
    }

  proclistoid = kogmo_rtdb_obj_searchinfo (db_h, "processes",
                                           KOGMO_RTDB_OBJTYPE_C3_PROCESSLIST,
                                           0, 0, 0, NULL, 1);
  if ( proclistoid < 0 )
    {
      if ( conninfo->flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
        return proclistoid;
      else
        DIE("error finding process list %d",proclistoid);
    }

  kogmo_rtdb_obj_initinfo (db_h, &db_h->procobjmeta, conninfo->procname,
                           KOGMO_RTDB_OBJTYPE_C3_PROCESS,
                           sizeof (kogmo_rtdb_obj_c3_process_t));
  db_h->procobjmeta.parent_oid = proclistoid;
  db_h->procobjmeta.max_cycletime = conninfo->cycletime;
  db_h->procobjmeta.avg_cycletime = conninfo->cycletime;
  db_h->procobjmeta.history_interval =
    db_h->localdata_p->default_history_interval;
  db_h->procobj.process.proc_oid = connoid;
  procoid = kogmo_rtdb_obj_insert (db_h, &db_h->procobjmeta);
  if ( procoid < 0 )
    {
      if ( conninfo->flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
        return procoid;
      else
        DIE("error inserting process data %d",procoid);
    }
  // copy data from ipc connection
  db_h->procobj.base.data_ts = db_h->ipc_h.this_process.connected_ts;
  db_h->procobj.base.size = sizeof (kogmo_rtdb_obj_c3_process_t);
  //memcpy (&db_h->procobj.process, &db_h->ipc_h.this_process,
  //        sizeof(struct kogmo_rtdb_ipc_process_t));
  db_h->procobj.process.pid   = db_h->ipc_h.this_process.pid;
  db_h->procobj.process.tid   = db_h->ipc_h.this_process.tid;
  db_h->procobj.process.flags = db_h->ipc_h.this_process.flags;
  db_h->procobj.process.proc_oid = connoid;
  db_h->procobj.process.uid   = getuid();
  db_h->procobj.process.version  = conninfo->version;
  db_h->procobj.process.status = KOGMO_RTDB_PROCSTATUS_UNKNOWN;

  ret = kogmo_rtdb_obj_writedata (db_h, db_h->procobjmeta.oid, &db_h->procobj);
  if ( ret < 0 )
    {
      if ( conninfo->flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
        return ret;
      else
        DIE("error committing process data %d",ret);
    }

  if (this_process_is_manager (db_h) )
    {
      kogmo_rtdb_objid_t dbinfooid;
      kogmo_rtdb_obj_c3_rtdb_t rtdb_obj;
      // init process list
      kogmo_rtdb_obj_initinfo (db_h, &objmeta, "rtdb",
                               KOGMO_RTDB_OBJTYPE_C3_RTDB, 0);
      objmeta.parent_oid = procoid;
      objmeta.max_cycletime = 1;
      objmeta.history_interval = 10;
      objmeta.size_max = sizeof (rtdb_obj);
      objmeta.flags.unique = 1;
      dbinfooid = kogmo_rtdb_obj_insert (db_h, &objmeta);
      if ( dbinfooid < 0 )
        {
          if ( conninfo->flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
            return dbinfooid;
          else
            DIE("error committing init data %d",dbinfooid);
        }
      // fill in data
      kogmo_rtdb_obj_initdata (db_h, &objmeta, &rtdb_obj);
      strncpy (rtdb_obj.rtdb.dbhost,
               (conninfo->dbhost == NULL || conninfo->dbhost[0]=='\0') ?
                "(default)" : conninfo->dbhost,
               KOGMO_RTDB_OBJMETA_NAME_MAXLEN);
      rtdb_obj.rtdb.started_ts = db_h->procobjmeta.created_ts;
      rtdb_obj.rtdb.version_rev = KOGMO_RTDB_REV;
      strncpy (rtdb_obj.rtdb.version_date, KOGMO_RTDB_DATE,
               sizeof (rtdb_obj.rtdb.version_date));
      rtdb_obj.rtdb.version_date[sizeof (rtdb_obj.rtdb.version_date)-1]='\0';
      snprintf (rtdb_obj.rtdb.version_id, sizeof (rtdb_obj.rtdb.version_id),
                KOGMO_RTDB_COPYRIGHT
                "Release %i%s (%s)\n", KOGMO_RTDB_REV, KOGMO_RTDB_REVSPEC, KOGMO_RTDB_DATE);
      rtdb_obj.rtdb.objects_max=rtdb_obj.rtdb.objects_free=KOGMO_RTDB_OBJ_MAX;
      rtdb_obj.rtdb.processes_max=rtdb_obj.rtdb.processes_free=KOGMO_RTDB_PROC_MAX;
      rtdb_obj.rtdb.memory_max=rtdb_obj.rtdb.memory_free=db_h->localdata_p->heap_size;
      ret = kogmo_rtdb_obj_writedata (db_h, dbinfooid, &rtdb_obj);
      if ( ret < 0 )
        {
          if ( conninfo->flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
            return ret;
          else
            DIE("error committing dbinfo data %d",ret);
        }
      // now the database is ready
      ret = kogmo_rtdb_ipc_connect_enable(&db_h->ipc_h);
      if ( ret != 0 )
        return ret;
    }
  DBGL(DBGL_API,"kogmo_rtdb_connect() done.");
  return connoid;
}

int
kogmo_rtdb_disconnect (kogmo_rtdb_handle_t *db_h, void *discinfo)
{
  int err;
  uint32_t flags = 0;
  DBGL(DBGL_API,"kogmo_rtdb_disconnect()");
  //if ( discinfo != NULL ); // unused
  //  flags = (uint32_t*)discinfo; // useless, should be in a struct later

  if (db_h == NULL || db_h->localdata_p == NULL)
      return -KOGMO_RTDB_ERR_NOCONN;

  CHK_DBH("kogmo_rtdb_disconnect",db_h,0);

  if (db_h->procobjmeta.oid != 0 && this_process_is_manager (db_h) )
    {
      kogmo_rtdb_kill_procs(db_h, 15);
      sleep(2);
      kogmo_rtdb_kill_procs(db_h, 9);
    }
  if ( db_h->procobjmeta.oid != 0 )
    {
      kogmo_rtdb_obj_info_t used_objmeta;
      kogmo_rtdb_objid_list_t objlist;
      int i, immediately_delete;
      kogmo_rtdb_objid_t err;

      DBGL (DBGL_IPC, "deleting all objects created by this process proc_oid %d",
                       db_h->procobj.process.proc_oid);

      //db_h->procobj.process.disconnected_ts = kogmo_rtdb_timestamp_now(db_h);
      //kogmo_rtdb_obj_writedata (db_h, db_h->procobjmeta.oid, &db_h->procobj);
      
      immediately_delete = db_h->procobj.process.flags & KOGMO_RTDB_CONNECT_FLAGS_IMMEDIATELYDELETE;

      err = kogmo_rtdb_obj_searchinfo_deleted (db_h, "", 0, 0,
                db_h->procobj.process.proc_oid, 0, objlist, 0);
      DBG("process has %i objects to%s delete",err,immediately_delete ? " immediately":"");
      if (err < 0 )
        return err;

      for (i=0; objlist[i] != 0; i++ )
        {
          err = kogmo_rtdb_obj_readinfo (db_h, objlist[i], 0, &used_objmeta );
          if ( err >= 0 && used_objmeta.flags.persistent )
            {
              DBG("keeping persistent object '%s'",used_objmeta.name);
              // TODO: Change owner or keep process-object?
              continue;
            }
          else
            {
              used_objmeta.oid = objlist[i]; // enough for delete
            }
           kogmo_rtdb_obj_delete_imm(db_h, &used_objmeta, immediately_delete);
        }

      DBGL (DBGL_IPC, "delete process object");
      kogmo_rtdb_obj_delete_imm (db_h, &db_h->procobjmeta, immediately_delete);
      db_h->procobjmeta.oid = 0;

      // hmm, db_h->localdata_p->rtdb_tracebufsize not accessible after shm unmap
      if ( db_h->localdata_p->rtdb_tracebufsize )
        kogmo_rtdb_ipc_mq_destroy(db_h->ipc_h.tracefd, db_h->ipc_h.shmid, this_process_is_manager (db_h) ? 1:0);
      if ( db_h->ipc_h.rxtracefd ) db_h->localdata_p->rtdb_trace = 0;
    }


  if (this_process_is_manager (db_h) )
    kogmo_rtdb_obj_local_destroy(db_h);

  err = kogmo_rtdb_ipc_disconnect (&db_h->ipc_h, flags);
  // free(db_h); - the exit-handler still depends on it
  DBGL(DBGL_API,"kogmo_rtdb_disconnect() done.");
  return err;
}

int
kogmo_rtdb_setstatus (kogmo_rtdb_handle_t *db_h, uint32_t status, _const char* msg, uint32_t flags)
{
  int err;
  CHK_DBH("kogmo_rtdb_setstatus",db_h,0);
  if(msg) strncpy(db_h->procobj.process.statusmsg, msg, KOGMO_RTDB_PROCSTATUS_MSG_MAXLEN);
  db_h->procobj.process.status = status;
  db_h->procobj.base.data_ts = kogmo_rtdb_timestamp_now (db_h);
  err = kogmo_rtdb_obj_writedata (db_h, db_h->procobjmeta.oid, &db_h->procobj);
  return err;
}

int
kogmo_rtdb_cycle_done (kogmo_rtdb_handle_t *db_h, uint32_t flags)
{
  int err;
  CHK_DBH("kogmo_rtdb_cycle_done",db_h,0);
  db_h->procobj.process.status = KOGMO_RTDB_PROCSTATUS_CYCLEDONE;
  err = kogmo_rtdb_obj_writedata (db_h, db_h->procobjmeta.oid, &db_h->procobj);
  return err;
}


kogmo_timestamp_t
kogmo_rtdb_timestamp_now (kogmo_rtdb_handle_t *db_h)
{
  CHK_DBH("kogmo_rtdb_timestamp_now",db_h,0);
  if ( db_h->localdata_p->flags.simmode && db_h->localdata_p->sim_ts )
    return db_h->localdata_p->sim_ts;
  return kogmo_timestamp_now();
}

int
kogmo_rtdb_timestamp_set (kogmo_rtdb_handle_t *db_h,
                          kogmo_timestamp_t new_ts)
{
  CHK_DBH("kogmo_rtdb_timestamp_now",db_h,0);
  // auch wenn simmode==0 tut's nicht weh:
  db_h->localdata_p->sim_ts = new_ts;
  if ( ! db_h->localdata_p->flags.simmode )
    return -KOGMO_RTDB_ERR_NOPERM;
  return 0;
}
