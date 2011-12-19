/*! \file rtdbrecstat.c
 * \brief Watch RTDB-Recorder Status
 *
 * (c) 2008 Matthias Goebl <mg@tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdio.h> /* printf */
#include <unistd.h> /* getpid */
#include <stdlib.h> /* exit */
#include "kogmo_rtdb.h"

#define DIEonERR(value) if (value<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-value);exit(1);}
#define ERR(msg...) do { \
 fprintf(stderr,"%d ERROR in %s line %i: ",getpid(),__FILE__,__LINE__); fprintf(stderr,msg); fprintf(stderr,"\n"); } while (0)

int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_obj_info_t statobj_info;
  kogmo_rtdb_obj_c3_recorderstat_t statobj;
  kogmo_rtdb_objid_t oid;
  kogmo_rtdb_objsize_t olen;
  kogmo_timestamp_string_t timestring;
  kogmo_timestamp_t now;
  double runtime, timelag, last_runtime=0, bandwidth, events_per_sec;
  unsigned int last_bytes=0, last_events=0;
  int err;
  
  // Verbindung zur Datenbank aufbauen
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "rtdbrecstat", 0.1); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);

  while (1)
   {
    do
    {
      // Auf Object warten
      oid = kogmo_rtdb_obj_searchinfo_wait (dbc, "recorderstat",
        KOGMO_RTDB_OBJTYPE_C3_RECORDERSTAT, 0, 0);
      if (oid<0) {ERR("Error %i while searching for object!",oid);break;}

      // Infoblock des Objektes holen
      err = kogmo_rtdb_obj_readinfo (dbc, oid, 0, &statobj_info);
      if (err<0) {ERR("Error %i while reading object info!",err);break;}

      while (1)
        {
          olen = kogmo_rtdb_obj_readdata_waitnext (dbc, oid, statobj.base.committed_ts, &statobj, sizeof(statobj));
          if (olen<0) {ERR("Error %i while reading object !",olen);break;}
          if ( olen < sizeof(statobj) )
            continue; // object to small (shouldn't happen - ask the object-writer!)

          now = kogmo_rtdb_timestamp_now(dbc);
          kogmo_timestamp_to_string(statobj.recorderstat.begin_ts, timestring);
          runtime = statobj.recorderstat.begin_ts ?
                     kogmo_timestamp_diff_secs(statobj.recorderstat.begin_ts,now) : 0;
          timelag = kogmo_timestamp_diff_secs(statobj.base.data_ts, statobj.base.committed_ts);
          bandwidth = runtime-last_runtime != 0 ? (double)(statobj.recorderstat.bytes_written - last_bytes)/1024/1024 / (runtime-last_runtime) : 0;
          events_per_sec = runtime-last_runtime != 0 ? (double)(statobj.recorderstat.events_written - last_events) / (runtime-last_runtime) : 0;

          printf("Recorder started %s, running %.2f secs, logged %i of %i events with %.3f MB. Lost %i events, %i/%i trace buffers free, %.6f secs lag. %.3f MB/s %.2f events/s.\n",
                 timestring, runtime, statobj.recorderstat.events_written, statobj.recorderstat.events_total,
                 (double)statobj.recorderstat.bytes_written/1024/1024, statobj.recorderstat.events_lost, statobj.recorderstat.event_buffree,
                 statobj.recorderstat.event_buflen, timelag, bandwidth, events_per_sec);

          last_bytes = statobj.recorderstat.bytes_written;
          last_runtime = runtime;
          last_events = statobj.recorderstat.events_written;   
        }
    } while (0);
    fprintf(stderr,"Retry..\n");
    sleep(1);
   }
  err = kogmo_rtdb_disconnect(dbc, NULL); DIEonERR(err);

  return 0;
}
