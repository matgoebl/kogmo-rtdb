/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file kogmo_rtdb_ipc_posix.c
 * \brief IPC-Functions to Access a local Real-time Vehicle Database
 * (this file handles the ugly truth - compatibility for several platforms)
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include "kogmo_rtdb_internal.h"

#ifdef KOGMO_RTDB_IPC_DO_POLLING
static int ipc_poll_mutex_usecs, ipc_poll_condvar_usecs;
#endif


void
kogmo_rtdb_require_revision (uint32_t req_rev)
{
  if ( req_rev != KOGMO_RTDB_REV ) {
     DIE("Wrong RTDB Library Revision! This program requires rev. %u,\n"
         "the libkogmo_rtdb in use has rev. %u.\n"
         "Please use the corresponding libkogmo_rtdb.so.\n"
         "Type 'ldd ./yourprogram' to find out which libraries you are using.\n",
         req_rev, KOGMO_RTDB_REV);
  }
}


static void
term_signal_handler (int signal)
{
  DBG ("kogmo_rtdb signal-handler: received signal %i", signal);
  exit (0);
}


static void ntfy_signal_handler(int signal)
{
  // just wakeup my sys-call
}


inline long int
roundup_pagesize (long long int sz, int extra_pages)
{
  long long int size =
    (int) ((sz + sysconf (_SC_PAGESIZE)) / sysconf (_SC_PAGESIZE) +
            extra_pages ) * sysconf (_SC_PAGESIZE);
  DBG ("roundup_pagesize(%lli,%i)->%lli", sz, extra_pages, size);
  return size;
}


kogmo_rtdb_objid_t
kogmo_rtdb_ipc_connect (struct kogmo_rtdb_ipc_handle_t *ipc_h,
                        char *dbhost,
                        char *procname, float cycletime, uint32_t flags,
                        char **data_p, long int *data_size,
                        void (*exit_handler) (int,void *), void *exit_arg)
{
  int err;
  int i;
  int is_manager=0,no_handlers=0,is_spectator,be_silent,is_realtime;

  strncpy(ipc_h->shmid,KOGMO_RTDB_SHMID,KOGMO_RTDB_PROC_NAME_MAXLEN);
  if (dbhost!=NULL && dbhost[0]!='\0')
    {
      strncat(ipc_h->shmid,".",KOGMO_RTDB_PROC_NAME_MAXLEN);
      strncat(ipc_h->shmid,dbhost,KOGMO_RTDB_PROC_NAME_MAXLEN);
    }

  ipc_h->shmfd=-1;
  ipc_h->shm_p=NULL;

  if ( procname[0] == '\0' ) {
    if ( flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
      return ( -KOGMO_RTDB_ERR_CONNDENY );
    else
      DIE("kogmo_rtdb_connect: own program name is empty or null");
  }

  ipc_h->this_process.proc_oid=0;
  ipc_h->this_process.connected_ts=kogmo_timestamp_now();
  ipc_h->this_process.cycletime = cycletime;
  ipc_h->this_process.pid=getpid();
  ipc_h->this_process.tid=pthread_self();

  strncpy(ipc_h->this_process.name,procname,sizeof(ipc_h->this_process.name));
  ipc_h->this_process.name[sizeof(ipc_h->this_process.name)-1]='\0';
  ipc_h->this_process.flags=flags;

  is_spectator=ipc_h->this_process.flags&KOGMO_RTDB_CONNECT_FLAGS_SPECTATOR?1:0;
  if (is_spectator)
   {
    ipc_h->this_process.flags&=~KOGMO_RTDB_CONNECT_FLAGS_MANAGER;
    ipc_h->this_process.flags&=~KOGMO_RTDB_CONNECT_FLAGS_REALTIME;
   }

  is_manager=ipc_h->this_process.flags&KOGMO_RTDB_CONNECT_FLAGS_MANAGER?1:0;
  no_handlers=ipc_h->this_process.flags&KOGMO_RTDB_CONNECT_FLAGS_NOHANDLERS?1:0;
  be_silent=ipc_h->this_process.flags&KOGMO_RTDB_CONNECT_FLAGS_SILENT?1:0;
  is_realtime=ipc_h->this_process.flags&KOGMO_RTDB_CONNECT_FLAGS_REALTIME?1:0;

  if( !be_silent )
    MSG("connecting to kognimobil vehicle realtime databasis "\
      "(%s|P%li|T%lu|F%x%s%s%s%s)...", ipc_h->this_process.name, (long int)ipc_h->this_process.pid, \
      (unsigned long int)ipc_h->this_process.tid, ipc_h->this_process.flags, \
      (is_manager?"|MGR":""), (no_handlers?"|NOH":""), \
      (is_spectator?"|SPEC":""), (DEBUG?"|DBG":"") );

#ifdef KOGMO_RTDB_IPC_DO_POLLING
  ipc_poll_mutex_usecs = getenv("KOGMO_RTDB_IPC_POLL_MUTEX_USECS") ?
                         atoi(getenv("KOGMO_RTDB_IPC_POLL_MUTEX_USECS")) :
                         KOGMO_RTDB_IPC_POLL_MUTEX_USECS_DEFAULT;
  ipc_poll_condvar_usecs = getenv("KOGMO_RTDB_IPC_POLL_CONDVAR_USECS") ?
                           atoi(getenv("KOGMO_RTDB_IPC_POLL_CONDVAR_USECS")) :
                           KOGMO_RTDB_IPC_POLL_CONDVAR_USECS_DEFAULT;
#endif

  // Setup Signal/Exit Handlers
  if ( getenv("KOGMO_RTDB_NOHANDLERS") )
    no_handlers = 1;

  if( !no_handlers )
    {
      int ok=1;
#ifndef MACOSX
      if( on_exit(exit_handler,exit_arg) != 0 ) ok=0;
#endif
      if( signal(SIGHUP, term_signal_handler) == SIG_ERR ) ok=0;
      if( signal(SIGINT, term_signal_handler) == SIG_ERR ) ok=0;
      if( signal(SIGQUIT, term_signal_handler) == SIG_ERR ) ok=0;
      if( signal(SIGTERM, term_signal_handler) == SIG_ERR ) ok=0;       

      if( signal(SIGILL, term_signal_handler) == SIG_ERR ) ok=0;
      if( signal(SIGBUS, term_signal_handler) == SIG_ERR ) ok=0;
      if( signal(SIGFPE, term_signal_handler) == SIG_ERR ) ok=0;
      if( signal(SIGSEGV, term_signal_handler) == SIG_ERR ) ok=0;

      if( signal(SIGPIPE, term_signal_handler) == SIG_ERR ) ok=0;
      if( signal(SIGCHLD, term_signal_handler) == SIG_ERR ) ok=0;

      if( signal(SIGUSR1, ntfy_signal_handler) == SIG_ERR ) ok=0;

      if ( !ok )
        {
          if ( ipc_h->this_process.flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
            return ( -KOGMO_RTDB_ERR_UNKNOWN );
          else
            DIE("kogmo_rtdb_connect: registering signal_handlers failed");
        }
    }


  // Setup Real-time Attributes
#ifdef KOGMO_RTDB_HARDREALTIME
  if (1)
#else
  if( is_realtime )
#endif
    {
      struct sched_param schedparam;
      if( mlockall(MCL_CURRENT|MCL_FUTURE) != 0 )
         DBG("kogmo_rtdb_connect: locking code in memory failed: %s",strerror(errno));

      if ( is_realtime )
        schedparam.sched_priority = 90;
      else if ( is_manager )
        schedparam.sched_priority = 50;
      else
        schedparam.sched_priority = 20;
      err=pthread_setschedparam( pthread_self(), SCHED_FIFO, &schedparam);
      if( err != 0 )
        DBG("kogmo_rtdb_connect: setting scheduler parameters failed: %s",strerror(err));
    }


  // Setup Shared Memory
  if(is_manager)
    {
      // Manager: Create New Shared Memory Segment
      shm_unlink(ipc_h->shmid);

      ipc_h->shmfd= shm_open(ipc_h->shmid,O_RDWR|O_CREAT|O_TRUNC,S_IRWXU);
      if( ipc_h->shmfd == -1 )
        {
          if ( ipc_h->this_process.flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
            return ( -KOGMO_RTDB_ERR_UNKNOWN );
          else
            DIE("kogmo_rtdb_connect: opening shared memory '%s' failed: %s",ipc_h->shmid,strerror(errno));
        }

        ipc_h->shm_size = sizeof(struct kogmo_rtdb_ipc_shm_t) + *data_size;
        DBG("kogmo_rtdb_connect: new shared memory size: %lli",(long long int)ipc_h->shm_size);
        err=ftruncate(ipc_h->shmfd, roundup_pagesize(ipc_h->shm_size, 1 /*1 extra page against XENO size-bug*/ ));
        if( err == -1 )
          {
            if ( ipc_h->this_process.flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
              return ( -KOGMO_RTDB_ERR_UNKNOWN );
            else
              DIE("kogmo_rtdb_connect: setting shared memory size failed: %s",strerror(errno));
        }
    }
  else
    {
      // Non-Manager: Connect to Existing Shared Memory Segment or *Wait*
      ipc_h->shmfd=-1;
      int connection_ok=0;

      while(!connection_ok)
        {
          struct stat shm_stat;

          ipc_h->shmfd= shm_open(ipc_h->shmid,is_spectator?O_RDONLY:O_RDWR,S_IRWXU|S_IRWXG|S_IRWXO);
          if( ipc_h->shmfd == -1 && errno == ENOENT)
            {
              if( !be_silent )
                MSGN("shared memory not found (retrying...)\r");
              if(ipc_h->this_process.flags&KOGMO_RTDB_CONNECT_FLAGS_TRYNOWAIT
                  && !(ipc_h->this_process.flags&KOGMO_RTDB_CONNECT_FLAGS_SILENT))
                ERR("shared memory not found");
              goto again;
            }

          if( ipc_h->shmfd == -1 )
            {
              ERR("opening shared memory failed: %s (retrying...)",strerror(errno));
              goto again;
            }

#if !defined(MACOSX) && !defined(KOGMO_RTDB_HARDREALTIME)
          err= fstat(ipc_h->shmfd, &shm_stat);
          if (err==-1)
            {
              ERR("getting shared memory permissions failed: %s",strerror(errno));
              goto again;
            }

          if( (shm_stat.st_mode & S_IRGRP) == 0 )
            {
              if( !be_silent )
                MSGN("shared memory not yet initialized (retrying...)\n");
              goto again;
            }
#endif

          connection_ok=1;
          break;

         again:
          if(ipc_h->shmfd != -1) {
           err=close(ipc_h->shmfd);
           if( err != 0 )
            ERR("closing shared memory failed: %s",strerror(errno));
          }
          if(ipc_h->this_process.flags&KOGMO_RTDB_CONNECT_FLAGS_TRYNOWAIT) {
           return (-KOGMO_RTDB_ERR_CONNDENY);
          }
          sleep(1);
          MSGN("retrying in a second..                             \r");
          sleep(1);
          MSGN("                                                   \r");

        }

      // initial mapping to read real size
      ipc_h->shm_size = sizeof(struct kogmo_rtdb_ipc_shm_t);
      ipc_h->shm_p=mmap(NULL,roundup_pagesize(ipc_h->shm_size,0), PROT_READ, MAP_SHARED, ipc_h->shmfd, 0);
      if( ipc_h->shm_p == MAP_FAILED)
        {
          if ( ipc_h->this_process.flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
            return ( -KOGMO_RTDB_ERR_UNKNOWN );
          else
            DIE("kogmo_rtdb_connect: initial mapping shared memory failed: %s",strerror(errno));
      }

      while(!ipc_h->shm_p->manager_alive_ts)
        {
          if( !be_silent )
            MSGN("shared memory not yet initialized (retrying...)\n");
          sleep(1);
        }

      if ( ipc_h->shm_p->revision != KOGMO_RTDB_REV )
        {
          if ( ipc_h->this_process.flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
            return ( -KOGMO_RTDB_ERR_UNKNOWN );
          else
            DIE("Connecting to running RTDB with wrong Revision!\n"
                "Running RTDB: rev. %u, but this Process uses libkogmo_rtdb rev. %u\n"
                "Please use the corresponding libkogmo_rtdb.so or try restarting kogmo_rtdb_man.\n"
                "Type 'ldd ./yourprogram' to find out which libraries you are using.\n",
                ipc_h->shm_p->revision, KOGMO_RTDB_REV);
        }

      *data_size = ipc_h->shm_p->data_size;

      err=munmap(ipc_h->shm_p,roundup_pagesize(ipc_h->shm_size,0));
      if( err != 0 )
        ERR("unmapping initial shared memory: %s",strerror(errno));

      // ready for re-map with correct size
      ipc_h->shm_size = sizeof(struct kogmo_rtdb_ipc_shm_t) + *data_size;
    }


  ipc_h->shm_p=mmap(NULL,roundup_pagesize(ipc_h->shm_size,0), is_spectator?PROT_READ:PROT_READ|PROT_WRITE, MAP_SHARED, ipc_h->shmfd, 0);
  if( ipc_h->shm_p == MAP_FAILED)
    {
      if ( ipc_h->this_process.flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
        return ( -KOGMO_RTDB_ERR_UNKNOWN );
      else
        DIE("kogmo_rtdb_connect: mapping shared memory failed: %s",strerror(errno));
    }

  DBGL(DBGL_IPC+DBGL_VERBOSE, "kogmo_rtdb_connect: localdata has %li bytes at %p", *data_size,ipc_h->shm_p);

  *data_p = (char*)ipc_h->shm_p + sizeof(struct kogmo_rtdb_ipc_shm_t);

  if ( !is_manager )
    {
      float alive_diff = kogmo_timestamp_diff_secs (
                           ipc_h->shm_p->manager_alive_ts,
                           kogmo_timestamp_now());
      if ( alive_diff > KOGMO_RTDB_MANAGER_ALIVE_INTERVAL*3 )
        {
          if ( ipc_h->this_process.flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
            return ( -KOGMO_RTDB_ERR_UNKNOWN );
          else
            DIE("manager is dead since %.3f seconds", alive_diff);
        }
      DBGL(DBGL_IPC+DBGL_VERBOSE, "manager last alive %.3f seconds ago", alive_diff);
    }

  if( !be_silent)
    MSG("kogmo_rtdb_connect: connection established (SHM at %p)", ipc_h->shm_p);

  if(is_manager)
    {
      memset(ipc_h->shm_p, 0, ipc_h->shm_size);
      ipc_h->shm_p->data_size = *data_size;
      ipc_h->shm_p->manager_pid = getpid();
      ipc_h->shm_p->manager_alive_ts = 0; // manager at initialization
      kogmo_rtdb_ipc_mutex_init (&ipc_h->shm_p->global_lock);
      // proc[]=[0..0] initialized by memset(..,0,..)
      kogmo_rtdb_ipc_mutex_init (&ipc_h->shm_p->proc_lock);
      ipc_h->shm_p->proc_free=KOGMO_RTDB_PROC_MAX;
      ipc_h->this_process.proc_oid = 1;
      ipc_h->shm_p->proc_oid_next = 2;
      ipc_h->shm_p->revision = KOGMO_RTDB_REV;
    }
  else
    {
      // find free process slot and enter client data
      struct kogmo_rtdb_ipc_process_t *scan_process_p=NULL;
      
      kogmo_rtdb_ipc_mutex_lock (&ipc_h->shm_p->proc_lock);
      i=-1;
      do
        {
          if ( ++i >= KOGMO_RTDB_PROC_MAX)
            {
              kogmo_rtdb_ipc_mutex_unlock (&ipc_h->shm_p->proc_lock);
              if ( ipc_h->this_process.flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
                return ( -KOGMO_RTDB_ERR_OUTOFOBJ );
              else
                DIE("cannot find free process slot");
             }

          scan_process_p = &ipc_h->shm_p->proc[i];
        }
      while ( scan_process_p->proc_oid != 0);

      ipc_h->this_process_slot = i;

      // get new proc_oid
      ipc_h->this_process.proc_oid = ipc_h->shm_p->proc_oid_next++;
      ipc_h->shm_p->proc_free--;
      memcpy(scan_process_p,&ipc_h->this_process,sizeof(struct kogmo_rtdb_ipc_process_t));
      kogmo_rtdb_ipc_mutex_unlock (&ipc_h->shm_p->proc_lock);
      DBGL(DBGL_IPC,"process got proc_oid %lli and slot %d",
                    (long long int) ipc_h->this_process.proc_oid, i);
    }

  return (ipc_h->this_process.proc_oid);
}





int
kogmo_rtdb_ipc_connect_enable (struct kogmo_rtdb_ipc_handle_t *ipc_h)
{
  int err;
  // unfortunately this doesn't work in rt-mode
  // => workaround is manager_alive_ts = 0 "manager at initialization"
  //  => possible race condition at startup when reading shm size :-(
#if defined(MACOSX) || defined(KOGMO_RTDB_HARDREALTIME)
  return(0);
#endif
  err= fchmod(ipc_h->shmfd, S_IRWXU|S_IRWXG|S_IRWXO);
  if (err==-1)
    {
      if ( ipc_h->this_process.flags & KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR )
        return ( -KOGMO_RTDB_ERR_UNKNOWN );
      else
        DIE("kogmo_rtdb_connect: setting shared memory permissions failed: %s",strerror(errno));
  }
  return(0);
}


int
kogmo_rtdb_ipc_disconnect (struct kogmo_rtdb_ipc_handle_t *ipc_h,
                           uint32_t flags)
{
 int err;
 int is_manager=ipc_h->this_process.flags&KOGMO_RTDB_CONNECT_FLAGS_MANAGER?1:0;
// int is_spectator=this_process.flags&KOGMO_RTDB_CONNECT_FLAGS_SPECTATOR?1:0;
 int be_silent=ipc_h->this_process.flags&KOGMO_RTDB_CONNECT_FLAGS_SILENT;


 if(!ipc_h->shm_p) {
  DBG("cannot disconnect: not connected");
  return(0);
 }

 if( !be_silent )
  MSG("disconnecting from kognimobil vehicle realtime databasis "\
      "(%s|P%li|T%lu|F%x)... ", ipc_h->this_process.name, (long int)ipc_h->this_process.pid, \
      (unsigned long int)ipc_h->this_process.tid, ipc_h->this_process.flags);

 if(is_manager) {
  //kogmo_rtdb_obj_local_destroy();
  //  for(i=0;i<KOGMO_RTDB_OBJ_LOCAL_SEM_SIZE;i++)
  //    kogmo_rtdb_mutex_destroy(&ipc_h->shm_p->objdata_lock[i]);
  kogmo_rtdb_ipc_mutex_destroy (&ipc_h->shm_p->proc_lock);
  kogmo_rtdb_ipc_mutex_destroy (&ipc_h->shm_p->global_lock);
 } else {
  if(ipc_h->this_process.proc_oid)
    {
      struct kogmo_rtdb_ipc_process_t *used_process_p=NULL;
      int i=0;

      kogmo_rtdb_ipc_mutex_lock (&ipc_h->shm_p->proc_lock);
      for(i=0;i<KOGMO_RTDB_PROC_MAX;i++)
        {
          if(ipc_h->shm_p->proc[i].proc_oid == ipc_h->this_process.proc_oid)
            {
              DBG("process: freed process slot %i with OID %i",i,ipc_h->shm_p->proc[i].proc_oid);
              used_process_p=&(ipc_h->shm_p->proc[i]);
              used_process_p->disconnected_ts = kogmo_timestamp_now(); //unnecessary
              // till now, there are no permanent processes
              memset(used_process_p,0,sizeof(struct kogmo_rtdb_ipc_process_t));
              ipc_h->shm_p->proc_free++;
            }
        }
      kogmo_rtdb_ipc_mutex_unlock (&ipc_h->shm_p->proc_lock);

    }
 }


 // Remove Shared Memory

 err=munmap(ipc_h->shm_p,roundup_pagesize(ipc_h->shm_size,0));
 if( err != 0 )
  ERR("unmapping shared memory: %s",strerror(errno));
 err=close(ipc_h->shmfd);
 if( err != 0 )
  ERR("closing shared memory failed: %s",strerror(errno));
 if(is_manager) {
  err=shm_unlink(ipc_h->shmid);
  if( err != 0 )
   ERR("unlinking shared memory: %s",strerror(errno));
 }

 ipc_h->shm_p=NULL;

 if( !be_silent )
  MSG("disconnected");

 return(0);
}





#ifndef KOGMO_RTDB_IPC_DO_POLLING /* !KOGMO_RTDB_IPC_DO_POLLING => HARD REALTIME :-) */

/// Setup Mutex
int
kogmo_rtdb_ipc_mutex_init(pthread_mutex_t *mutex)
{
  int err;
  pthread_mutexattr_t mattr;

  err=pthread_mutexattr_init(&mattr);
  if( err != 0 )
    DIE("initializing mutex attributes failed: %s",strerror(err));

  err=pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_ERRORCHECK);
  if( err != 0 )
    ERR("setting mutex attribute ERRORCHECK failed: %s",strerror(err));

#ifdef KOGMO_RTDB_HARDREALTIME
  err=pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);
  if( err != 0 )
    ERR("setting mutex attribute PRIO_INHERIT failed: %s",strerror(err));
#endif

#ifndef MACOSX
  err=pthread_mutexattr_setpshared(&mattr,PTHREAD_PROCESS_SHARED);
  if( err != 0 )
    ERR("setting mutex attribute PROCESS_SHARED failed: %s",strerror(err));
#endif

  err=pthread_mutex_init(mutex,&mattr);
  if( err != 0 )
    DIE("initializing mutex failed: %s",strerror(err));

  err=pthread_mutexattr_destroy(&mattr);
  if( err != 0 )
    DIE("destroying mutex attributes failed: %s",strerror(err));
  return(0);
}

/// Destroy Mutex
int
kogmo_rtdb_ipc_mutex_destroy(pthread_mutex_t *mutex)
{
  int err;
  // Remove Mutex
  err=pthread_mutex_destroy(mutex);
  // ignore this so far, EBUSY, well-known-bug
  if( err != 0 )
    DBG("removing mutex failed: %s",strerror(err));
 return err;
}

int
kogmo_rtdb_ipc_mutex_lock(pthread_mutex_t *mutex)
{
  int err;
  /* if (DO_POLL) do {
     err=pthread_mutex_trylock(mutex);
     if(err==EBUSY) usleep(5000); // do polling every 5ms
   } while(err==EBUSY);
  else
  */
    err=pthread_mutex_lock(mutex);

  if (err == EDEADLK )
    {
      DBG("mutex lock: deadlock avoided (happens on process cleanup).");
      return 0;
    }

  if( err != 0 )
    DIE("locking mutex failed: %s",strerror(err));

  return err;
}

int
kogmo_rtdb_ipc_mutex_unlock(pthread_mutex_t *mutex)
{
  int err;
  err=pthread_mutex_unlock(mutex);
  if (err == EPERM )
    {
      DBG("mutex unlock: permission denied (happens on timeout).");
      return 0;
    }
  if( err != 0 )
    DIE("unlocking mutex failed: %s",strerror(err));
  return err;
}



/// Setup Condition Variable
int
kogmo_rtdb_ipc_condvar_init(pthread_cond_t *condvar)
{
  int err;
  pthread_condattr_t cattr;

  err=pthread_condattr_init(&cattr);
  if( err != 0 )
    DIE("initializing condition variable attributes failed: %s",strerror(err));

#ifndef MACOSX
  err=pthread_condattr_setpshared(&cattr,PTHREAD_PROCESS_SHARED);
  if( err != 0 )
    DIE("setting condition variable attributes PROCESS_SHARED failed: %s",strerror(err));
#endif

  err=pthread_cond_init(condvar,&cattr);
  if( err != 0 )
    DIE("initializing condition variable failed: %s",strerror(err));

  err=pthread_condattr_destroy(&cattr);
  if( err != 0 )
    DIE("destroying condition variable attributes failed: %s",strerror(err));

  return(0);
}

/// Destroy it
int
kogmo_rtdb_ipc_condvar_destroy(pthread_cond_t *condvar)
{
  int err;
  err=pthread_cond_destroy(condvar);
  return err;
}



int
kogmo_rtdb_ipc_condvar_wait(pthread_cond_t *condvar, pthread_mutex_t *mutex, kogmo_timestamp_t wakeup_ts)
{
  int err;
  struct timespec ats;
  ats.tv_nsec = ( wakeup_ts * KOGMO_TIMESTAMP_NANOSECONDSPERTICK ) % 1000000000;
  ats.tv_sec  =   wakeup_ts / KOGMO_TIMESTAMP_TICKSPERSECOND;

  DBGL(DBGL_IPC,"before pthread_cond_(timed)wait(,%lli)", (long long int)wakeup_ts);
  if (wakeup_ts)
    err=pthread_cond_timedwait(condvar, mutex, &ats);
  else
    err=pthread_cond_wait(condvar,mutex);
  DBGL(DBGL_IPC,"after pthread_cond_timedwait()");

  if( err != 0 && err != ETIMEDOUT )
    DIE("waiting on condition variable failed: %s(%i)",strerror(err),err);

  if ( err == ETIMEDOUT )
    return -KOGMO_RTDB_ERR_TIMEOUT;

  return err;
}

int
kogmo_rtdb_ipc_condvar_signalall(pthread_cond_t *condvar)
{
  int err;

  DBGL(DBGL_IPC,"before pthread_cond_broadcast()");
  err=pthread_cond_broadcast(condvar);
  DBGL(DBGL_IPC,"after pthread_cond_broadcast()");

  if( err != 0 )
    DIE("signaling condition variable failed: %s",strerror(err));

  return err;
}



#else /* KOGMO_RTDB_IPC_DO_POLLING !!! WARNING: THIS IS AN EMULATION WITH POLLING -> NO NO NO REALTIME !!! */

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
// atomic_cas32() from libboost (boost/interprocess/detail/atomic.hpp):
inline uint32_t atomic_cas32
   (volatile uint32_t *mem, uint32_t with, uint32_t cmp)
{
   uint32_t prev = cmp;
   asm volatile( "lock\n\t"
                 "cmpxchg %3,%1"
               : "=a" (prev), "=m" (*(mem))
               : "0" (prev), "r" (with)
               : "memory", "cc");
   return prev;
}

#elif defined(__GNUC__) && defined(__mips__)
// from linux-2.6.30.4/arch/mips/include/asm/cmpxchg.h: (only for cpu_has_llsc && !R10000_LLSC_WAR)
#define __cmpxchg_asm(ld, st, m, old, new)                              \
({                                                                      \
        __typeof(*(m)) __ret;                                           \
                __asm__ __volatile__(                                   \
                "       .set    push                            \n"     \
                "       .set    noat                            \n"     \
                "       .set    mips3                           \n"     \
                "1:     " ld "  %0, %2          # __cmpxchg_asm \n"     \
                "       bne     %0, %z3, 2f                     \n"     \
                "       .set    mips0                           \n"     \
                "       move    $1, %z4                         \n"     \
                "       .set    mips3                           \n"     \
                "       " st "  $1, %1                          \n"     \
                "       beqz    $1, 3f                          \n"     \
                "2:                                             \n"     \
                "       .subsection 2                           \n"     \
                "3:     b       1b                              \n"     \
                "       .previous                               \n"     \
                "       .set    pop                             \n"     \
                : "=&r" (__ret), "=R" (*m)                              \
                : "R" (*m), "Jr" (old), "Jr" (new)                      \
                : "memory");                                            \
        __ret;                                                          \
})

extern void __cmpxchg_called_with_bad_pointer(void);
inline uint32_t atomic_cas32
   (volatile uint32_t *mem, uint32_t with, uint32_t cmp)
{
   uint32_t prev = cmp;
   prev = __cmpxchg_asm("ll", "sc", mem, cmp, with);
   return prev;
}
#endif

static inline void xusleep(int udelay)
{
  struct timespec interval;
  interval.tv_sec = (int)udelay/1000000;
  interval.tv_nsec = (int)(udelay*1000)%1000000000;
  nanosleep(&interval, NULL);
}


int
kogmo_rtdb_ipc_mutex_init(pthread_mutex_t *mutex)
{
  *(uint32_t*)mutex = 0;
  return(0);
}

int
kogmo_rtdb_ipc_mutex_destroy(pthread_mutex_t *mutex)
{
 return 0;
}

// sizeof(pthread_mutex_t) >> sizeof(uint32_t) ..
int
kogmo_rtdb_ipc_mutex_lock(pthread_mutex_t *mutex)
{
  uint32_t prev;
  while(1)
   {
    prev = atomic_cas32((uint32_t*)mutex,1,0);
    if ( *(uint32_t*)mutex == 1 && prev == 0 )
      break;
    usleep(ipc_poll_mutex_usecs);
   }
  return 0;
}

int
kogmo_rtdb_ipc_mutex_unlock(pthread_mutex_t *mutex)
{
  atomic_cas32((uint32_t*)mutex,0,1);
  return 0;
}

int
kogmo_rtdb_ipc_condvar_init(pthread_cond_t *condvar)
{
  return 0;
}

int
kogmo_rtdb_ipc_condvar_destroy(pthread_cond_t *condvar)
{
  return 0;
}

// this is unused since we set KOGMO_RTDB_CONNECT_FLAGS_DO_POLLING with KOGMO_RTDB_IPC_DO_POLLING
int
kogmo_rtdb_ipc_condvar_wait(pthread_cond_t *condvar, pthread_mutex_t *mutex, kogmo_timestamp_t wakeup_ts)
{
  kogmo_rtdb_ipc_mutex_unlock(mutex);
  // wakeup_ts ignored in this work-around!
  usleep(ipc_poll_condvar_usecs);
  kogmo_rtdb_ipc_mutex_lock(mutex);
  return 0;
}

int
kogmo_rtdb_ipc_condvar_signalall(pthread_cond_t *condvar)
{
  return 0;
}

#endif /* KOGMO_RTDB_IPC_DO_POLLING */






#ifndef KOGMO_RTDB_IPC_NO_PTHREADFUNCS

int
kogmo_rtdb_pthread_create(kogmo_rtdb_handle_t *db_h,
                          pthread_t *thread,
                          pthread_attr_t *attr,
                          void *(*start_routine)(void*), void *arg)
{
  int ret;
  struct sched_param schedparam;
  pthread_attr_t myat;
  pthread_attr_t *at;
  if (attr==NULL) 
    {
      at=&myat;
      ret = pthread_attr_init (at);
    }
  else
    {
      at=attr;
    }
  ret = pthread_attr_setschedpolicy(at, SCHED_FIFO);
  schedparam.sched_priority = 15;
  ret = pthread_attr_setschedparam (at, &schedparam);
  if( mlockall(MCL_CURRENT|MCL_FUTURE) != 0 )
   ERR("locking code in memory failed: %s",strerror(errno));
  ret = pthread_create(thread, at, start_routine, arg);
  return ret;
}

int
kogmo_rtdb_pthread_kill(kogmo_rtdb_handle_t *db_h,
                        pthread_t thread, int sig)
{
  return pthread_kill(thread,sig);
}

int
kogmo_rtdb_pthread_join(kogmo_rtdb_handle_t *db_h,
                        pthread_t thread, void **value_ptr)
{
  return pthread_join(thread, value_ptr);
}

#endif /* ifndef KOGMO_RTDB_IPC_NO_PTHREADFUNCS */


#ifndef KOGMO_RTDB_IPC_NO_MQUEUES

/// Handle Message Queues
int kogmo_rtdb_ipc_mq_init(mqd_t *mqfd, char *name, int do_init, int size, int len) {
 int err;
 if ( do_init )
   {
     struct mq_attr attr;
     attr.mq_maxmsg = len;
     attr.mq_msgsize = size;
     attr.mq_flags = 0;
     err = mq_unlink (name); // may fail
     *mqfd = mq_open(name, O_WRONLY|O_CREAT|O_EXCL|O_NONBLOCK, 0666, &attr);
     if ( *mqfd == -1 ) return -1; //xDIE("mq_open and init failed");
   }
 else
   {
     *mqfd = mq_open(name, O_WRONLY|O_NONBLOCK);
     if ( *mqfd == -1 ) DIE("mq_open failed");
   }
 return *mqfd;
}

int kogmo_rtdb_ipc_mq_destroy(mqd_t mqfd, char *name, int do_destroy) {
 int err;
 err = mq_close(mqfd);
 if ( err == -1 ) DBG("mq_close failed");
 if ( do_destroy )
   {
     err = mq_unlink (name);
     if ( err == -1 ) DBG("mq_unlink failed");
   }
 return err;
}

int
kogmo_rtdb_ipc_mq_send(mqd_t mqfd, void *msg, int size)
{
  int err;
  err = mq_send(mqfd,msg,size,0);
  //NEVER -> if ( err == -1 ) DIE("mq_send failed");
  //-> tracer notices lost messages by itself
  return err;
}

int
kogmo_rtdb_ipc_mq_receive_init(mqd_t *mqfd, char *name)
{
  *mqfd = mq_open(name, O_RDONLY);
  if ( *mqfd == -1 )
    DIE("mq_open failed");
  return *mqfd;
}
int
kogmo_rtdb_ipc_mq_receive(mqd_t mqfd, void *msg, int size)
{
  int err;
  unsigned priop=0;
  err = mq_receive(mqfd, msg, size, &priop);
  if ( err == -1 )
    DIE("mq_receive failed");
  return err;
}
int
kogmo_rtdb_ipc_mq_curmsgs(mqd_t mqfd)
{
  int err;
  struct mq_attr attr;
  err = mq_getattr(mqfd, &attr);
  if ( err == -1 )
    {
      ERR("mq_getattr failed");
      return err;
    }
  return attr.mq_curmsgs;
}

#else /* KOGMO_RTDB_IPC_NO_MQUEUES */

/// No Message Queues
int kogmo_rtdb_ipc_mq_init(mqd_t *mqfd, char *name, int do_init, int size, int len) {
 if ( do_init )
   {
     DBG("Error: No message queues on this platform - recording not possibe");
   }
 return -1;
}

int kogmo_rtdb_ipc_mq_destroy(mqd_t mqfd, char *name, int do_destroy) {
 return -1;
}

int
kogmo_rtdb_ipc_mq_send(mqd_t mqfd, void *msg, int size)
{
  return -1;
}

int
kogmo_rtdb_ipc_mq_receive_init(mqd_t *mqfd, char *name)
{
  return -1;
}

int
kogmo_rtdb_ipc_mq_receive(mqd_t mqfd, void *msg, int size)
{
  return -1;
}

int
kogmo_rtdb_ipc_mq_curmsgs(mqd_t mqfd)
{
  return -1;
}

#endif /* KOGMO_RTDB_IPC_NO_MQUEUES */



int
kogmo_rtdb_sleep_until(kogmo_rtdb_handle_t *db_h,
                       kogmo_timestamp_t wakeup_ts)
{
  struct timespec time_wait,time_rem;
  kogmo_timestamp_t ts;
  int ret=0;
  CHK_DBH("kogmo_rtdb_sleep_until",db_h,0);
  ts = wakeup_ts - kogmo_rtdb_timestamp_now (db_h);
  DBGL(DBGL_API,"API: kogmo_rtdb_sleep_until(%lli nsecs)",(long long int)ts);
  if ( ts < 0 )
    {
      DBG("kogmo_rtdb_sleep_until() tried to sleep <0 seconds");
      return 0;
    }
  while ( ts > 0 )
    {
      time_rem.tv_nsec = ( ts * KOGMO_TIMESTAMP_NANOSECONDSPERTICK ) % 1000000000;
      // time_rem.tv_sec = ts / KOGMO_TIMESTAMP_TICKSPERSECOND;
      // limit a single sleep to <2 seconds (time can jump in simulation mode and when playing recordings)
      time_rem.tv_sec = ts / KOGMO_TIMESTAMP_TICKSPERSECOND >= 1 ? 1 : 0;
      do
        {
          time_wait.tv_nsec = time_rem.tv_nsec;
          time_wait.tv_sec = time_rem.tv_sec;
          ret = nanosleep (&time_wait, &time_rem);
        }
      while(ret==-1 && errno == EINTR);
      ts = wakeup_ts - kogmo_rtdb_timestamp_now (db_h);
      if ( ts > 0 )
        DBG("kogmo_rtdb_sleep_until() sleeping %lli nsecs more (simulation mode or more than 1 second requested)", (long long int)ts);
    }
  return ret;
}




