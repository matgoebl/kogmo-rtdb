/*! \file kogmo_rtdb_test.c
 * \brief Testprogram for Concurrent Access
 *
 * Copyright (c) 2005,2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */


#include <stdio.h> /* printf */
#include <unistd.h> /* usleep */
#include <string.h> /* strerror */

#include "kogmo_rtdb.h"
#include "kogmo_time.h"

#if !defined(NO_TSC) && !defined(MACOSX)
#include "tscmeasure.c"
#else
typedef kogmo_timestamp_t tscstamp_t;
tscstamp_t readtsc(void) {
  return(kogmo_timestamp_now());
}
double tsc2seconds(tscstamp_t tsc) {
  return(kogmo_timestamp_diff_secs(tsc,0));
}
#endif

#define DIE(msg...) do { \
  fprintf(stderr,"%i DIED in %s line %i: ",getpid(),__FILE__,__LINE__); \
  fprintf(stderr,msg); \
  fprintf(stderr,"\n"); \
  printf("%% FAIL"); \
  if ( do_timing ) {\
   printf(" in %s line %i: ",__FILE__,__LINE__); \
   printf(msg); \
  } \
  printf("\n"); \
  exit(1); \
 } while (0)

void usage(void) {printf("\n\
Usage: kogmo_rtdb_test [OPTIONS..]\n\
-c  do_create = 1\n\
-d  do_delete = 1\n\
-r  do_read = 1\n\
-w  do_write = 1 (special with -t)\n\
-p  do_ptr = 1 (no effect on tracking)\n\
-R  do_read_deny = 1\n\
-W  do_write_allow = 1\n\
-H  do_cyclewatch = 1\n\
-n  numloops = atoi (optarg)\n\
-N  numcreateloops = atoi (optarg)\n\
-K  do_keep_alloc=1\n\
-I  do_immediately_delete=1\n\
-x  set_x = atof (optarg)\n\
-s  size = atoi (optarg)\n\
-S  do_sleep = atof (optarg)\n\
-t  do_track = 1 (prints every second cycle!)\n\
-T  do_timing = 1 (takes 4 cycles lead time!)\n\
-D  do_delay = atof (optarg) \n\
-Y  do_history \n\
-v  be_verbose = 1\n\
-U  no realtime warmup\n\
-h  help\n\
");}

int no_rt_warmup = 0;
inline void rt(void)
{
  if(no_rt_warmup) return;
  struct timespec interval;
  interval.tv_sec = 0;
  interval.tv_nsec = 1;
  nanosleep(&interval, NULL);
}

#define MAX_DATA_SIZE (1024*768*3)
int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  tscstamp_t start,stop,sum,max;
  int i;
  kogmo_rtdb_obj_info_t om;
  kogmo_rtdb_obj_info_t om2;
  typedef struct
  {
    int32_t intval[2];
  } kogmo_rtdb_subobj_c3_priv_myintsobj_t;
  typedef struct
  {
    kogmo_rtdb_subobj_base_t base;
    kogmo_rtdb_subobj_c3_priv_myintsobj_t ints;
    char testdata[MAX_DATA_SIZE];
  } kogmo_rtdb_obj_c3_priv_myobj_t;
  kogmo_rtdb_obj_c3_priv_myobj_t myobj,*myobj_p;
  const int size_max = sizeof (kogmo_rtdb_obj_c3_priv_myobj_t);
  const int size_min = size_max - MAX_DATA_SIZE;

  kogmo_rtdb_objid_t oid;
  int ret;
  int opt;
  int do_create=0, do_delete=0, do_read=0, do_write=0, numloops=1, numcreateloops=1;
  int do_read_deny=0, do_write_allow=0,do_keep_alloc=0,do_immediately_delete=0;
  int do_timing=0, do_track=0, be_verbose=0, do_ptr=0, do_cyclewatch=0;
  float do_sleep=0, do_delay=0, do_history=0;
  int set_x=0;
  int size=0;

  while( ( opt = getopt (argc, argv, "cdrwpRWHn:N:KIx:s:S:tTD:UvY:h") ) != -1 )
    switch(opt)
      {
        case 'c': do_create = 1; break;
        case 'd': do_delete = 1; break;
        case 'r': do_read = 1; break;
        case 'w': do_write = 1; break;
        case 'p': do_ptr = 1; break;
        case 'R': do_read_deny = 1; break;
        case 'W': do_write_allow = 1; break;
        case 'H': do_cyclewatch = 1; break;
        case 'n': numloops = atoi (optarg); break;
        case 'N': numcreateloops = atoi (optarg); break;
        case 'K': do_keep_alloc=1; break;
        case 'I': do_immediately_delete=1; break;
        case 'x': set_x = atoi (optarg); break;
        case 's': size = atoi (optarg); break;
        case 'S': do_sleep = atof (optarg); break;
        case 't': do_track = 1; break;
        case 'T': do_timing = 1; break;
        case 'D': do_delay = atof (optarg) ; break;
        case 'U': no_rt_warmup = 1; break;
        case 'Y': do_history = atof (optarg); break;
        case 'v': be_verbose = 1; break;
        case 'h': usage(); exit(0); break;
        default: usage(); exit(1); break;
      }

  kogmo_rtdb_connect_initinfo(&dbinfo, "", "kogmo_rtdb_test", do_delay ? do_delay : 0.010);
  if ( do_immediately_delete )
    dbinfo.flags = KOGMO_RTDB_CONNECT_FLAGS_IMMEDIATELYDELETE;
  ret = kogmo_rtdb_connect(&dbc, &dbinfo);
  if ( ret < 0 )
    DIE("connect failed, error %i (%s)", ret, strerror(-ret));

  if ( size )
    {
      if ( size < size_min )
        DIE("size %i too small, minimum is %i", size, size_min);
      if ( size > size_max )
        DIE("size %i too large, maximum is %i", size, size_max);
    }
  else
    {
      size = size_min;
    }

  if ( do_track && do_write )
    do_create = do_delete = 1;

  while ( numcreateloops-- )
    {
      if ( do_create )
        {
          kogmo_rtdb_obj_initinfo (dbc, &om, do_track ? "c3_test_object_track" : "c3_test_object",
                                   KOGMO_RTDB_OBJTYPE_C3_INTS,
                                   size);
          om.history_interval = do_delay >= 0.001 ? 3 : 0.1;
          if ( do_history > 0 ) om.history_interval = do_history;
          om.flags.read_deny = do_read_deny;
          om.flags.write_allow = do_write_allow;
          om.flags.keep_alloc = do_keep_alloc;
          om.flags.immediately_delete = do_immediately_delete;
          om.flags.cycle_watch = do_cyclewatch;
          if ( do_delay )
            {
              om.avg_cycletime = do_delay;
              om.max_cycletime = do_delay*0.9;
            }
          rt();
          start = readtsc();
          oid = kogmo_rtdb_obj_insert (dbc, &om);
          stop = readtsc();
          i = om.history_size;
          if ( oid < 0 )
            DIE("object insert failed, error %i (%s)", oid, strerror(-oid)); 
          if ( do_timing )
            printf("%% CREATE %s %i %f %i\n", do_write_allow ? "W":"_", size,
                   tsc2seconds(stop-start), i);
          if ( do_track )
            memcpy(&om2,&om,sizeof(om));
        }
      if ( !do_create || do_track )
        {
          rt();
          start = readtsc();
          oid = kogmo_rtdb_obj_searchinfo_wait (dbc, "c3_test_object", KOGMO_RTDB_OBJTYPE_C3_INTS,
                                           0, 0);
          if (oid < 0 )
            DIE("cannot find object");
          ret = kogmo_rtdb_obj_readinfo (dbc, oid, 0, &om );
          stop = readtsc();
          if ( ret < 0 )
            DIE("cannot access object metadata");
          if ( do_timing )
            printf("%% SEARCHSELECTMETA %s %i %f\n", do_write_allow ? "W":"_", size,
                   tsc2seconds(stop-start));
        }

      if ( do_write && !do_track )
        {
          sum = 0;
          max = 0;
          myobj.base.size    = size; //sizeof(myobj);
          for ( i=1; i<=numloops; i++ )
            {
              rt();
              myobj.base.data_ts = kogmo_rtdb_timestamp_now(dbc);
              myobj.ints.intval[0] = set_x;
              myobj.ints.intval[1] = i;
              start = readtsc();
              ret = kogmo_rtdb_obj_writedata (dbc, om.oid, &myobj);
              if ( ret < 0 )
                DIE("object commit failed, error %i (%s)", ret, strerror(-ret));
              stop = readtsc();
              if ( do_timing )
                printf("%% COMMIT %s %i %f\n", do_write_allow ? "W":"_", size,
                       tsc2seconds(stop-start));
              if ( be_verbose )
                printf("committed: oid=%lli, ts=%lli\n", (long long int) om.oid,
                       (long long int) myobj.base.committed_ts);
              sum += stop - start;
              if ( sum > max )
                max = sum;
              if ( do_delay )
                //usleep(do_delay * 1000000); // this is a task-switching point
                kogmo_rtdb_sleep_until( dbc, kogmo_timestamp_add_secs( kogmo_rtdb_timestamp_now(dbc), do_delay) ); // stay in realtime context as producer
            }
    //      printf("Time for 1 commit is max %.9f seconds, avg. %.9f, %i commits in %.9f seconds.\n",
    //             tsc2seconds(max), tsc2seconds(sum)/numloops, numloops, tsc2seconds(sum));
        }

      if ( do_read )
        {
          kogmo_timestamp_t next_ts=0, ts_now=0;
          sum = 0;
          max = 0;
          for ( i=1; i<=numloops; i++ )
            {
              myobj_p = &myobj;
              rt();
              if ( do_track ) {
                // ein zyklus vorlauf
                ret = kogmo_rtdb_obj_readdata_waitnext (dbc, om.oid, next_ts, &myobj, size_max);
                next_ts = myobj.base.committed_ts;
                ret = kogmo_rtdb_obj_readdata_waitnext (dbc, om.oid, next_ts, &myobj, size_max);
                ts_now = kogmo_rtdb_timestamp_now(dbc);
                next_ts = myobj_p->base.committed_ts;
              } else {
                start = readtsc();
                if ( do_ptr )
                  ret = kogmo_rtdb_obj_readdata_ptr (dbc, om.oid, 0, &myobj_p);
                else
                  ret = kogmo_rtdb_obj_readdata (dbc, om.oid, 0, &myobj, size_max);
                stop = readtsc();
              }
              if ( ret < 0 )
                DIE("object select failed, error %i (%s)", ret, strerror(-ret));
              if ( do_track && do_write )
                {
                  // memcpy(&myobj2,&myobj,sizeof(myobj));
                  ret = kogmo_rtdb_obj_writedata (dbc, om2.oid, &myobj);
                  if ( ret < 0 )
                    DIE("object commit failed, error %i (%s)", ret, strerror(-ret));
                }
              if ( be_verbose )
                printf("selected: oid=%lli, ts=%lli\n", (long long int) om.oid,
                       (long long int) myobj_p->base.committed_ts);
              if ( do_timing && i>3)
                printf("%% SELECT %s %i %f\n", do_write_allow ? "W":"_", myobj_p->base.size,
                       do_track ? kogmo_timestamp_diff_secs(myobj_p->base.data_ts, ts_now)
                                : tsc2seconds(stop-start));
              sum += stop - start;
              if ( do_delay )
                usleep(do_delay * 1000000); // this is a task-switching point
    //printf("%i: %.9f\n",i,kogmo_timestamp_diff_secs (ts_begin,myobj_p->base.data_ts));
            }
    //      printf("Time for 1 select is max %.9f seconds, avg. %.9f, %i commits in %.9f seconds.\n",
    //             tsc2seconds(max), tsc2seconds(sum)/numloops, numloops, tsc2seconds(sum));
        }

      if ( do_sleep )
        usleep (do_sleep * 1000000 );

      if ( do_delete )
        {
          if ( do_track )
            memcpy(&om,&om2,sizeof(om));
          i = om.history_size;
          rt();
          start = readtsc();
          ret = kogmo_rtdb_obj_delete (dbc, &om);
          stop = readtsc();
          if ( ret < 0 )
            DIE("object delete failed, error %i (%s)", ret, strerror(-ret)); 
          if ( do_timing )
            printf("%% DELETE %s %i %f %i\n", do_write_allow ? "W":"_", size,
                   tsc2seconds(stop-start), i);
        }
    }

  kogmo_rtdb_disconnect(dbc, NULL);

  return 0;
}
