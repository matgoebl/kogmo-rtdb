/*! \file kogmo_rtdb_ratetest.c
 * \brief Example for writing to the RTDB with high commit rates
 *
 * (c) 2007 Matthias Goebl <mg@tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdio.h> /* printf */
#include <unistd.h> /* sleep,getpid */
#include <stdlib.h> /* exit */
#include <math.h> /* sin,cos */
#include "kogmo_rtdb.h"

#define DIEonERR(value) if (value<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-value);exit(1);}

#define OBJSIZE_MAX (3*1024*1024)
#define SIZE_MIN ((int)sizeof(kogmo_rtdb_subobj_base_t))

typedef struct
{
  kogmo_rtdb_subobj_base_t base;
  char testdata[OBJSIZE_MAX-SIZE_MIN];
} myobj_t;

int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_obj_info_t myobj_info;
  myobj_t myobj;
  kogmo_rtdb_objid_t oid;
  int err, i, loops;
  kogmo_timestamp_t ts_start,ts_stop;
  int size = 1024;
  
  if ( argc >= 2 ) size = atoi(argv[1]);
  if ( size > OBJSIZE_MAX || size < SIZE_MIN )
    {
      printf("Usage: kogmo_rtdb_ratetest [OBJSIZE]\n");
      printf("Tests the maximal kogmo-rtdb commit rate.\n");
      printf("OBJSIZE must be between %i and %i.\n", SIZE_MIN, OBJSIZE_MAX);
      exit(1);
    }
  loops = size > 1000 ? 1000000000 / size : 1000000;

  // Verbindung zur Datenbank aufbauen, Zykluszeit unbekannt.. :-(
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "kogmo_rtdb_ratetest", 0.001); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);

  // Object erstellen
  err = kogmo_rtdb_obj_initinfo (dbc, &myobj_info,
    "rate-test-object", KOGMO_RTDB_OBJTYPE_C3_TEXT, sizeof (myobj)); DIEonERR(err);
  myobj_info.max_cycletime = myobj_info.avg_cycletime = 0.001;
  myobj_info.history_interval = 0.010; // => about 10 history slots
  myobj_info.size_max = size;
  oid = kogmo_rtdb_obj_insert (dbc, &myobj_info); DIEonERR(oid);

  // Datenobjekt initialisieren
  err = kogmo_rtdb_obj_initdata (dbc, &myobj_info, &myobj); DIEonERR(err);
  err = kogmo_rtdb_obj_writedata (dbc, myobj_info.oid, &myobj); DIEonERR(err);

  while(1)
    {
      ts_start = kogmo_timestamp_now();
      for(i=0;i<loops;i++)
        {
          err = kogmo_rtdb_obj_writedata (dbc, myobj_info.oid, &myobj); DIEonERR(err);
        }
      ts_stop = kogmo_timestamp_now();
      double runtime = kogmo_timestamp_diff_secs ( ts_start, ts_stop );
      printf("Committing %d Bytes: Time for %i loops is %f seconds => %.3f us each, %.0f per second.\n",
             size, loops, runtime, runtime/loops*1e6, loops/runtime);
    }

  err = kogmo_rtdb_obj_delete (dbc, &myobj_info); DIEonERR(err);
  err = kogmo_rtdb_disconnect (dbc, NULL); DIEonERR(err);
  return 0;
}
