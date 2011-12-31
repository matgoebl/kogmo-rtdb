/*! \file kogmo_rtdb_reader_nonblocking.c
 * \brief Example for timeout-blocking read from the RTDB
 *
 * (c) 2005,2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdio.h> /* printf */
#include <unistd.h> /* getpid */
#include <stdlib.h> /* exit */
#include "kogmo_rtdb.h"

#define DIEonERR(value) if (value<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-value);exit(1);}

int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_obj_info_t demoobj_info;
  kogmo_rtdb_obj_c3_ints256_t demoobj;
  kogmo_rtdb_objid_t oid;
  kogmo_rtdb_objsize_t olen;
  int err;
  
  // Verbindung zur Datenbank aufbauen, unsere Zykluszeit is 33 ms
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "a2_roadviewer_example", 0.033); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);

  // Auf Demo-Objekt warten
  do
    {
      oid = kogmo_rtdb_obj_searchinfo_wait_until (dbc, "demo_object",
              KOGMO_RTDB_OBJTYPE_C3_INTS, 0, 0,
              kogmo_timestamp_add_secs (kogmo_timestamp_now(), 3.7) ); // alle 3.7 Sekunden aufwachen
      if ( oid == -KOGMO_RTDB_ERR_TIMEOUT )
        {
          printf("(search timeout)\n");
        }
    }
  while( oid == -KOGMO_RTDB_ERR_TIMEOUT );
  DIEonERR(oid);

  // Infoblock des Objektes holen (wenn Metadaten benoetigt werden)
  err = kogmo_rtdb_obj_readinfo (dbc, oid, 0, &demoobj_info); DIEonERR(err);

  while (1)
    {
      kogmo_timestamp_string_t timestring;
      olen = kogmo_rtdb_obj_readdata_waitnext_until (dbc, oid, demoobj.base.committed_ts, &demoobj, sizeof(demoobj),
               // HIER: max. 1.1 Sekunden blockieren
               // TODO-mg: eigentlich sollte man kogmo_rtdb_timestamp_now() nehmen, aber das gibt
               //  in der Simulation Probleme
               kogmo_timestamp_add_secs (kogmo_timestamp_now(), 1.1) );
      if ( olen == -KOGMO_RTDB_ERR_TIMEOUT )
        {
          printf("(timeout)\n");
          continue;
        }
      DIEonERR(olen);
      if ( olen < sizeof(demoobj) )
        continue; // object to small (shouldn't happen - ask the object-writer!)
      kogmo_timestamp_to_string(demoobj.base.data_ts, timestring);
      printf("%s: i[0]=%i, ...\n", timestring, demoobj.ints.intval[0]);
    }

  err = kogmo_rtdb_disconnect(dbc, NULL); DIEonERR(err);

  return 0;
}
