/*! \file kogmo_rtdb_slotreader.c
 * \brief Example for reading from the RTDB slot-by-slot
 *
 * (c) 2005-2009 Matthias Goebl <mg@tum.de>
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
  kogmo_rtdb_obj_slot_t demoobj_slot;
  int err;
  
  // Verbindung zur Datenbank aufbauen, unsere Zykluszeit is 33 ms
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "demo_object_reader", 0.033); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);

  // Auf Demo-Objekt warten
  oid = kogmo_rtdb_obj_searchinfo_wait (dbc, "demo_object",
    KOGMO_RTDB_OBJTYPE_C3_INTS, 0, 0); DIEonERR(oid);

  // Infoblock des Objektes holen (wenn Metadaten benoetigt werden)
  err = kogmo_rtdb_obj_readinfo (dbc, oid, 0, &demoobj_info); DIEonERR(err);

  // Objekt erstmalig holen (initialer Slot)
  olen = kogmo_rtdb_obj_readdataslot_init (dbc, &demoobj_slot, oid); DIEonERR(olen);

  while (1)
    {
      kogmo_timestamp_string_t timestring;
      olen = kogmo_rtdb_obj_readdataslot_relative (dbc, &demoobj_slot, +1, &demoobj, sizeof(demoobj));
      if ( olen == -KOGMO_RTDB_ERR_TOOFAST )
        {
          // the reader already read the latest slot and now tried t
          // to read the next slot, that is not yet written
          printf(".\n");
          usleep(500000);
          continue;
        }
      if ( olen == -KOGMO_RTDB_ERR_HISTWRAP )
        {
          printf("History Wrap-around -> reader too slow -> automatically restarted at current slot -> loosing data of some slots!\n");
          continue;
        }
      if ( olen < (int)sizeof(demoobj) )
        {
          printf("Error: object to small. this shouldn't happen - somebody wrote data with the wrong size!\n");
          continue;
        }
      kogmo_timestamp_to_string(demoobj.base.data_ts, timestring);
      printf("%s: i[0]=%i, ...\n", timestring, demoobj.ints.intval[0]);
    }

  err = kogmo_rtdb_disconnect(dbc, NULL); DIEonERR(err);

  return 0;
}
