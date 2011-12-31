/*! \file kogmo_rtdb_watcher.c
 * \brief Example for following object additions and deletions (Very simple - displays only ids..)
 *
 * (c) 2005,2006 Matthias Goebl <matthias.goebl*goebl.net>
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
  int err, ret, i;
  kogmo_rtdb_objid_list_t known_idlist;
  kogmo_rtdb_objid_list_t added_idlist;
  kogmo_rtdb_objid_list_t deleted_idlist;
  known_idlist[0]=0;

  // Verbindung zur Datenbank aufbauen, unsere Zykluszeit is 33 ms
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "c3_rtdb_watcher", 1); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);

  while(1) {
    ret = kogmo_rtdb_obj_searchinfo_waitnext_until(dbc, NULL, 0, 0, 0,
                       known_idlist, added_idlist, deleted_idlist,
                       kogmo_timestamp_add_secs (kogmo_timestamp_now(), 1.5) );
    if ( ret == -KOGMO_RTDB_ERR_TIMEOUT )
      {
        printf("(timeout, nothing happened)\n");
        continue;
      }
    DIEonERR(ret);
    printf("=%i\n",ret);
    i=-1;
    while ( added_idlist[++i] )
      printf("+ %lli\n", (long long int)added_idlist[i]);
    i=-1;
    while ( deleted_idlist[++i] )
      printf("- %lli\n", (long long int)deleted_idlist[i]);

    i=-1;
    while ( known_idlist[++i] )
      {
        err = kogmo_rtdb_obj_readinfo (dbc, known_idlist[i], 0, &obj_info);
        if ( err <0 )
          strcpy(obj_info.name, "?");
        printf("* %lli\t%s\n", (long long int)known_idlist[i], obj_info.name);
      }
  }

  err = kogmo_rtdb_disconnect(dbc, NULL); DIEonERR(err);

  return 0;
}
