/*! \file rtdbstat.c
 * \brief Check status of the RTDB
 *
 * (c) 2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdio.h> /* printf */
#include <stdlib.h> /* exit */
#include "kogmo_rtdb.h"

int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_objid_t oid;
  
  kogmo_rtdb_connect_initinfo (&dbinfo, "", "rtdbstat", 1);
  dbinfo.flags = KOGMO_RTDB_CONNECT_FLAGS_TRYNOWAIT | KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR;
  oid = kogmo_rtdb_connect (&dbc, &dbinfo);
  if ( oid < 0 )
    {
      printf("ERROR: cannot connect to RTDB\n");
      exit(1);
    }

  printf("OK: connection to RTDB ok.\n");

  kogmo_rtdb_disconnect(dbc, NULL);

  return 0;
}
