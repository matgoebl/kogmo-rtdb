/*! \file textreader_multithread.c
 * \brief Example for reading from the RTDB with multiple threads
 *
 * (c) 2005-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdio.h> /* printf */
#include <unistd.h> /* getpid */
#include <stdlib.h> /* exit */
#include <pthread.h>
#include "kogmo_rtdb.h"

#define DIEonERR(value) if (value<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-value);exit(1);}

char *name = "testtext";

pthread_t tid1=0;
pthread_t tid2=0;

void *
read_thread (void *dummy)
{
  kogmo_rtdb_obj_c3_text_t textobj;
  kogmo_rtdb_objid_t oid;
  kogmo_rtdb_objsize_t olen;
  int textsize;

  int err;
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "textreader", 1.0); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);


  oid = kogmo_rtdb_obj_searchinfo_wait (dbc, name, KOGMO_RTDB_OBJTYPE_C3_TEXT, 0, 0); DIEonERR(oid);
  textobj.base.committed_ts = 0;

  while (1)
    {
      olen = kogmo_rtdb_obj_readdata_waitnext (dbc, oid, textobj.base.committed_ts, &textobj, sizeof(textobj)); DIEonERR(olen);
      textsize = textobj.base.size - sizeof(textobj.base);
      if ( textsize >= C3_TEXT_MAXCHARS )
        textsize = C3_TEXT_MAXCHARS-1;
      textobj.text.data[textsize]='\0'; // zur sicherheit null-terminieren
      printf("Got Text: %s\n",textobj.text.data);
    }
}


int
main (int argc, char **argv)
{
  int err;
  if(argc>=2) name = argv[1];

  err=pthread_create(&tid1, NULL, &read_thread, NULL);
  err=pthread_create(&tid2, NULL, &read_thread, NULL);
  err=pthread_join(tid1, NULL);
  err=pthread_join(tid2, NULL);

  return 0;
}
