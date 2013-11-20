#include <kogmo_rtdb_internal.h>

char*
kogmo_rtdb_obj_dumpbase_str (kogmo_rtdb_handle_t *db_h, void *data_p)
{
  kogmo_rtdb_subobj_base_t *objbase;
  kogmo_timestamp_string_t tstr, data_tstr;
  kogmo_rtdb_obj_c3_process_info_t pi;
  NEWSTR(buf);

  objbase = (kogmo_rtdb_subobj_base_t*) data_p;

  if ( objbase->size < sizeof(kogmo_rtdb_subobj_base_t) )
    {
      STRPRINTF(buf,"* NO DATA\n");
      return buf;
    }

  kogmo_timestamp_to_string(objbase->committed_ts, tstr);
  kogmo_timestamp_to_string(objbase->data_ts, data_tstr);
  kogmo_rtdb_obj_c3_process_getprocessinfo (db_h, objbase->committed_proc,
                                            objbase->committed_ts, pi);
  STRPRINTF(buf,"* DATA:\n");
  STRPRINTF(buf,"Committed: %s by %s, %lli bytes\n", tstr,
                pi, (long long int)objbase->size);
  STRPRINTF(buf,"Data created: %s\n", data_tstr);

  return buf;
}


char*
kogmo_rtdb_obj_dumphex_str (kogmo_rtdb_handle_t *db_h, void *data_p)
{
  kogmo_rtdb_objsize_t size;
  int i;
  NEWSTR(buf);
  size = ( (kogmo_rtdb_subobj_base_t*) data_p )->size;
  STRPRINTF(buf, "* BINARY DATA:\n");
  for(i=0;i<size;i++) {
   STRPRINTF(buf,"%02x ", ((unsigned char*)data_p)[i]);
  }
  STRPRINTF(buf,"\n");
  return buf;
}




// Functions for Process-Object (internal):


#include <kogmo_rtdb_internal.h>

kogmo_rtdb_objid_t
kogmo_rtdb_obj_c3_process_searchprocessobj (kogmo_rtdb_handle_t *db_h,
                                            kogmo_timestamp_t ts,
                                            kogmo_rtdb_objid_t proc_oid)
{
  kogmo_rtdb_objid_t proclistoid,procoid;

  proclistoid = kogmo_rtdb_obj_searchinfo (db_h, "processes",
                                           KOGMO_RTDB_OBJTYPE_C3_PROCESSLIST,
                                           0, 0, ts, NULL, 1);
DBG("%lli",(long long)proclistoid);
  if ( proclistoid < 0 )
    return 0;

  procoid = kogmo_rtdb_obj_searchinfo (db_h, "",
                                       KOGMO_RTDB_OBJTYPE_C3_PROCESS,
                                       proclistoid, proc_oid, ts, NULL, 1);
  if ( procoid < 0 )
    return 0;

  return procoid;
}

kogmo_rtdb_objid_t
kogmo_rtdb_obj_c3_process_getprocessinfo (kogmo_rtdb_handle_t *db_h,
                                          kogmo_rtdb_objid_t proc_oid,
                                          kogmo_timestamp_t ts,
                                          kogmo_rtdb_obj_c3_process_info_t str)
{
  int err;
  kogmo_rtdb_obj_info_t om;
  kogmo_rtdb_obj_c3_process_t po;
  kogmo_rtdb_objid_t oid;

  memset(str, 0, sizeof(kogmo_rtdb_obj_c3_process_info_t));
  strcpy(str, "?");

  oid = kogmo_rtdb_obj_c3_process_searchprocessobj (db_h, ts, proc_oid);

  if ( oid < 0 )
    return oid;

  err = kogmo_rtdb_obj_readinfo ( db_h, oid, ts, &om);
  if ( err < 0 )
    return err;

  err = kogmo_rtdb_obj_readdata (db_h, oid, ts, &po, sizeof (po) );
  if ( err < 0 )
    return err;

  sprintf(str, "%s%s(%lli)", om.deleted_ts ? "D!":"", om.name,
               (long long int) om.oid);

  //sprintf(str, "%s%s [OID %lli, PID %i]", om.deleted_ts ? "D!":"", om.name,
  //             (long long int) om.oid,
  //             po.process.pid);

  return oid;
}
