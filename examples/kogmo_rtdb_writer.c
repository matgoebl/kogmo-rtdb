/*! \file kogmo_rtdb_writer.c
 * \brief Example for writing to the RTDB
 *
 * (c) 2005,2006 Matthias Goebl <matthias.goebl*goebl.net>
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

int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_obj_info_t demoobj_info;
  kogmo_rtdb_obj_c3_ints256_t demoobj;
  kogmo_rtdb_objid_t oid;
  int err, i;
  
  // Verbindung zur Datenbank aufbauen, unsere Zykluszeit is 33 ms
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "demo_object_writer", 0.033); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);

  // Demo-Object erstellen
  err = kogmo_rtdb_obj_initinfo (dbc, &demoobj_info,
    "demo_object", KOGMO_RTDB_OBJTYPE_C3_INTS, sizeof (demoobj)); DIEonERR(err);
  demoobj_info.max_cycletime = demoobj_info.avg_cycletime = 1;
  demoobj_info.history_interval = 3;
  oid = kogmo_rtdb_obj_insert (dbc, &demoobj_info); DIEonERR(oid);

  // Datenobjekt initialisieren
  err = kogmo_rtdb_obj_initdata (dbc, &demoobj_info, &demoobj); DIEonERR(err);

  // Fixe Werte eintragen
  demoobj.ints.intval[0] = 0;

  for ( i=1; ; i++ )
    {
      // Von welchem Zeitpunkt sind die Daten?
      demoobj.base.data_ts = kogmo_timestamp_now();
      // Nun die eigentlichen Daten (Achtung: sinnlos! Nur zur Demo!)
      demoobj.ints.intval[0] = i;

      // Daten schreiben
      err = kogmo_rtdb_obj_writedata (dbc, demoobj_info.oid, &demoobj); DIEonERR(err);

      sleep (1);
    }

  err = kogmo_rtdb_obj_delete (dbc, &demoobj_info); DIEonERR(err);

  err = kogmo_rtdb_disconnect (dbc, NULL); DIEonERR(err);

  return 0;
}
