/*! \file exampleobj_writer.c
 * \brief Example for writing the ExampleObject
 *
 * (c) 2005-2009 Matthias Goebl <mg@tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdio.h> /* printf */
#include <unistd.h> /* sleep,getpid */
#include <stdlib.h> /* exit */
#include <math.h> /* sin,cos */

//#define KOGMO_RTDB_DONT_INCLUDE_ALL_OBJECTS // if you define this, you must also include exampleobj_rtdbobj.h!
#include "kogmo_rtdb.h"
//#include "exampleobj_rtdbobj.h"

#define DIEonERR(value) if (value<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-value);exit(1);}

int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_obj_info_t exampleobj_info;
  kogmo_rtdb_obj_ExampleObj_t exampleobj;
  kogmo_rtdb_objid_t oid;
  int err, i;
  
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "example_object_writer", 0.033); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);

  err = kogmo_rtdb_obj_initinfo (dbc, &exampleobj_info, "example_object", KOGMO_RTDB_OBJTYPE_ExampleObj, sizeof (exampleobj)); DIEonERR(err);
  oid = kogmo_rtdb_obj_insert (dbc, &exampleobj_info); DIEonERR(oid);

  err = kogmo_rtdb_obj_initdata (dbc, &exampleobj_info, &exampleobj); DIEonERR(err);

  exampleobj.data.x = 123;
  exampleobj.data.y = 456;

  for ( i=1; ; i++ )
    {
      exampleobj.base.data_ts = kogmo_timestamp_now();
      exampleobj.data.i = i;
      exampleobj.data.f = 1.0/i;

      err = kogmo_rtdb_obj_writedata (dbc, exampleobj_info.oid, &exampleobj); DIEonERR(err);

      sleep (1);
    }

  err = kogmo_rtdb_obj_delete (dbc, &exampleobj_info); DIEonERR(err);

  err = kogmo_rtdb_disconnect (dbc, NULL); DIEonERR(err);

  return 0;
}
