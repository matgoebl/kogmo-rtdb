/*! \file textwriter.c
 * \brief Example for writing text (also XML) to the RTDB
 *
 * (c) 2005-2007 Matthias Goebl <mg@tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdio.h> /* printf */
#include <unistd.h> /* sleep,getpid */
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
  kogmo_rtdb_obj_info_t textobj_info;
  kogmo_rtdb_obj_c3_text1k_t textobj;
  kogmo_rtdb_objid_t oid;
  int err;
  char *name = "testtext";   
  if(argc>=2) name = argv[1];
  
  // Verbindung zur Datenbank aufbauen, unsere Zykluszeit is 1 s
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "textwriter", 1.0); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);

  err = kogmo_rtdb_obj_initinfo (dbc, &textobj_info,
    name, KOGMO_RTDB_OBJTYPE_C3_TEXT, sizeof (textobj)); DIEonERR(err);
  textobj_info.history_interval = 10;
  textobj_info.avg_cycletime = 1;
  textobj_info.max_cycletime = 1;
  oid = kogmo_rtdb_obj_insert (dbc, &textobj_info); DIEonERR(oid);

  // Datenobjekt initialisieren
  err = kogmo_rtdb_obj_initdata (dbc, &textobj_info, &textobj); DIEonERR(err);

  //strcpy(textobj.text.data,"Hello World!");
  //textobj.base.size = sizeof(textobj.base) + strlen(textobj.text.data);
  //textobj.base.data_ts = kogmo_timestamp_now();
  //err = kogmo_rtdb_obj_writedata (dbc, textobj_info.oid, &textobj); DIEonERR(err);
  //sleep (10);

  while ( fgets(textobj.text.data, sizeof(textobj.text.data), stdin) != NULL )
    {
      textobj.base.size = sizeof(textobj.base) + strlen(textobj.text.data);
      textobj.base.data_ts = kogmo_timestamp_now();
      err = kogmo_rtdb_obj_writedata (dbc, textobj_info.oid, &textobj); DIEonERR(err);
    }

  err = kogmo_rtdb_obj_delete (dbc, &textobj_info); DIEonERR(err);

  err = kogmo_rtdb_disconnect (dbc, NULL); DIEonERR(err);

  return 0;
}
