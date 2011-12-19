/*! \file kogmo_rtdb_lister.c
 * \brief Example for reading a list of objects from the RTDB
 *
 * (c) 2007 Matthias Goebl <mg@tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdio.h> /* printf */
#include <unistd.h> /* getpid */
#include <stdlib.h> /* exit */
#include <string.h>
#include "kogmo_rtdb.h"

#define DIEonERR(value) if (value<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-value);exit(1);}

int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_obj_info_t obj_info;
  kogmo_rtdb_objid_t oid;
  kogmo_rtdb_objid_list_t oidlist;
  int err,i;
  
  // Verbindung zur Datenbank aufbauen
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "kogmo_rtdb_lister", 1.0); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);


  // Liste aller Objekte eines Typs holen (hier z.B. die "Prozess"-Objekte
  oid = kogmo_rtdb_obj_searchinfo (dbc, NULL,
    KOGMO_RTDB_OBJTYPE_C3_PROCESS, 0, 0, 0, oidlist, 0); DIEonERR(oid);


  // Namen aller gefundenen Objekte anzeigen
  // (dazu muss man erst den Infoblock des Objektes holen)
  i=-1;
  while ( oidlist[++i] )
    {
      err = kogmo_rtdb_obj_readinfo (dbc, oidlist[i], 0, &obj_info);
      if ( err <0 )
        strcpy(obj_info.name, "?");
      printf("* %lli\t%s\n", (long long int)oidlist[i], obj_info.name);
    }

  err = kogmo_rtdb_disconnect(dbc, NULL); DIEonERR(err);

  return 0;
}
