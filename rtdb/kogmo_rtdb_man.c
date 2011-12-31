/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
#include <stdlib.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>


#include "kogmo_rtdb_internal.h"
#include "kogmo_rtdb_ipc_posix.h"
#include "kogmo_rtdb_obj_local.h"

#define PROGNAME "c3_rtdb_manager"

void
usage (void)
{
  printf(
"Usage: kogmo_rtdb_man [.....]\n"
" -n        no-daemon: don't fork into background\n"
" -d        debugging: show debugging information, implies -n\n"
#ifndef KOGMO_RTDB_HARDREALTIME
" -s        start in simulation mode (arbitrary commit-timestamps and time)\n"
#endif
" -S SIZE   create database with the given size, defaults to %d bytes,\n"
"           overrides the environment variable KOGMO_RTDB_HEAPSIZE\n"
" -H DBHOST create database with the given name, must begin with 'local:',\n"
"           eg. 'local:bla'. defaults to '%s', overrides the \n"
"           environment variable KOGMO_RTDB_DBHOST\n"
//" -I INTERVAL[,MAXSIZE]  force minimum history interval\n"
//"           (only for objects with a maximum size of MAXSIZE bytes.)\n"
" -k        kill old database (specified by -H or KOGMO_RTDB_DBHOST) and exit\n"
" -h        print this help message\n\n",
KOGMO_RTDB_DEFAULT_HEAP_SIZE,
KOGMO_RTDB_DEFAULT_DBHOST);
  exit(1);
}

pthread_t procchk_tid=0;
void * procchk_thread (void *dummy);
kogmo_rtdb_handle_t *dbc=NULL;
kogmo_timestamp_t procchk_alive_ts = 0;

int main(int argc, char *argv[]) {
 int err;
 pid_t old_manager_pid,pid;
 int null_fd=-1;
 int daemon=1,justkill=0,simmode=0,polling=0;
 int opt;
 int i;
 kogmo_rtdb_objid_t oret;
 kogmo_rtdb_connect_info_t dbinfo;
 int use_procchk_thread = 0;
 kogmo_timestamp_t cycle_ts = 0, ts_now;

 printf (KOGMO_RTDB_COPYRIGHT
         "Release %i%s (%s)\n", KOGMO_RTDB_REV,KOGMO_RTDB_REVSPEC, KOGMO_RTDB_DATE);

 kogmo_rtdb_require_revision(KOGMO_RTDB_REV);

 while( ( opt = getopt (argc, argv, "ndsPDS:H:kI:h") ) != -1 )
  switch(opt)
   {
    case 'n': daemon = 0; break;
    case 'd': daemon = 0; kogmo_rtdb_debug|=DBGL_APP; break;
#ifndef KOGMO_RTDB_HARDREALTIME
    case 's': simmode = 1; break;
    case 'P': polling = 1; break;
#else
    case 'P': printf("Error: forced polling is only available in the non-real-time version!\n\n"); usage(); break;
    case 's': printf("Error: simulation mode is only available in the non-real-time version!\n\n"); usage(); break;
#endif
    case 'D': daemon = 0; kogmo_rtdb_debug = DBGL_MAX; break;
    case 'S': setenv("KOGMO_RTDB_HEAPSIZE",optarg,1); break;
    case 'H': setenv("KOGMO_RTDB_DBHOST",optarg,1); break;
    case 'I': setenv("KOGMO_RTDB_MINHIST",optarg,1); break;
    case 'k': justkill = 1; break;
    case 'h':
    default: usage(); break;
   }

#ifdef KOGMO_RTDB_IPC_DO_POLLING
 polling = 1;
#endif

 DBG("manager handle is at %p and points to %p",&dbc,dbc);

 kogmo_rtdb_connect_initinfo(&dbinfo, "", PROGNAME, 1.0);
 dbinfo.flags = KOGMO_RTDB_CONNECT_FLAGS_NOHANDLERS |
                KOGMO_RTDB_CONNECT_FLAGS_TRYNOWAIT |
                KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR |
                KOGMO_RTDB_CONNECT_FLAGS_SILENT;


 if( justkill ) {
  printf("\nI will try to kill the database by sending the manager a signal.\n");
  printf("The manager will then terminate all its clients (SIGTERM, 2 seconds, SIGKILL).\n");
  printf("If this fails, a stale database might hang around.\n");
  printf("To completely remove your database:\n");
  printf("- Look for processes that are still connected:\n");
  printf("  Enter '/usr/sbin/lsof | grep /dev/shm/kogmo'\n");
  printf("  (/dev/shm/kogmo.matthias.goebl = database local:matthias.goebl)\n");
  printf("- Kill them (this includes the correspondig manager process)\n");
  printf("- Remove the shared memory segment ('rm /dev/shm/kogmo.matthias.goebl')\n");
  printf("  Only removing the file does neither frees the memory nor kills the processes!\n");
  printf("\n");
  oret = kogmo_rtdb_connect(&dbc, &dbinfo);
  if ( oret >= 0 ) {
   old_manager_pid= dbc->ipc_h.shm_p->manager_pid;
   kogmo_rtdb_disconnect(dbc, NULL);
   printf("OK: Found old database, sending the manager (pid %d) the termination signal..\n",old_manager_pid);
   err=kill(old_manager_pid,SIGTERM);
   if( err != 0 ) {
    printf("ERROR: Killing old manager at pid %i failed - database was stale.\n",old_manager_pid);
    exit(1);
   } else {
    printf("OK: Signal delivered.\n");
    sleep(3);
    oret = kogmo_rtdb_connect(&dbc, &dbinfo);
    if ( oret < 0 ) {
     kogmo_rtdb_disconnect(dbc, NULL);
     printf("OK: Manager is terminated.\n");
     exit(0);
    } else {
     printf("ERROR: Database still alive.\n");
     exit(1);
    }
   }
  } else {
   printf("ERROR: Found no old database.\n");
   exit(1);
  }
  exit(0); // never reached
 }


 if( kogmo_rtdb_connect(&dbc, &dbinfo) >= 0 ) {
  kogmo_rtdb_disconnect(dbc, NULL);
  printf("\nERROR: Database already exists.\n");
  printf("Do not restart databases if it's not really necessary.\n");
  printf("If you restart a database, please make sure that you restart all clients!\n");
  printf("To restart it, first kill it with 'kogmo_rtdb_man -k' and then start it again.\n");
  exit(0);
 }


 // daemonize

 if( daemon ) {
  umask(0);
  chdir("/");
  null_fd=open("/dev/null", O_RDWR);
  if( null_fd == -1 )
   DIE("opening /dev/null failed: %s",strerror(errno));
  if( signal(SIGHUP,SIG_IGN) == SIG_ERR )
   DIE("cannot ignore SIGHUP");
  pid=fork();
  if( pid < 0 )
   DIE("cannot fork: %s",strerror(errno));
  if( pid !=0 ) {
   for(i=0;i<5;i++) {
    kogmo_rtdb_connect_initinfo(&dbinfo, "", "kogmo_rtdb_man_check", 1.0);
    dbinfo.flags = KOGMO_RTDB_CONNECT_FLAGS_NOHANDLERS |
                   KOGMO_RTDB_CONNECT_FLAGS_TRYNOWAIT |
                   KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR |
                   KOGMO_RTDB_CONNECT_FLAGS_SILENT;
    if( kogmo_rtdb_connect(&dbc, &dbinfo) > 0 ) {
     kogmo_rtdb_disconnect(dbc, NULL);
     printf("OK - connection to just started RTDB successful.\n");
     _exit(0);
    }
    printf("wait..\n");
    sleep(1);
   }
   printf("ERROR - cannot connect to just started RTDB!\n");
   printf("Please check with 'df -h|grep /dev/shm' that there is enough shm-ram free!\n");
   _exit(1);
  }
  setsid();
  pid=fork();
  if( pid < 0 )
   DIE("cannot fork: %s",strerror(errno));
  if( pid !=0 ) _exit(0);
 }

 kogmo_rtdb_connect_initinfo(&dbinfo, "", PROGNAME, KOGMO_RTDB_MANAGER_ALIVE_INTERVAL);
 dbinfo.flags = KOGMO_RTDB_CONNECT_FLAGS_MANAGER |
                (simmode ? KOGMO_RTDB_CONNECT_FLAGS_SIMULATION : 0) |
                (polling ? KOGMO_RTDB_CONNECT_FLAGS_DO_POLLING : 0);
 err = kogmo_rtdb_connect(&dbc, &dbinfo);
 if ( err < 0 )
   DIE("error %i creating database: %s", err, strerror(-err));
 DBG("manager handle is at %p and points to %p",&dbc,dbc);

 if( daemon ) {
  close(0);close(1);close(2);
  dup2(null_fd,0);
  dup2(null_fd,1);
  dup2(null_fd,2);
 }

 // now a daemon

#ifdef KOGMO_RTDB_HARDREALTIME
 if ( daemon )
   use_procchk_thread = 1;
#endif
 //test: if(getenv("PT")) use_procchk_thread = 1;

 ts_now = kogmo_timestamp_now(); //kogmo_rtdb_timestamp_now (dbc);
#if defined(KOGMO_RTDB_IPC_NO_PTHREADFUNCS) && !defined(KOGMO_RTDB_HARDREALTIME)
 use_procchk_thread = 0;
#else
 if (use_procchk_thread)
   {
     procchk_alive_ts = ts_now;
     err = kogmo_rtdb_pthread_create(dbc, &procchk_tid, NULL, &procchk_thread, NULL);
     if ( err != 0 )
       DIE("cannot start process check thread: %i",err);
   }
#endif

 for(;;) {
  if ( ! use_procchk_thread )
    {
      err = kogmo_rtdb_objmeta_purge_procs (dbc);
      if(err<0) MSG("error %d in purge_procs()",err);
    }

  err = kogmo_rtdb_objmeta_purge_objs (dbc);
  if(err<0) MSG("error %d in purge()",err);

  err = kogmo_rtdb_objmeta_upd_stats (dbc);
  if(err<0) MSG("error %d in upd_stats()",err);

  if(!daemon && kogmo_rtdb_debug & DBGL_APP) 
  do {
   kogmo_rtdb_objid_t oid;
   kogmo_rtdb_obj_c3_rtdb_t rtdbobj;
   kogmo_rtdb_objid_t err;
   kogmo_timestamp_string_t tstring;
   oid = kogmo_rtdb_obj_searchinfo (dbc, "rtdb", KOGMO_RTDB_OBJTYPE_C3_RTDB, 0, 0, 0, NULL, 1);
   if (oid<0) {MSG("error %d in searchinfo(C3_RTDB)",oid);break;}
   err = kogmo_rtdb_obj_readdata (dbc, oid, 0, &rtdbobj, sizeof(rtdbobj));
   if (err<0) {MSG("error %d in readdata(C3_RTDB)",err);break;}
   kogmo_timestamp_to_string(rtdbobj.base.data_ts,tstring);
   printf("RTDB Status: Last Update: %s, Resources free: Memory: %d/%dMB Objects: %d/%d Processes: %d/%d\n",
    tstring, rtdbobj.rtdb.memory_free/1024/1024, rtdbobj.rtdb.memory_max/1024/1024,
    rtdbobj.rtdb.objects_free, rtdbobj.rtdb.objects_max,
    rtdbobj.rtdb.processes_free, rtdbobj.rtdb.processes_max);
  } while(0);

  ts_now = kogmo_timestamp_now(); //kogmo_rtdb_timestamp_now (dbc);
  if ( use_procchk_thread && kogmo_timestamp_diff_secs(procchk_alive_ts, ts_now) > 3*KOGMO_RTDB_MANAGER_ALIVE_INTERVAL )
    {
      MSG("MANAGER ERROR: process check thread died");
    }

  dbc->ipc_h.shm_p -> manager_alive_ts = ts_now;
  kogmo_rtdb_cycle_done(dbc,0);

  //if (!cycle_ts || kogmo_timestamp_diff_secs(cycle_ts,kogmo_timestamp_now()) > 2*KOGMO_RTDB_MANAGER_ALIVE_INTERVAL )
  cycle_ts = kogmo_timestamp_add_secs( ts_now, KOGMO_RTDB_MANAGER_ALIVE_INTERVAL );
  if ( simmode )
    usleep(1e6*KOGMO_RTDB_MANAGER_ALIVE_INTERVAL);
  else
    kogmo_rtdb_sleep_until (dbc, cycle_ts);
 }

 kogmo_rtdb_disconnect(dbc, NULL);
 exit(0);

}

void * procchk_thread (void *dummy)
{
 int err;
 kogmo_timestamp_t cycle_ts = 0, ts_now;
 for(;;)
   {
     err = kogmo_rtdb_objmeta_purge_procs (dbc);
     if(err<0) MSG("error %d in purge_procs()",err);

     ts_now = kogmo_timestamp_now(); //kogmo_rtdb_timestamp_now (dbc);
     procchk_alive_ts = ts_now;
     cycle_ts = kogmo_timestamp_add_secs( ts_now, KOGMO_RTDB_MANAGER_ALIVE_INTERVAL );
     kogmo_rtdb_sleep_until (dbc, cycle_ts); // procchk_thread is never used in simmode
 }
 return NULL;
}
