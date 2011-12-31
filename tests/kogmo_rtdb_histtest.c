/*! \file kogmo_rtdb_histtest.c
 * \brief Testprogram for History
 *
 * Copyright (c) 2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdio.h> /* printf */
#include <unistd.h> /* sleep,getpid */
#include <stdlib.h> /* exit */
#include "kogmo_rtdb.h"

#define DIEonERR(value) if (value<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-value);exit(1);}

int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_obj_info_t obj_info;
  kogmo_rtdb_obj_c3_ints256_t obj, *obj_p;
  kogmo_timestamp_t d[4], c[4], t0;
  kogmo_rtdb_objid_t oid;
  int ok = 1;
  int err;
  int i;

  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "history-test", 0.1); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);

  err = kogmo_rtdb_obj_initinfo (dbc, &obj_info, "history-test-object", KOGMO_RTDB_OBJTYPE_C3_INTS, sizeof (obj)); DIEonERR(err);
  oid = kogmo_rtdb_obj_insert (dbc, &obj_info); DIEonERR(oid);
  err = kogmo_rtdb_obj_initdata (dbc, &obj_info, &obj); DIEonERR(err);

  t0 = kogmo_rtdb_timestamp_now(dbc);

if(0)
  for(i=0;i<4;i++)
    {
      d[i] = obj.base.data_ts = kogmo_rtdb_timestamp_now(dbc);
      obj.ints.intval[0] = i;
      err = kogmo_rtdb_obj_writedata (dbc, obj_info.oid, &obj); DIEonERR(err);
      c[i] = obj.base.committed_ts;
    }
else
  for(i=0;i<4;i++)
    {
      err = kogmo_rtdb_obj_writedata_ptr_begin (dbc, obj_info.oid, &obj_p); DIEonERR(err);
      d[i] = obj_p->base.data_ts = kogmo_rtdb_timestamp_now(dbc);
      obj_p->base.size = sizeof (obj);
      obj_p->ints.intval[0] = i;
      err = kogmo_rtdb_obj_writedata_ptr_commit (dbc, obj_info.oid, &obj_p); DIEonERR(err);
      c[i] = obj_p->base.committed_ts;
    }

#define PRT(str,ref) do { \
                 printf("%s: X=%i C=%f D=%f\n", str, obj.ints.intval[0], \
                 kogmo_timestamp_diff_secs (t0, obj.base.committed_ts), \
                 kogmo_timestamp_diff_secs (t0, obj.base.data_ts     ) ); \
                 if ( obj.ints.intval[0] != ref ) {printf(" => ERROR: X=%i != %i !!!\n",obj.ints.intval[0],ref);ok=0;} \
                 } while(0)

  err = kogmo_rtdb_obj_readdata (dbc, obj_info.oid, 0, &obj, sizeof(obj)); DIEonERR(err); PRT("read latest",3);

  printf(              "readdata:\n");
  err = kogmo_rtdb_obj_readdata (dbc, obj_info.oid, c[2], &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]",2);
  err = kogmo_rtdb_obj_readdata (dbc, obj_info.oid, c[2]+1, &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]+1",2);
  err = kogmo_rtdb_obj_readdata (dbc, obj_info.oid, c[2]-1, &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]-1",1);
  printf(              "readdata_older:\n");
  err = kogmo_rtdb_obj_readdata_older (dbc, obj_info.oid, c[2], &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]",1);
  err = kogmo_rtdb_obj_readdata_older (dbc, obj_info.oid, c[2]+1, &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]+1",2);
  err = kogmo_rtdb_obj_readdata_older (dbc, obj_info.oid, c[2]-1, &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]-1",1);
  printf(              "readdata_younger:\n");
  err = kogmo_rtdb_obj_readdata_younger (dbc, obj_info.oid, c[2], &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]",3);
  err = kogmo_rtdb_obj_readdata_younger (dbc, obj_info.oid, c[2]+1, &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]+1",3);
  err = kogmo_rtdb_obj_readdata_younger (dbc, obj_info.oid, c[2]-1, &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]-1",2);

  printf(              "readdata_datatime:\n");
  err = kogmo_rtdb_obj_readdata_datatime (dbc, obj_info.oid, d[2], &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]",2);
  err = kogmo_rtdb_obj_readdata_datatime (dbc, obj_info.oid, d[2]+1, &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]+1",2);
  err = kogmo_rtdb_obj_readdata_datatime (dbc, obj_info.oid, d[2]-1, &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]-1",1);
  printf(              "readdata_dataolder:\n");
  err = kogmo_rtdb_obj_readdata_dataolder (dbc, obj_info.oid, d[2], &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]",1);
  err = kogmo_rtdb_obj_readdata_dataolder (dbc, obj_info.oid, d[2]+1, &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]+1",2);
  err = kogmo_rtdb_obj_readdata_dataolder (dbc, obj_info.oid, d[2]-1, &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]-1",1);
  printf(              "readdata_datayounger:\n");
  err = kogmo_rtdb_obj_readdata_datayounger (dbc, obj_info.oid, d[2], &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]",3);
  err = kogmo_rtdb_obj_readdata_datayounger (dbc, obj_info.oid, d[2]+1, &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]+1",3);
  err = kogmo_rtdb_obj_readdata_datayounger (dbc, obj_info.oid, d[2]-1, &obj, sizeof(obj)); DIEonERR(err); PRT("read [2]-1",2);

  err = kogmo_rtdb_obj_delete (dbc, &obj_info); DIEonERR(err);
  err = kogmo_rtdb_disconnect (dbc, NULL); DIEonERR(err);

  if ( !ok )
    {
      printf("\nWARNING: THERE WERE ERRORS!!!\n\n");
      return 1;
    }

  return 0;
}
