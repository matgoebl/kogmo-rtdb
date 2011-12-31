/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file kogmo_rtdb_record.c
 * \brief AVI-Recorder for the Real-time Database
 *
 * Copyright (c) 2004-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */


#define _FILE_OFFSET_BITS 64
#include "stdio.h" /* printf */
#include <unistd.h> /* usleep */
#include <string.h> /* strerror */
#include <stdlib.h> /* qsort */
#include <getopt.h>
#include <signal.h>
#include "kogmo_rtdb_internal.h"
#include "kogmo_rtdb_trace.h"
#include "kogmo_rtdb_stream.h"
#include "kogmo_rtdb_avirawcodec.h"
#include "kogmo_rtdb_version.h"

#define DIEonERR(value) if (value<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-value);exit(1);}
#undef DIE
#define DIE(msg...) do { \
 fprintf(stderr,"%i DIED in %s line %i: ",getpid(),__FILE__,__LINE__); fprintf(stderr,msg); fprintf(stderr,"\n"); exit(1); } while (0)

#define MAXOPTLIST 50
#define STAT_CYCLE_TIME (0.1)

void
usage (void)
{
  fprintf(stderr,
"KogMo-RTDB Recorder (Rev.%d) (c) Matthias Goebl <matthias.goebl*goebl.net> RCS-TUM\n"
"Usage: kogmo_rtdb_record [.....]\n"
" -i ID    record object with id ID\n"
" -t TID   record objects with type TID\n"
" -n NAME  record objects with name NAME\n"
" -I ID    do not record object with id ID\n"
" -T TID   do not record objects with type TID\n"
" -N NAME  do not record objects with name NAME\n"
//" (regular expression if starting with ~)\n"
"          (can be repeated up to 50 times)\n"
" -0 NAME  write data of first object NAME as avi stream 0\n"
"          (it must be an KOGMO_RTDB_OBJTYPE_A2_IMAGE and must exist\n"
"          at recorder start time to determine the image dimensions.\n"
"          Only if any stream is given an avi header will be created)\n"
" => First start all camera-grabbers for all cameras then start recorder!\n"
" ...\n"
" -9 NAME  write data of first object NAME as avi stream 9\n"
" -r TID   raw-record selected objects with type TID (does not imply -t)\n"
" -a       record all objects\n"
//" -P       do not dump process and database objects\n"
" -l       log all object inserts/updates/deletes to stdout\n"
" -o FILE  write recorded data to file (nothing is recorded by default)\n"
" -s SECS  exit after recording SECS seconds (default is infinite or CTRL-C)\n"
" -B       print used disk bandwidth every second\n"
" -P FPS   set frames/second to FPS (default: 1/avg_cycletime of stream 0)\n"
" -W NAME  start recording when NAME gets created and end it when it is deleted\n"
" -q       don't print error messages when recording. the exit summary and -l/-B remains.\n"
" -h       print this help message\n"
"You can only use one recorder at a time. A second one will stall the other.\n"
"-i/-t/-n can be given up to %d times each.\n"
"The player objects playerctrl/stat/cmd will be filtered out automatically.\n\n",KOGMO_RTDB_REV,MAXOPTLIST);
  exit(1);
}

int
aviraw_fput_rtdb(FILE *fp, kogmo_timestamp_t ts, kogmo_rtdb_objid_t oid, uint32_t datatype,
                 void *data, int size)
{
  int ret;
  struct kogmo_rtdb_stream_chunk_t rtdbchunk;
  if (!fp) return -1;
  rtdbchunk.fcc[0] = 'R';
  rtdbchunk.fcc[1] = 'T';
  rtdbchunk.fcc[2] = 'D';
  rtdbchunk.fcc[3] = 'B';
  rtdbchunk.cb     =  size + sizeof(rtdbchunk) - 8;
  rtdbchunk.ts     =  ts;
  rtdbchunk.oid    =  oid;
  rtdbchunk.type   =  datatype;
  ret = fwrite(&rtdbchunk,sizeof(rtdbchunk),1,fp);
  if(ret!=1)
    DIE("cannot write rtdb stream data");//return ret<0? ret : -1;

  if ( size )
    {
      ret = fwrite(data,size,1,fp);
      if(ret!=1) DIE("cannot write rtdb stream data (%i)",ferror(fp)); //return ret<0? ret : -1;
    }

  if ( size & 1 ) // odd number
    {
      ret = fwrite("\0",1,1,fp);
      if(ret!=1) DIE("cannot write rtdb stream data (%i)",ferror(fp)); //return ret<0? ret : -1;
    }
  return 0;
}

kogmo_rtdb_objid_list_t knownid;

int
known_obj(kogmo_rtdb_objid_t id)
{
  int i;
  if(id==0) // init
    {
      //memset(&knownid,0,sizeof(knownid);
      for(i=0;i<KOGMO_RTDB_OBJIDLIST_MAX;i++)
        {
          knownid[i]=0;
        }
      return 0;
    }
  if(id<0) // delete id
    {
      for(i=0;i<KOGMO_RTDB_OBJIDLIST_MAX;i++)
        {
          if (knownid[i]==-id)
            knownid[i]=0;
        }
      return 0;
    }
  // query/add id
  for(i=0;i<KOGMO_RTDB_OBJIDLIST_MAX;i++)
    {
      if (knownid[i]==id)
        return 1;
    }
  for(i=0;i<KOGMO_RTDB_OBJIDLIST_MAX;i++)
    {
      if (knownid[i]==0)
        {
          knownid[i]=id;
          return 0;
        }
    }
  DIE("no free id-slots available");
}


// comparison function for qsort to sort oids ascending           
static int compare_oid(const void *a, const void *b) {
     kogmo_rtdb_objid_t * oid_a = (kogmo_rtdb_objid_t *) a;
     kogmo_rtdb_objid_t * oid_b = (kogmo_rtdb_objid_t *) b;
     if ( *oid_a < *oid_b ) return -1;
     if ( *oid_a > *oid_b ) return 1;
     return 0;
}


static void term_signal_handler (int signal);
static void do_exit (void);

// needed by term_signal_handler() & do_exit():
FILE *fp=NULL;
kogmo_timestamp_t initial_ts=0;
long int lost_messages=0;
kogmo_rtdb_handle_t *dbc=NULL;
unsigned long int events_total_written = 0, events_total = 0;

int
main (int argc, char **argv)
{
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_objid_t oid, recorderoid;
  //kogmo_rtdb_obj_info_t ctrlobj_info;
  //kogmo_rtdb_obj_c3_recorderctrl_t ctrlobj;
  kogmo_rtdb_obj_info_t statobj_info;
  kogmo_rtdb_obj_c3_recorderstat_t statobj;
  kogmo_timestamp_t ts;
  kogmo_timestamp_t last_status_ts=0, last_bandwidth_ts=0;
  long long int last_bandwidth_bytes_written=0;
  unsigned long int last_bandwidth_events_written=0;
  kogmo_timestamp_string_t timestring;
  kogmo_rtdb_obj_info_t obj_info;
  struct
  {
    kogmo_rtdb_subobj_base_t base;
    char data[5*1024*1024]; // THIS WILL BE THE MAXIMUM RECORDABLE OBJECT SIZE!!
  } obj_data, *obj_data_p;

  int event;
  int err,freebuf=-1;
  int i,j;
  kogmo_rtdb_objsize_t olen=0;
  kogmo_rtdb_objid_t oret;
 
  int do_oid=0, do_tid=0, do_name=0, do_xoid=0, do_xtid=0, do_xname=0;
  int do_all=0, do_log=0, do_avi=0, do_bandwidth=0, do_quiet=0;
  float do_fps=0, fps_stream0=0, fps=0, do_seconds=0;
  kogmo_rtdb_objid_t do_raw=0;
  kogmo_rtdb_objid_t     oid_list[MAXOPTLIST], xoid_list[MAXOPTLIST];
  kogmo_rtdb_objtype_t   tid_list[MAXOPTLIST], xtid_list[MAXOPTLIST];
  char                 *name_list[MAXOPTLIST], *xname_list[MAXOPTLIST];
  char *do_output=NULL,*do_waitobject=NULL;
  int opt;
  int traceit=0, streamit=0, junkit=0, record_enable=1;
  char w;

  int init_phase, init_i=0;
  kogmo_rtdb_objid_list_t initial_objects;

  char                 *name_stream[10]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
  kogmo_rtdb_objid_t     oid_stream[10]={0,0,0,0,0,0,0,0,0,0};
  kogmo_rtdb_obj_a2_image_t *videoobj_p;

  kogmo_rtdb_obj_slot_t trace_slot;
  int32_t obj_slot=-1, hist_slot=-1;
  int tracebufsize;
  int size;
  void *data;
  uint32_t datatype;
  struct kogmo_rtdb_stream_chunk_t rtdbchunk;

  kogmo_rtdb_require_revision(KOGMO_RTDB_REV);

  rtdbchunk.fcc[0] = 'R';
  rtdbchunk.fcc[1] = 'T';
  rtdbchunk.fcc[2] = 'D';
  rtdbchunk.fcc[3] = 'B';

  for(i=0;i<MAXOPTLIST;i++)
    {
      oid_list[i]=0;
      xoid_list[i]=0;
      tid_list[i]=0;
      xtid_list[i]=0;
      name_list[i]=NULL;
      xname_list[i]=NULL;
    }
  known_obj(0);

  while( ( opt = getopt (argc, argv, "i:t:n:I:T:N:0:1:2:3:4:5:6:7:8:9:r:alo:s:BW:P:qh") ) != -1 )
    switch(opt)
      {
        case 'i': if (++do_oid>MAXOPTLIST) DIE("ERROR: at maximum %d -%c items are allowed!",MAXOPTLIST,opt);
                  oid_list[do_oid-1] = strtol(optarg, (char **)NULL, 0); break;
        case 't': if (++do_tid>MAXOPTLIST) DIE("ERROR: at maximum %d -%c items are allowed!",MAXOPTLIST,opt);
                  tid_list[do_tid-1] = strtol(optarg, (char **)NULL, 0); break;
        case 'n': if (++do_name>MAXOPTLIST) DIE("ERROR: at maximum %d -%c items are allowed!",MAXOPTLIST,opt);
                  name_list[do_name-1] = optarg; break;
        case 'I': if (++do_xoid>MAXOPTLIST) DIE("ERROR: at maximum %d -%c items are allowed!",MAXOPTLIST,opt);
                  xoid_list[do_xoid-1] = strtol(optarg, (char **)NULL, 0); break;
        case 'T': if (++do_xtid>MAXOPTLIST) DIE("ERROR: at maximum %d -%c items are allowed!",MAXOPTLIST,opt);
                  xtid_list[do_xtid-1] = strtol(optarg, (char **)NULL, 0); break;
        case 'N': if (++do_xname>MAXOPTLIST) DIE("ERROR: at maximum %d -%c items are allowed!",MAXOPTLIST,opt);
                  xname_list[do_xname-1] = optarg; break;
        case '0': name_stream[0] = optarg; do_avi=1; break;
        case '1': name_stream[1] = optarg; do_avi=1; break;
        case '2': name_stream[2] = optarg; do_avi=1; break;
        case '3': name_stream[3] = optarg; do_avi=1; break;
        case '4': name_stream[4] = optarg; do_avi=1; break;
        case '5': name_stream[5] = optarg; do_avi=1; break;
        case '6': name_stream[6] = optarg; do_avi=1; break;
        case '7': name_stream[7] = optarg; do_avi=1; break;
        case '8': name_stream[8] = optarg; do_avi=1; break;
        case '9': name_stream[9] = optarg; do_avi=1; break;
        case 'r': do_raw = strtol(optarg, (char **)NULL, 0); break;
        case 'a': do_all = 1; break;
        case 'l': do_log = 1; break;
        case 'o': do_output = optarg; break;
        case 's': do_seconds = strtof(optarg, (char **)NULL); break;
        case 'B': do_bandwidth = 1; break;
        case 'P': do_fps = strtof(optarg, (char **)NULL); break;
        case 'W': do_waitobject = optarg; record_enable = 0; break;
        case 'q': do_quiet = 1; break;
        case 'h':
        default: usage(); break;
      }

  signal(SIGHUP, term_signal_handler);
  signal(SIGINT, term_signal_handler);
  signal(SIGQUIT, term_signal_handler);
  signal(SIGTERM, term_signal_handler);

  signal(SIGILL, term_signal_handler);
  signal(SIGBUS, term_signal_handler);
  signal(SIGFPE, term_signal_handler);
  signal(SIGSEGV, term_signal_handler);

  signal(SIGPIPE, term_signal_handler);
  signal(SIGCHLD, term_signal_handler);

  // Verbindung zur Datenbank aufbauen
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "c3_recorder", STAT_CYCLE_TIME); DIEonERR(err);
  dbinfo.flags = KOGMO_RTDB_CONNECT_FLAGS_NOHANDLERS;
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);
  recorderoid = kogmo_rtdb_obj_c3_process_searchprocessobj (dbc, 0, oid); DIEonERR(recorderoid);

  // Object fuer Status erstellen, initialisieren und initiale Werte eintragen
  err = kogmo_rtdb_obj_initinfo (dbc, &statobj_info,
    "recorderstat", KOGMO_RTDB_OBJTYPE_C3_RECORDERSTAT, sizeof (statobj)); DIEonERR(err);
  statobj_info.parent_oid = recorderoid;
  statobj_info.avg_cycletime = statobj_info.max_cycletime = STAT_CYCLE_TIME;
  oid = kogmo_rtdb_obj_insert (dbc, &statobj_info); DIEonERR(oid);
  err = kogmo_rtdb_obj_initdata (dbc, &statobj_info, &statobj); DIEonERR(err);
  //err = kogmo_rtdb_obj_writedata (dbc, statobj_info.oid, &statobj); DIEonERR(err);

  if ( do_output )
    {
      fp = fopen (do_output, "w");
      if ( fp==NULL ) DIE("cannot output file '%s'",do_output);
    }

  if ( do_avi )
    {
      avirawheader_t avirawheader;
      int mainheader_done=0;
      for(i=0;i<NSTREAMS;i++)
        {
          if ( name_stream[i] != NULL )
            {
              int bpp;
              uint32_t compression_fourcc=0;
              oid_stream[i] = kogmo_rtdb_obj_searchinfo (dbc, name_stream[i], KOGMO_RTDB_OBJTYPE_A2_IMAGE, 0, 0, 0, NULL, 1);
              if ( oid_stream[i] < 0 )
                {
                  printf("avistream: cannot find an image object named '%s' for stream %i\n",name_stream[i],i);
                  oid_stream[i] = 0;
                  continue;
                }
              err = kogmo_rtdb_obj_readinfo (dbc, oid_stream[i], 0, &obj_info);
              if ( err < 0 )
                {
                  printf("avistream: cannot read info of image object named '%s' for stream %i\n",name_stream[i],i);
                  oid_stream[i] = 0;
                  continue;
                }
              fps = 1.0/obj_info.avg_cycletime;
              if ( !mainheader_done ) fps_stream0 = fps;
              if ( do_fps ) fps = do_fps;
              olen = kogmo_rtdb_obj_readdata (dbc, oid_stream[i], 0, &obj_data, obj_info.size_max);
              if ( olen < 0 )
                {
                  printf("avistream: cannot read data of object named '%s' for stream %i\n",name_stream[i],i);
                  oid_stream[i] = 0;
                  continue;
                }
              if ( olen < sizeof(kogmo_rtdb_obj_a2_image_t) )
                DIE("avistream: size too small for object named '%s'",name_stream[i]);
              videoobj_p = (kogmo_rtdb_obj_a2_image_t *) &obj_data;
              bpp = videoobj_p->image.channels * (videoobj_p->image.depth&0xFFFF);
              switch(videoobj_p->image.colormodell)
                {
                  case A2_IMAGE_COLORMODEL_YUV411: compression_fourcc = 0x31313459; bpp = 12; break; // Y411 YUV 4:1:1 with a packed, 6 byte/4 pixel macroblock structure.
                                                                    //  0x31555949            IYU1 12 bit format used in mode 2 of the IEEE 1394 Digital Camera 1.04 spec. This is equivalent to Y411
                  case A2_IMAGE_COLORMODEL_YUV422: compression_fourcc = 0x59565955; bpp = 16; break; // UYVY YUV 4:2:2 (Y sample at every pixel, U and V sampled at every second pixel horizontally on each line). A macropixel contains 2 pixels in 1 u_int32.
                  case A2_IMAGE_COLORMODEL_YUV444: compression_fourcc = 0x32555949; bpp = 24; break; // IYU2 24 bit format used in mode 0 of the IEEE 1394 Digital Camera 1.04 spec
                  // http://www.jmcgowan.com/avicodecs.html: FOURCC values for DIB compression.
                  default: compression_fourcc = 0; break; // alternative: 'RAW '=0x20574152 'RGB '=0x20424752
                }
              DBG("aviraw: init stream %i: %.2f fps, %ix%x, %i bpp, compresson: 0x%x", i, fps, videoobj_p->image.width, videoobj_p->image.height, bpp, compression_fourcc);
              if ( !mainheader_done )
                {
                  err = aviraw_initheader(&avirawheader, fps, videoobj_p->image.width, videoobj_p->image.height, bpp);
                  if ( err ) DIE("avistream: error initializing main header");
                  mainheader_done = 1;
                  for(j=0;j<NSTREAMS;j++)
                    {
                       err = aviraw_initstream(&avirawheader, j, fps, videoobj_p->image.width, videoobj_p->image.height, bpp, compression_fourcc);
                       if ( err ) DIE("avistream: error initializing stream header");
                    }
                }
              err = aviraw_initstream(&avirawheader, i, fps, videoobj_p->image.width, videoobj_p->image.height, bpp, compression_fourcc);
              if ( err ) DIE("avistream: error initializing stream header");
            }
        }
        if ( !mainheader_done )
          {
            printf("avistream: cannot create an avi header without any video streams\n");
          }
        else
          {
            err = aviraw_fput_header(fp,&avirawheader);
            if ( err!=0 ) DIE("cannot write avi header");
          }
    }

  kogmo_rtdb_obj_trace_activate(dbc, 0, &tracebufsize);
  if ( tracebufsize == 0 )
    {
      printf("Error: The running RTDB has no record buffers.\n");
      printf("Please verify that you have POSIX message queues (kernel config option\n");
      printf("CONFIG_POSIX_MQUEUE, /proc/sys/fs/mqueue/msg_max must exist).\n");
      printf("You must restart the RTDB and its manager after you enabled it.\n\n");
      exit(1);
    }
  if ( tracebufsize < KOGMO_RTDB_TRACE_BUFSIZE )
    {
      printf("Warning: The running RTDB has only %d record buffers, %i were expected.\n",tracebufsize,KOGMO_RTDB_TRACE_BUFSIZE);
      printf("With too little record buffers you might loose data (error 'LOST MESSAGES').\n");
      printf("To fix this, please enter 'echo %i > /proc/sys/fs/mqueue/msg_max' as root\n",KOGMO_RTDB_TRACE_BUFSIZE_DESIRED);
      printf("and restart the RTDB and its manager.\n\n");
    }
  fflush(stdout);

  kogmo_rtdb_obj_trace_activate(dbc, 1, NULL);
  do
    {
      freebuf = kogmo_rtdb_obj_trace_receive (dbc, &oid, &ts, &event, &obj_slot, &hist_slot);
  //    printf("flush %i %i %i \n",freebuf, tracebufsize-freebuf, tracebufsize);
    }
  while (freebuf >= 0 && tracebufsize-freebuf > 1);
  //printf("go\n");
  //kogmo_rtdb_obj_trace_activate(dbc, 0, NULL);

  if (do_waitobject)
    {
      oid = kogmo_rtdb_obj_searchinfo ( dbc, do_waitobject, 0,0,0, 0, NULL, 1);
      if ( oid > 0 )
        record_enable = 1;
      if ( do_log )
        printf("# %s START-OBJECT '%s'.\n", 
               record_enable ? "FOUND" : "WAITING FOR", do_waitobject);
    }

  initial_ts = 0;
  init_phase = 1;

  while (1)
    {
      if ( init_phase && record_enable )
        {
          if ( ! initial_ts )
            {
              int n_obj=0;
              initial_ts = kogmo_rtdb_timestamp_now(dbc);
              oid = kogmo_rtdb_obj_searchinfo ( dbc, NULL, 0,0,0, initial_ts, initial_objects,0);
              if ( oid < 0 )
                DIE("cannot get list of initial objects");
              while ( n_obj < KOGMO_RTDB_OBJIDLIST_MAX && initial_objects[n_obj] > 0)
                n_obj++;
              // sort oids numerically ascending so is it guaranteed that a parent objects is created first
              qsort(&initial_objects[0], n_obj, sizeof(kogmo_rtdb_objid_t), compare_oid);
              init_i=0;
            }
          freebuf=-1;
          if ( initial_objects[init_i] != 0 )
            {
              oid = initial_objects[init_i];
              ts = initial_ts;
              event = KOGMO_RTDB_TRACE_UPDATED;
              kogmo_timestamp_to_string(ts, timestring);
              obj_slot=-1;
              hist_slot=-1;
            }
          else
            {
              init_phase = 0;
            }
          init_i++;
        }

      if ( ! init_phase || ! record_enable)
        {
          freebuf = kogmo_rtdb_obj_trace_receive (dbc, &oid, &ts, &event, &obj_slot, &hist_slot);
          DIEonERR(freebuf);
          kogmo_timestamp_to_string(ts, timestring);
          if (freebuf==0)
            {
              if (!do_quiet) printf("%05i %s # ERROR: LOST MESSAGES!\n",freebuf,timestring);
              lost_messages++;
              aviraw_fput_rtdb(fp,ts,0,KOGMO_RTDB_STREAM_TYPE_ERROR,NULL,0);
            }
        }

      if ( do_seconds > 0 )
        {
          if ( kogmo_timestamp_diff_secs (initial_ts, ts) > do_seconds )
            do_exit();
        }

      if ( do_bandwidth && fp)
        {
          double time_elapsed = kogmo_timestamp_diff_secs (last_bandwidth_ts, ts);
          if ( time_elapsed > 1 )
            {
              long long int total_bytes_written = ftello(fp);
              long long int bytes_written = total_bytes_written - last_bandwidth_bytes_written;
              long int events_written = events_total_written - last_bandwidth_events_written;
              if (last_bandwidth_ts)
                printf("# INFO: wrote %.3f MB and %li events in the last %.2f seconds =%.2f MB/s, %.3f MB and %li/%li events total, %li lost\n",
                     (float)bytes_written/1024/1024, events_written,
                     time_elapsed,
                     bytes_written>0 ? (float)bytes_written/1024/1024 / time_elapsed : 0,
                     (float)total_bytes_written/1024/1024,
                     events_total_written, events_total, lost_messages);
              last_bandwidth_ts = ts;
              last_bandwidth_bytes_written = total_bytes_written;
              last_bandwidth_events_written = events_total_written;
            }
        }

      if ( 1 ) // always update status object (todo: merge with do_bandwidth, move out of getevent->..->readobj)
        {
          double time_elapsed = kogmo_timestamp_diff_secs (last_status_ts, ts);
          if ( time_elapsed >= statobj_info.avg_cycletime )
            {
              last_status_ts = ts;
              statobj.base.data_ts = ts;
              statobj.recorderstat.begin_ts = initial_ts;
              statobj.recorderstat.bytes_written = fp ? ftello(fp) : 0;
              statobj.recorderstat.events_written = events_total_written;
              statobj.recorderstat.events_total = events_total;
              statobj.recorderstat.events_lost = lost_messages;
              statobj.recorderstat.event_buflen = tracebufsize;
              statobj.recorderstat.event_buffree = freebuf;
              err = kogmo_rtdb_obj_writedata (dbc, statobj_info.oid, &statobj); DIEonERR(err);
            }
        }

      oret = kogmo_rtdb_obj_readinfo (dbc, oid, ts, &obj_info);
      if ( oret < 0 )
        {
          if (!do_quiet) printf("%05i %s # ERROR: TOO SLOW? reading object info failed\n",freebuf,timestring);
          lost_messages++;
          aviraw_fput_rtdb(fp,ts,oid,KOGMO_RTDB_STREAM_TYPE_ERROR,NULL,0);
          continue;
        }


      if ( do_waitobject )
        {
          if ( ! record_enable && event == KOGMO_RTDB_TRACE_INSERTED &&
               strncmp(do_waitobject,obj_info.name,KOGMO_RTDB_OBJMETA_NAME_MAXLEN)==0 )
            {
              if ( do_log )
                printf("# START-OBJECT '%s' APPEARED.\n", do_waitobject);
              record_enable = 1;
            }
          if ( event == KOGMO_RTDB_TRACE_DELETED &&
               strncmp(do_waitobject,obj_info.name,KOGMO_RTDB_OBJMETA_NAME_MAXLEN)==0 )
            {
              if ( do_log )
                printf("# STOP-OBJECT '%s' DISAPPEARD.\n", do_waitobject);
              do_exit();
            }
        }

      if ( ! record_enable )
        continue;

      events_total++;

      // Now decide, whether to log this object:
      traceit=0; streamit=0; junkit=0;
      if ( do_all )
        traceit++;
      // Filter out Player Objects:
      if ( obj_info.otype == KOGMO_RTDB_OBJTYPE_C3_PLAYERSTAT && strncmp("playerstat",obj_info.name,KOGMO_RTDB_OBJMETA_NAME_MAXLEN)==0 ) traceit=0;
      if ( obj_info.otype == KOGMO_RTDB_OBJTYPE_C3_PLAYERCTRL && strncmp("playerctrl",obj_info.name,KOGMO_RTDB_OBJMETA_NAME_MAXLEN)==0 ) traceit=0;
      #ifdef OBSOLETE_COMMAND_OBJECT
       if ( obj_info.otype == KOGMO_RTDB_OBJTYPE_C3_SIXDOF     && strncmp("kogmo_rtdb_play_cmd",obj_info.name,KOGMO_RTDB_OBJMETA_NAME_MAXLEN)==0 ) traceit=0;
      #endif
      // Filter out Recorder Objects:
      if ( obj_info.otype == KOGMO_RTDB_OBJTYPE_C3_RECORDERSTAT && strncmp("recorderstat",obj_info.name,KOGMO_RTDB_OBJMETA_NAME_MAXLEN)==0 ) traceit=0;
      if ( obj_info.otype == KOGMO_RTDB_OBJTYPE_C3_RECORDERCTRL && strncmp("recorderctrl",obj_info.name,KOGMO_RTDB_OBJMETA_NAME_MAXLEN)==0 ) traceit=0;

      for(i=0;i<MAXOPTLIST;i++)
        {
          if ( oid_list[i] && oid_list[i] == oid ) traceit++;
          if ( tid_list[i] && tid_list[i] == obj_info.otype ) traceit++;
          if ( name_list[i] && strncmp(name_list[i],obj_info.name,KOGMO_RTDB_OBJMETA_NAME_MAXLEN)==0 ) traceit++;
        }
      for(i=0;i<MAXOPTLIST;i++)
        {
          if ( xoid_list[i] && xoid_list[i] == oid ) traceit=0;
          if ( xtid_list[i] && xtid_list[i] == obj_info.otype ) traceit=0;
          if ( xname_list[i] && strncmp(xname_list[i],obj_info.name,KOGMO_RTDB_OBJMETA_NAME_MAXLEN)==0 ) traceit=0;
        }
      for(i=0;i<=9;i++)
        {
          if ( oid_stream[i] && oid_stream[i] == oid )
            {
              traceit++;
              streamit=i+1;
              break;
            }
        }
      if ( do_raw && do_raw == obj_info.otype )
        junkit=1;

      if ( ! traceit && ! do_log ) continue;

      switch(event)
        {
          case KOGMO_RTDB_TRACE_UPDATED:
            if ( init_phase )
              {
                olen = kogmo_rtdb_obj_readdata (dbc, oid, ts, &obj_data, sizeof(obj_data));
              }
            else
              {
                trace_slot.oid = oid;
                trace_slot.committed_ts = ts;
                trace_slot.object_slot = obj_slot;
                trace_slot.history_slot = hist_slot;
                olen = kogmo_rtdb_obj_readdataslot_ptr (dbc, -1, 0, &trace_slot, &obj_data_p);
                if ( olen > 0 )
                  {
                    memcpy(&obj_data, obj_data_p, olen < sizeof(obj_data) ? olen : sizeof(obj_data));
                    olen = kogmo_rtdb_obj_readdataslot_ptr (dbc, 1, 0, &trace_slot, &obj_data_p); 
                  }
              }
            if ( olen < 0 && ( !init_phase || olen != -KOGMO_RTDB_ERR_NOTFOUND ) )
              {
                if (!do_quiet) printf("%05i %s # ERROR: TOO SLOW? reading data for object %lli failed\n",freebuf,timestring,(long long int)oid);
                lost_messages++;
                aviraw_fput_rtdb(fp,ts,oid,KOGMO_RTDB_STREAM_TYPE_ERROR,NULL,0);
              }
            if ( olen < 0 )
              olen = 0;
            if ( olen > sizeof(obj_data) )
              {
              if (!do_quiet) printf("%05i %s # ERROR: internal record buffer too small for object %lli: buffer (%lli) < object data (%lli) will be truncated\n",
                     freebuf,timestring,(long long int)oid, (long long int)sizeof(obj_data), (long long int)olen );
// TODO: auto-resize!!!
              }
            break;
        }

      if ( do_log )
        {
          if ( traceit ) w='*'; else w=' ';
          switch(event)
            {
              case KOGMO_RTDB_TRACE_INSERTED:
                printf("%05i %s +%c %lli '%s'\n",freebuf,timestring,w,(long long int)oid, obj_info.name);
                break;
              case KOGMO_RTDB_TRACE_CHANGED:
                printf("%05i %s *%c %lli '%s'\n",freebuf,timestring,w,(long long int)oid, obj_info.name);
                break;
              case KOGMO_RTDB_TRACE_DELETED:
                printf("%05i %s -%c %lli '%s'\n",freebuf,timestring,w,(long long int)oid, obj_info.name);
                known_obj(-oid);
                break;
              case KOGMO_RTDB_TRACE_UPDATED:
                printf("%05i %s .%c %lli '%s' %lli bytes\n",freebuf,timestring,w,(long long int)oid, obj_info.name, (long long int)olen);
                break;
              default:
                printf("%05i %s # Unknown Event %i\n",freebuf,timestring,event);
                break;
            }
        }

      if ( ! traceit ) continue;

      // from here: really record object
      events_total_written++;

      if ( event == KOGMO_RTDB_TRACE_UPDATED && !known_obj(oid) )
        {
          if ( do_log )
            {
              printf("%05i %s ++ %lli '%s'\n",freebuf,timestring,(long long int)oid, obj_info.name);
            }
          if ( fp )
            {
              aviraw_fput_rtdb(fp,ts,oid,KOGMO_RTDB_STREAM_TYPE_RFROBJ,&obj_info,sizeof(obj_info));
            }
        }

      if ( olen <= 0 ) continue;

      if ( ! fp ) continue;

      size = 0;
      datatype = 0;
      switch(event)
        {
          case KOGMO_RTDB_TRACE_INSERTED:
            datatype = KOGMO_RTDB_STREAM_TYPE_ADDOBJ;
            size = sizeof(obj_info);
            data = &obj_info;
            break;
          case KOGMO_RTDB_TRACE_CHANGED:
            datatype = KOGMO_RTDB_STREAM_TYPE_CHGOBJ;
            size = sizeof(obj_info);
            data = &obj_info;
            break;
          case KOGMO_RTDB_TRACE_DELETED:
            datatype = KOGMO_RTDB_STREAM_TYPE_DELOBJ;
            size = sizeof(obj_info);
            data = &obj_info;
            break;
          case KOGMO_RTDB_TRACE_UPDATED:
            datatype = KOGMO_RTDB_STREAM_TYPE_UPDOBJ;
            data = &obj_data;
            size = obj_data.base.size;
            if ( streamit )
              {
                datatype = KOGMO_RTDB_STREAM_TYPE_UPDOBJ_NEXT;
                size = sizeof (kogmo_rtdb_obj_a2_image_t);
              }
            if ( junkit )
              {
                datatype = KOGMO_RTDB_STREAM_TYPE_UPDOBJ_NEXT;
                size = sizeof (kogmo_rtdb_subobj_base_t);
              }
            break;
          default:
            continue;
            break;
        }

      if ( ! datatype) continue;


      aviraw_fput_rtdb(fp,ts,oid,datatype,data,size);

      if ( event == KOGMO_RTDB_TRACE_UPDATED )
        {
          if ( streamit )
            {
              videoobj_p = (kogmo_rtdb_obj_a2_image_t *) &obj_data;
              err = aviraw_fput_stream(fp,streamit-1,&videoobj_p->image.data[0],obj_data.base.size - sizeof(kogmo_rtdb_obj_a2_image_t));
              if ( err!=0 ) DIE("cannot write avi stream");
            }
          else if ( junkit )
            {
              err = aviraw_fput_junk(fp, &obj_data.data[0], obj_data.base.size - sizeof (kogmo_rtdb_subobj_base_t));
              if ( err!=0 ) DIE("cannot write avi raw data");
            }
        }



      // so richtiger schwachsinn waere (da wuerden wir uns selbst loggen..): kogmo_rtdb_cycle_done(dbc,0);
    }

  return 0;
}

static void
do_exit (void)
{
 long long int bytes_written = 0;
 // TODO: last known time aus hauptprogramm!
 double time_elapsed = initial_ts ? kogmo_timestamp_diff_secs (initial_ts, kogmo_rtdb_timestamp_now(dbc)) : 0.001;
 if(fp)
   bytes_written = ftello(fp);
 // TODO: selbst rechnen!
 printf("# END. wrote %.3f GB and %li/%li events in %.2f seconds (%.1f MB/s, %.1f Ev/s) with >=%li events/data blocks lost.\n",
        (float)bytes_written/1024/1024/1024, events_total_written, events_total,
        initial_ts ? time_elapsed : 0,
        bytes_written>0 && time_elapsed>0 ? (float)bytes_written/1024/1024 / time_elapsed : 0,
        time_elapsed>0 ? (float)events_total_written / time_elapsed : 0,
        lost_messages);
 if (dbc)
   {
     kogmo_rtdb_obj_trace_activate(dbc, 0, NULL);
     kogmo_rtdb_disconnect (dbc, NULL);
   }
 exit(0);
}

static void
term_signal_handler (int signal)
{
 printf("# STOP. received signal %i.\n", signal);
 do_exit();
}
