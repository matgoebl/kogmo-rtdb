/*! \file textreader.c
 * \brief Example for reading text (also XML) from the RTDB
 *
 * (c) 2005-2007 Matthias Goebl <mg@tum.de>
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
  kogmo_rtdb_obj_info_t textobj_info;
  kogmo_rtdb_obj_c3_text_t textobj;         
  kogmo_rtdb_objid_t oid;
  kogmo_rtdb_objsize_t olen;
  kogmo_timestamp_string_t timestring;
  int textsize;
  int err;
  char *name = "testtext";
  if(argc>=2) name = argv[1];

  // Verbindung zur Datenbank aufbauen, unsere Zykluszeit ist 1 s
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "textreader", 1.0); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);

  // Auf Object warten
  oid = kogmo_rtdb_obj_searchinfo_wait (dbc, name,
    KOGMO_RTDB_OBJTYPE_C3_TEXT, 0, 0); DIEonERR(oid);

  // Infoblock des Objektes holen und anzeigen
  err = kogmo_rtdb_obj_readinfo (dbc, oid, 0, &textobj_info); DIEonERR(err);

  textobj.base.committed_ts = 0;

  while (1)
    {
      olen = kogmo_rtdb_obj_readdata_waitnext (dbc, oid, textobj.base.committed_ts, &textobj, sizeof(textobj)); DIEonERR(olen);
      kogmo_timestamp_to_string(textobj.base.data_ts, timestring);
      textsize = textobj.base.size - sizeof(textobj.base);
      if ( textsize >= C3_TEXT_MAXCHARS )
        textsize = C3_TEXT_MAXCHARS-1;
      textobj.text.data[textsize]='\0'; // zur sicherheit null-terminieren
      printf("--- %s - %s ---\n%s\n", textobj_info.name, timestring, textobj.text.data);
    }

  err = kogmo_rtdb_disconnect(dbc, NULL); DIEonERR(err);

  return 0;
}
