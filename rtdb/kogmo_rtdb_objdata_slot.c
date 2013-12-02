/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file kogmo_rtdb_objdata_slot.c
 * \brief Management Functions for Object Data in the RTDB
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include "kogmo_rtdb_internal.h"



kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdataslot_ptr (kogmo_rtdb_handle_t *db_h,
// mode: 0=get latest + init objslot,
//      -1=use offset for reading (don't check old objslot)
//       1=check old objslot + use offset for reading
                                 int32_t mode, int32_t offset,
                                 kogmo_rtdb_obj_slot_t *objslot,
                                 void *data_pp)
{
  kogmo_rtdb_obj_info_t *scan_objmeta_p;
  kogmo_rtdb_subobj_base_t *scan_objbase_p, *last_scan_objbase_p = NULL;
  volatile kogmo_timestamp_t scan_ts,final_ts;

  CHK_DBH("kogmo_rtdb_obj_readdataslot_ptr",db_h,0);
  CHK_PTR(objslot);

  if ( mode == 0 )
    {
        // TODO: optimize
        scan_objmeta_p = kogmo_rtdb_obj_findmeta_byid (db_h, objslot->oid );
        if ( scan_objmeta_p == NULL )
          return -KOGMO_RTDB_ERR_NOTFOUND;
        objslot->object_slot = kogmo_rtdb_obj_slotnum (db_h, scan_objmeta_p );
        objslot->history_slot =  scan_objmeta_p->history_slot;
        if ( objslot->history_slot < 0 ) // no writes yet
          return -KOGMO_RTDB_ERR_NOTFOUND;
        objslot->committed_ts = invalid_ts;
        offset = 0;
    }
  else
    {
      // now objslot->object_slot must be valid
      if ( objslot->object_slot < 0 || objslot->object_slot >= KOGMO_RTDB_OBJ_MAX )
        return -KOGMO_RTDB_ERR_INVALID;

      scan_objmeta_p = &db_h->localdata_p->objmeta[objslot->object_slot];

      if ( objslot->oid != scan_objmeta_p ->oid )
        return -KOGMO_RTDB_ERR_NOTFOUND;
    }

  if ( mode == 1 )
    {
      // remember old position with committed_ts
      last_scan_objbase_p = (kogmo_rtdb_subobj_base_t *)
                 & ( db_h->localdata_p->heap [
                                                 scan_objmeta_p->buffer_idx +
                                                 objslot->history_slot * scan_objmeta_p->size_max
                                               ] );
    }

  // calculate next slot position
  objslot->history_slot = ( objslot->history_slot + ( offset % scan_objmeta_p->history_size ) + scan_objmeta_p->history_size )
                          % scan_objmeta_p->history_size;

  // calculate object data pointer from history slot number
  scan_objbase_p = (kogmo_rtdb_subobj_base_t *)
             & ( db_h->localdata_p->heap [
                                             scan_objmeta_p->buffer_idx +
                                             objslot->history_slot * scan_objmeta_p->size_max
                                           ] );

  COPY_INT64_HIGHFIRST( scan_ts, scan_objbase_p->committed_ts);

  if ( scan_ts == invalid_ts )
    {
      return -KOGMO_RTDB_ERR_NOTFOUND;
    }

  if ( mode == 1 && last_scan_objbase_p != NULL )
    {
      // check old position against known committed_ts
      COPY_INT64_LOWFIRST( final_ts, last_scan_objbase_p->committed_ts );
      if ( final_ts != objslot->committed_ts )
        return -KOGMO_RTDB_ERR_HISTWRAP;

      // slot no longer existant, exceeded tail
      if ( offset < 0 && scan_ts > objslot->committed_ts )
        return -KOGMO_RTDB_ERR_HISTWRAP;

      // not yet existant, exceeded head
      if ( offset > 0 && scan_ts < objslot->committed_ts )
        return -KOGMO_RTDB_ERR_TOOFAST;
    }

  objslot->committed_ts = scan_ts;
  if ( data_pp != NULL )
    *(kogmo_rtdb_subobj_base_t**) data_pp = scan_objbase_p;
  return scan_objbase_p->size; // return real size;
}



kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdataslot_init (kogmo_rtdb_handle_t *db_h,
                                  kogmo_rtdb_obj_slot_t *objslot,
                                  kogmo_rtdb_objid_t oid)
{
  kogmo_rtdb_objsize_t olen;
  CHK_DBH("kogmo_rtdb_obj_readdataslot_init",db_h,0);
  CHK_PTR(objslot);

  objslot->oid = oid;
  olen = kogmo_rtdb_obj_readdataslot_ptr (db_h, 0, 0, objslot, NULL);
  DBG("kogmo_rtdb_obj_readdataslot_init(oid %lli) returns %lli", (long long int)oid, (long long int)olen);
  return olen;
}

// kogmo_rtdb_obj_readdataslot_relative() makes sure, that no history-wrap occured by re-checking the last known slot
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdataslot_relative (kogmo_rtdb_handle_t *db_h,
                                      kogmo_rtdb_obj_slot_t *objslot,
                                      int offset,
                                      void *data_p,
                                      kogmo_rtdb_objsize_t size)
{
  kogmo_rtdb_objsize_t olen,realsize;
  void *objdata_p;

  CHK_DBH("kogmo_rtdb_obj_readdataslot",db_h,0);
  CHK_PTR(objslot);
  CHK_PTR(data_p);

  olen = kogmo_rtdb_obj_readdataslot_ptr (db_h, 1, offset, objslot, &objdata_p);
  if ( olen < 0 )
    {
      DBG("kogmo_rtdb_obj_readdataslot(oid %lli, offset %i) returns %lli", (long long int)objslot->oid, offset, (long long int)olen);
      kogmo_rtdb_obj_readdataslot_ptr (db_h, 0, 0, objslot, NULL); // re-init
      return olen;
    }
  realsize = olen;
  memcpy(data_p, objdata_p, realsize < size ? realsize : size);
  olen = kogmo_rtdb_obj_readdataslot_ptr (db_h, 1, 0, objslot, NULL);
  if ( olen < 0 )
    {
      DBG("kogmo_rtdb_obj_readdataslot(oid %lli, offset %i) returns %lli", (long long int)objslot->oid, offset, (long long int)olen);
      kogmo_rtdb_obj_readdataslot_ptr (db_h, 0, 0, objslot, NULL); // re-init
      return olen;
    }
  DBG("kogmo_rtdb_obj_readdataslot_next(oid %lli, offset %i) returns %lli at slot %i", (long long int)objslot->oid, offset, (long long int)realsize, objslot->history_slot);
  return realsize;
}


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdataslot_relative_ptr (kogmo_rtdb_handle_t *db_h,
                                          kogmo_rtdb_obj_slot_t *objslot,
                                          int offset,
                                          void *data_pp)
{
  kogmo_rtdb_objsize_t olen;
  void *objdata_p;

  CHK_DBH("kogmo_rtdb_obj_readdataslot_ptr",db_h,0);
  CHK_PTR(objslot);
  CHK_PTR(data_pp);

  olen = kogmo_rtdb_obj_readdataslot_ptr (db_h, 1, offset, objslot, &objdata_p);
  if ( olen < 0 )
    {
      DBG("kogmo_rtdb_obj_readdataslot_next(oid %lli, offset %i) returns %lli", (long long int)objslot->oid, offset, (long long int)olen);
      kogmo_rtdb_obj_readdataslot_ptr (db_h, 0, 0, objslot, NULL); // re-init
      return olen;
    }
  *(kogmo_rtdb_subobj_base_t**) data_pp = objdata_p;
  //DBG("kogmo_rtdb_obj_readdataslot_next(oid %lli, offset %i) returns %lli at slot %i", (long long int)objslot->oid, offset, (long long int)realsize, objslot->history_slot);
  return olen;
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdataslot_check (kogmo_rtdb_handle_t *db_h,
                                   kogmo_rtdb_obj_slot_t *objslot)
{
  kogmo_rtdb_objsize_t olen;

  CHK_DBH("kogmo_rtdb_obj_readdataslot_ptr",db_h,0);
  CHK_PTR(objslot);

  olen = kogmo_rtdb_obj_readdataslot_ptr (db_h, 1, 0, objslot, NULL);
  //DBG("kogmo_rtdb_obj_readdataslot_next(oid %lli, offset %i) returns %lli at slot %i", (long long int)objslot->oid, offset, (long long int)realsize, objslot->history_slot);
  return olen;
}
