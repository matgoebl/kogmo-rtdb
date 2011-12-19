/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */
/*! \file kogmo_rtdb_objdata.c
 * \brief Management Functions for Object Data in the RTDB
 *
 * Copyright (c) 2003-2006 Matthias Goebl <mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */
 
#include "kogmo_rtdb_internal.h"

const volatile kogmo_timestamp_t invalid_ts = 0;

int
kogmo_rtdb_obj_initdata (kogmo_rtdb_handle_t *db_h,
                         kogmo_rtdb_obj_info_t *metadata_p,
                         void *data_p)
{
  CHK_DBH("kogmo_rtdb_obj_initdata",db_h,0);
  CHK_PTR(metadata_p);
  CHK_PTR(data_p);
  memset (data_p, 0, metadata_p->size_max);
  ( (kogmo_rtdb_subobj_base_t*) data_p ) -> size = metadata_p->size_max;
  return 0;
}


// internal:
inline static void *
kogmo_rtdb_obj_histscan (kogmo_rtdb_handle_t *db_h, int32_t adj,
                         kogmo_rtdb_obj_info_t *scan_objmeta_p,
                         int32_t *currslot, int32_t *firstslot);


int
kogmo_rtdb_obj_writedata (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_objid_t oid, void *data_p)
{
  kogmo_rtdb_obj_info_t *used_objmeta_p;
  int32_t history_slot;
  kogmo_rtdb_objsize_t size;
  volatile kogmo_timestamp_t committed_ts;
  int no_notifies;
  void *heap_data_p;

  committed_ts = kogmo_rtdb_timestamp_now (db_h);

  CHK_DBH("kogmo_rtdb_obj_writedata",db_h,0);
  CHK_PTR(data_p);    

  size = ( (kogmo_rtdb_subobj_base_t*) data_p ) -> size;

  DBGL (DBGL_API,"kogmo_rtdb_obj_writedata(oid %i, data %p, size %i)", oid, data_p, size);

  used_objmeta_p = kogmo_rtdb_obj_findmeta_byid (db_h, oid);
  if ( used_objmeta_p == NULL ) return -KOGMO_RTDB_ERR_NOTFOUND;
  if ( used_objmeta_p -> buffer_idx == 0 ) return -KOGMO_RTDB_ERR_NOTFOUND;

  // better fail: if ( size == 0 ) size = used_objmeta_p -> size_max;
  if ( size > used_objmeta_p -> size_max ) return -KOGMO_RTDB_ERR_INVALID;
  if ( size < sizeof ( kogmo_rtdb_subobj_base_t ) ) return -KOGMO_RTDB_ERR_INVALID;

  if ( !used_objmeta_p -> flags.write_allow
    && used_objmeta_p -> created_proc != db_h->ipc_h.this_process.proc_oid
    && !this_process_is_admin (db_h) )
    {
      DBGL (DBGL_MSG,"commit permission denied for oid %d", used_objmeta_p -> oid);
      return -KOGMO_RTDB_ERR_NOPERM;
    }

  // Update Base Data
  ((kogmo_rtdb_subobj_base_t *) data_p) -> committed_proc = db_h->ipc_h.this_process.proc_oid;

  // lock object, if concurrent writes are allowed
  if ( used_objmeta_p -> flags.write_allow )
    kogmo_rtdb_obj_lock (db_h, used_objmeta_p);

  //if(DEBUG)
  //  if(getenv("KOGMO_RTDB_DEBUG_TEST_BLOCKTIME_WRITE"))
  //    usleep(atof(getenv("KOGMO_RTDB_DEBUG_TEST_BLOCKTIME_WRITE"))*1000000);

  if ( used_objmeta_p -> flags.cycle_watch )
    {
      kogmo_rtdb_subobj_base_t *scan_objbase;
      float min_cycle_time = MIN_CYCLETIME(used_objmeta_p -> min_cycletime, used_objmeta_p -> max_cycletime);
      // the latest data
      scan_objbase = kogmo_rtdb_obj_histscan (db_h, 0, used_objmeta_p, NULL, NULL);
      if ( scan_objbase != NULL ) // there have been previous commits..
        {
          float time_since_last_commit = kogmo_timestamp_diff_secs (scan_objbase->committed_ts, committed_ts);
          DBGL (DBGL_MSG,"cycle-watch: time_since_last_commit %f < %f ?", time_since_last_commit, min_cycle_time);
          if ( time_since_last_commit < min_cycle_time )
            {
              if ( used_objmeta_p -> flags.write_allow )
                kogmo_rtdb_obj_unlock (db_h, used_objmeta_p);
              DBGL (DBGL_MSG,"updates too fast for oid %d: %f < %f", used_objmeta_p -> oid, time_since_last_commit, min_cycle_time);
              return -KOGMO_RTDB_ERR_TOOFAST;
            }
        }
    }

  no_notifies = used_objmeta_p -> flags.no_notifies | db_h->localdata_p->flags.no_notifies;

  history_slot = used_objmeta_p -> history_slot;
  if ( history_slot < 0 )
    {
      history_slot = 0;
    }
  else
    {
      history_slot = ( history_slot + 1 ) % used_objmeta_p -> history_size;
    }

  heap_data_p = &db_h->localdata_p -> heap
                  [ used_objmeta_p -> buffer_idx
                  + history_slot * used_objmeta_p -> size_max ];

  // committed_ts makes an entry valid(>0) / invalid(==0)
  // 2. copy committed_ts as 0 (entry invalid)
  ((kogmo_rtdb_subobj_base_t *) data_p) -> committed_ts = invalid_ts;

  // a new committed_ts indicates, that an entry changed
  // 3. make sure, that a new committed_ts is never equal to the previous
  //     one in the ringbuffer-tail
  // --> this wont happen as we have nanosecond ticks and a deep buffer
  // but if, this is a work-around (as this should not occur too often)
  // -> warn, if the old committed_ts is equal to the new
  if ( committed_ts ==
         ((kogmo_rtdb_subobj_base_t *) heap_data_p) -> committed_ts
       && ! db_h->localdata_p->flags.simmode )
    {
      committed_ts++; // wrong by 1 nanosecond, this may increase...
      DBG("warning: obj_commit(%i) forged commit-time(+1) to avoid duplicates!", oid);
    }

  // 4. make slot invalid
  COPY_INT64_HIGHFIRST( (((kogmo_rtdb_subobj_base_t *) heap_data_p) -> committed_ts), invalid_ts);

  // 5. copy data with committed_ts==0
  memcpy ( heap_data_p, data_p, size );

  // 5b. if there is no data_ts, set it to committed_ts
  if ( ((kogmo_rtdb_subobj_base_t *) heap_data_p) -> data_ts == invalid_ts )
    ((kogmo_rtdb_subobj_base_t *) heap_data_p) -> data_ts = committed_ts;

  // 5c. update size (at least at its minimum)
  // ((kogmo_rtdb_subobj_base_t *) heap_data_p) -> size = size;

  // pre-6. block new notify-listeners unless notifies are disabled
  if ( ! no_notifies )
    kogmo_rtdb_obj_do_notify_prepare(db_h, used_objmeta_p);
  //DBG("obj slot: %i",kogmo_rtdb_obj_slotnum (db_h, used_objmeta_p));

  // 6. make slot valid with new committed_ts
  COPY_INT64_LOWFIRST( ((kogmo_rtdb_subobj_base_t *) heap_data_p) -> committed_ts, committed_ts);

  // 7. set pointer to this slot
  used_objmeta_p -> history_slot = history_slot;

  if ( used_objmeta_p -> flags.write_allow )
    kogmo_rtdb_obj_unlock (db_h, used_objmeta_p);

  // 8. send notifies unless notifies are disabled
  if ( ! no_notifies )
    kogmo_rtdb_obj_do_notify (db_h, used_objmeta_p);

  // set my own commited_ts back to the original value
  ((kogmo_rtdb_subobj_base_t *) data_p) -> committed_ts = committed_ts;

  DBGL (DBGL_API,"obj_committed oid %i into slot %i with %i bytes at ts=%lli",
        oid, history_slot, size, (long long int) committed_ts);

  IFDBGL (DBGL_DB+DBGL_VERBOSE)
    {
      char *p;
      p = kogmo_rtdb_obj_dumpbase_str (db_h, data_p);
      printf("{\n%s}\n",p);
      free(p);
      IFDBGL (DBGL_VERYVERBOSE)
      if(0)  {
          p = kogmo_rtdb_obj_dumphex_str (db_h, data_p);
          printf("{\n%s}\n",p);
          free(p);
        }
    }

  kogmo_rtdb_obj_trace_send (db_h, oid, committed_ts, KOGMO_RTDB_TRACE_UPDATED,
        kogmo_rtdb_obj_slotnum (db_h, used_objmeta_p), history_slot);

  return 0;
}


int
kogmo_rtdb_obj_writedata_ptr_begin (kogmo_rtdb_handle_t *db_h,
                                    kogmo_rtdb_objid_t oid,
                                    void *data_pp)
{
  kogmo_rtdb_obj_info_t *used_objmeta_p;
  int32_t history_slot;
  void *heap_data_p;

  CHK_DBH("kogmo_rtdb_obj_writedata_ptr_begin",db_h,0);
  CHK_PTR(data_pp);

  DBGL (DBGL_API,"kogmo_rtdb_obj_writedata_ptr_begin(oid %i, data* %p)", oid, data_pp);

  used_objmeta_p = kogmo_rtdb_obj_findmeta_byid (db_h, oid);
  if ( used_objmeta_p == NULL ) return -KOGMO_RTDB_ERR_NOTFOUND;
  if ( used_objmeta_p -> buffer_idx == 0 ) return -KOGMO_RTDB_ERR_NOTFOUND;

  if ( used_objmeta_p -> flags.write_allow
    || used_objmeta_p -> created_proc != db_h->ipc_h.this_process.proc_oid )
    {
      DBGL (DBGL_MSG,"no ptr-write for public or others objects");
      return -KOGMO_RTDB_ERR_INVALID;
    }

  history_slot = used_objmeta_p -> history_slot;
  if ( history_slot < 0 )
    {
      history_slot = 0;
    }
  else
    {
      history_slot = ( history_slot + 1 ) % used_objmeta_p -> history_size;
    }

  heap_data_p = &db_h->localdata_p -> heap
                  [ used_objmeta_p -> buffer_idx
                  + history_slot * used_objmeta_p -> size_max ];
	
  // make slot invalid
  COPY_INT64_HIGHFIRST( ((kogmo_rtdb_subobj_base_t *) heap_data_p) -> committed_ts, invalid_ts);

  // slightly unaesthetic
  *(kogmo_rtdb_subobj_base_t**) data_pp = (kogmo_rtdb_subobj_base_t*) heap_data_p;

  return 0;
}


int
kogmo_rtdb_obj_writedata_ptr_commit (kogmo_rtdb_handle_t *db_h,
                                    kogmo_rtdb_objid_t oid,
                                    void *data_pp)
{
  kogmo_rtdb_obj_info_t *used_objmeta_p;
  int32_t history_slot;
  kogmo_rtdb_objsize_t size;
  volatile kogmo_timestamp_t committed_ts;
  int no_notifies;
  void *heap_data_p;

  committed_ts = kogmo_rtdb_timestamp_now (db_h);

  CHK_DBH("kogmo_rtdb_obj_writedata",db_h,0);
  CHK_PTR(data_pp);    

  size = ( *(kogmo_rtdb_subobj_base_t**) data_pp ) -> size;

  DBGL (DBGL_API,"kogmo_rtdb_obj_writedata_ptr_commit(oid %i, data* %p, size %i)", oid, data_pp, size);

  used_objmeta_p = kogmo_rtdb_obj_findmeta_byid (db_h, oid);
  if ( used_objmeta_p == NULL ) return -KOGMO_RTDB_ERR_NOTFOUND;
  if ( used_objmeta_p -> buffer_idx == 0 ) return -KOGMO_RTDB_ERR_NOTFOUND;

  if ( size > used_objmeta_p -> size_max ) return -KOGMO_RTDB_ERR_INVALID;
  if ( size < sizeof ( kogmo_rtdb_subobj_base_t ) ) return -KOGMO_RTDB_ERR_INVALID;

  if ( used_objmeta_p -> flags.write_allow
    || used_objmeta_p -> created_proc != db_h->ipc_h.this_process.proc_oid )
    {
      DBGL (DBGL_MSG,"no ptr-write for public or others objects");
      return -KOGMO_RTDB_ERR_INVALID;
    }

  if ( used_objmeta_p -> flags.cycle_watch )
    {
      kogmo_rtdb_subobj_base_t *scan_objbase;
      float min_cycle_time = MIN_CYCLETIME(used_objmeta_p -> min_cycletime, used_objmeta_p -> max_cycletime);
      // the latest data
      scan_objbase = kogmo_rtdb_obj_histscan (db_h, 0, used_objmeta_p, NULL, NULL);
      if ( scan_objbase != NULL ) // there have been previous commits..
        {
          float time_since_last_commit = kogmo_timestamp_diff_secs (scan_objbase->committed_ts, committed_ts);
          DBGL (DBGL_MSG,"cycle-watch: time_since_last_commit %f < %f ?", time_since_last_commit, min_cycle_time);
          if ( time_since_last_commit < min_cycle_time )
            {
              DBGL (DBGL_MSG,"updates too fast for oid %d: %f < %f", used_objmeta_p -> oid, time_since_last_commit, min_cycle_time);
              return -KOGMO_RTDB_ERR_TOOFAST;
            }
        }
    }

  no_notifies = used_objmeta_p -> flags.no_notifies | db_h->localdata_p->flags.no_notifies;

  history_slot = used_objmeta_p -> history_slot;
  if ( history_slot < 0 )
    {
      history_slot = 0;
    }
  else
    {
      history_slot = ( history_slot + 1 ) % used_objmeta_p -> history_size;
    }

  heap_data_p = &db_h->localdata_p -> heap
                  [ used_objmeta_p -> buffer_idx
                  + history_slot * used_objmeta_p -> size_max ];

  if ( *(kogmo_rtdb_subobj_base_t**) data_pp != heap_data_p )
    {
    DBGL (DBGL_MSG,"obj data ptr no longer valid!");
    return -KOGMO_RTDB_ERR_INVALID;
  }

  // Update Base Data
  ((kogmo_rtdb_subobj_base_t *) heap_data_p) -> committed_proc = db_h->ipc_h.this_process.proc_oid;

  // 5b. if there is no data_ts, set it to committed_ts
  if ( ((kogmo_rtdb_subobj_base_t *) heap_data_p) -> data_ts == invalid_ts )
    ((kogmo_rtdb_subobj_base_t *) heap_data_p) -> data_ts = committed_ts;

  // pre-6. block new notify-listeners unless notifies are disabled
  if ( ! no_notifies )
    kogmo_rtdb_obj_do_notify_prepare(db_h, used_objmeta_p);

  // 6. make slot valid with new committed_ts
  COPY_INT64_LOWFIRST( ((kogmo_rtdb_subobj_base_t *) heap_data_p) -> committed_ts, committed_ts);

  // 7. set pointer to this slot
  used_objmeta_p -> history_slot = history_slot;

  // 8. send notifies unless notifies are disabled
  if ( ! no_notifies )
    kogmo_rtdb_obj_do_notify (db_h, used_objmeta_p);

  DBGL (DBGL_API,"obj_ptr-committed oid %i into slot %i with %i bytes at ts=%lli",
        oid, history_slot, size, (long long int)committed_ts);

  IFDBGL (DBGL_DB+DBGL_VERBOSE)
    {
      char *p;
      p = kogmo_rtdb_obj_dumpbase_str (db_h, data_pp);
      printf("{\n%s}\n",p);
      free(p);
      IFDBGL (DBGL_VERYVERBOSE)
      if(0)  {
          p = kogmo_rtdb_obj_dumphex_str (db_h, data_pp);
          printf("{\n%s}\n",p);
          free(p);
        }
    }

  kogmo_rtdb_obj_trace_send (db_h, oid, committed_ts, KOGMO_RTDB_TRACE_UPDATED,
        kogmo_rtdb_obj_slotnum (db_h, used_objmeta_p), history_slot);

  return 0;
}



/* ******************** OBJECT SELECT HELPERS ******************** */

/*! \brief Find an Object-History-Slot by its Slot-Number with adjustment.
 * For internal use only.
 * adj: 0 = get current slot, must be called first for initialization!
 *     -1 = get previous slot,
 *     +1 = get next slot
 * Note:
 *  - currslot & firstslot are internal and will be changed
 * \returns Pointer to its Data or NULL if not found or history-overrun
 */
inline static void *
kogmo_rtdb_obj_histscan (kogmo_rtdb_handle_t *db_h, int32_t adj,
                         kogmo_rtdb_obj_info_t *scan_objmeta_p,
                         int32_t *currslot_p2, int32_t *firstslot_p)
{
  kogmo_rtdb_subobj_base_t *objbase;
  int32_t currslot_tmp;
  int32_t *currslot_p; // = currslot_p2 ? currslot_p2 : &currslot_tmp;
  //xDBG("histscan: request: adj:%i, curr:%i last:%i",adj,xx*currslot_p,xx*firstslot_p);

  if ( scan_objmeta_p -> history_size == 0)
    return NULL;

  if ( currslot_p2 )
    {
      currslot_p = currslot_p2;
    }
  else
    {
      currslot_p = &currslot_tmp;
      *currslot_p = scan_objmeta_p -> history_slot;
    }

  if ( adj == 0 )
    {
      // get current slot = the latest data
      *currslot_p = scan_objmeta_p -> history_slot;
      if ( firstslot_p )
        *firstslot_p = *currslot_p;
    }
  else
    {
      *currslot_p += adj;
    }

  if ( currslot_p < 0 ) // no writes yet
    return NULL;

  // calculate next slot position
  *currslot_p = (*currslot_p + scan_objmeta_p -> history_size ) % scan_objmeta_p -> history_size;
  //xDBG("histscan: calculated next slot: %i",*currslot_p);

  // wrap-around check
  if ( firstslot_p && *currslot_p == *firstslot_p && adj != 0 )
    {
      //xDBG("histscan: error: wrap around!");
      return NULL;
    }

  // calculate object data pointer from history slot number
  objbase = (kogmo_rtdb_subobj_base_t *)
             & ( db_h->localdata_p -> heap [
                                             scan_objmeta_p -> buffer_idx +
                                             *currslot_p * scan_objmeta_p -> size_max
                                           ] );

  if ( CMP_INT64_HIGH(objbase -> committed_ts, invalid_ts) )
    {
      //xDBG("histscan: error: unused slot");
      return NULL;
    }

  //xDBG("histscan: result: curr:%i last:%i",*currslot_p,*firstslot_p);
  return objbase;
}


inline static kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata__mode (kogmo_rtdb_handle_t *db_h, int mode,
                             kogmo_rtdb_objid_t oid, kogmo_timestamp_t ts,
                             void *data_p, kogmo_rtdb_objsize_t size);

#define RTDBSEL_LAST 0
#define RTDBSEL_OLDER 1
#define RTDBSEL_YOUNGER 2
#define RTDBSEL_DATATIME 3
#define RTDBSEL_DATAOLDER 4
#define RTDBSEL_DATAYOUNGER 5
#define RTDBSEL_MASK_TIME 7
#define RTDBSEL_PTR 8

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                       kogmo_timestamp_t ts,
                       void *data_p, kogmo_rtdb_objsize_t size)
{
  CHK_DBH("kogmo_rtdb_obj_readdata",db_h,0);
  return kogmo_rtdb_obj_readdata__mode (db_h, RTDBSEL_LAST, oid, ts, data_p, size);
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_older (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                             kogmo_timestamp_t ts,
                             void *data_p, kogmo_rtdb_objsize_t size)
{
  CHK_DBH("kogmo_rtdb_obj_readdata_older",db_h,0);
  return kogmo_rtdb_obj_readdata__mode (db_h, RTDBSEL_OLDER, oid, ts, data_p, size);
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_younger (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                       kogmo_timestamp_t ts,
                       void *data_p, kogmo_rtdb_objsize_t size)
{
  CHK_DBH("kogmo_rtdb_obj_readdata_younger",db_h,0);
  return kogmo_rtdb_obj_readdata__mode (db_h, RTDBSEL_YOUNGER, oid, ts, data_p, size);
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_datatime (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                       kogmo_timestamp_t ts,
                       void *data_p, kogmo_rtdb_objsize_t size)
{
  CHK_DBH("kogmo_rtdb_obj_readdata_datatime",db_h,0);
  return kogmo_rtdb_obj_readdata__mode (db_h, RTDBSEL_DATATIME, oid, ts, data_p, size);
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_dataolder (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                             kogmo_timestamp_t ts,
                             void *data_p, kogmo_rtdb_objsize_t size)
{
  CHK_DBH("kogmo_rtdb_obj_readdata_dataolder",db_h,0);
  return kogmo_rtdb_obj_readdata__mode (db_h, RTDBSEL_DATAOLDER, oid, ts, data_p, size);
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_datayounger (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                       kogmo_timestamp_t ts,
                       void *data_p, kogmo_rtdb_objsize_t size)
{
  CHK_DBH("kogmo_rtdb_obj_readdata_datayounger",db_h,0);
  return kogmo_rtdb_obj_readdata__mode (db_h, RTDBSEL_DATAYOUNGER, oid, ts, data_p, size);
}


// now everything with pointers
// (be aware of wrap-arounds! with pointers, you must care about wrap-arounds by yourself!see my dissertation/papers)..

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_ptr (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                             kogmo_timestamp_t ts, void *data_p)
{
  CHK_DBH("kogmo_rtdb_obj_readdata_ptr",db_h,0);
  return kogmo_rtdb_obj_readdata__mode (db_h, RTDBSEL_LAST | RTDBSEL_PTR, oid, ts, data_p, 0);
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_older_ptr (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                                   kogmo_timestamp_t ts, void *data_p)
{
  CHK_DBH("kogmo_rtdb_obj_readdata_older_ptr",db_h,0);
  return kogmo_rtdb_obj_readdata__mode (db_h, RTDBSEL_OLDER | RTDBSEL_PTR, oid, ts, data_p, 0);
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_younger_ptr (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                                     kogmo_timestamp_t ts, void *data_p)
{
  CHK_DBH("kogmo_rtdb_obj_readdata_younger_ptr",db_h,0);
  return kogmo_rtdb_obj_readdata__mode (db_h, RTDBSEL_YOUNGER | RTDBSEL_PTR, oid, ts, data_p, 0);
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_datatime_ptr (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                                      kogmo_timestamp_t ts, void *data_p)
{
  CHK_DBH("kogmo_rtdb_obj_readdata_datatime_ptr",db_h,0);
  return kogmo_rtdb_obj_readdata__mode (db_h, RTDBSEL_DATATIME | RTDBSEL_PTR, oid, ts, data_p, 0);
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_dataolder_ptr (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                                       kogmo_timestamp_t ts, void *data_p)
{
  CHK_DBH("kogmo_rtdb_obj_readdata_dataolder_ptr",db_h,0);
  return kogmo_rtdb_obj_readdata__mode (db_h, RTDBSEL_DATAOLDER | RTDBSEL_PTR, oid, ts, data_p, 0);
}

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_datayounger_ptr (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                                         kogmo_timestamp_t ts, void *data_p)
{
  CHK_DBH("kogmo_rtdb_obj_readdata_datayounger_ptr",db_h,0);
  return kogmo_rtdb_obj_readdata__mode (db_h, RTDBSEL_DATAYOUNGER | RTDBSEL_PTR, oid, ts, data_p, 0);
}



inline static kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata__mode (kogmo_rtdb_handle_t *db_h, int mode,
                       kogmo_rtdb_objid_t oid, kogmo_timestamp_t ts,
                       void *data_p, kogmo_rtdb_objsize_t size)
{
  kogmo_rtdb_obj_info_t *scan_objmeta_p;
  kogmo_rtdb_subobj_base_t *scan_objbase,*next_scan_objbase;
  volatile kogmo_timestamp_t scan_ts=0,next_scan_ts=0,scan_data_ts=0,next_scan_data_ts=0,final_ts=0;
  kogmo_rtdb_objsize_t avail_size;
  int32_t sl=0,fsl=0; // state for kogmo_rtdb_obj_histscan()

  CHK_DBH("kogmo_rtdb_obj_readdata_(...)",db_h,0);
  CHK_PTR(data_p);    

  IFDBGL (DBGL_API)
    {
      kogmo_timestamp_string_t tstr;
      if (ts)
        kogmo_timestamp_to_string(ts, tstr);
      DBGL (DBGL_API,"obj_readdata_%i(oid %i, %s, %p, size %i)",
            mode, oid, ts ? tstr : "0", data_p, size);
    }

  scan_objmeta_p = kogmo_rtdb_obj_findmeta_byid (db_h, oid);
  if ( scan_objmeta_p == NULL ) return -KOGMO_RTDB_ERR_NOTFOUND;
  if ( scan_objmeta_p -> buffer_idx == 0 ) return -KOGMO_RTDB_ERR_NOTFOUND;
  if ( scan_objmeta_p -> flags.read_deny
    && scan_objmeta_p -> created_proc != db_h->ipc_h.this_process.proc_oid
    && !this_process_is_admin (db_h) )
    {
      DBGL (DBGL_MSG,"read permission denied for oid %d", scan_objmeta_p ->oid);
      return -KOGMO_RTDB_ERR_NOPERM;
    }

  // the latest data
  scan_objbase = kogmo_rtdb_obj_histscan (db_h, 0, scan_objmeta_p, &sl, &fsl);
  //xDBG("(1)selected slot %i, max %i",sl,fsl);
  if ( scan_objbase == NULL ) return -KOGMO_RTDB_ERR_NOTFOUND;

  switch (mode & RTDBSEL_MASK_TIME) {
    case RTDBSEL_LAST:
      if ( ts == invalid_ts && scan_objmeta_p -> deleted_ts != invalid_ts )
        {
          DBG("object now deleted");
          return -KOGMO_RTDB_ERR_NOTFOUND;
        }
      COPY_INT64_HIGHFIRST( scan_ts, scan_objbase -> committed_ts );
      DBG("scan: find latest for ts=%lli (first: %lli)",(long long int)ts,(long long int)scan_ts);
      if ( ts == invalid_ts ) break; // search for ts==0 means latest data
      while ( scan_ts > ts  )
        {
          DBG("scan: still %lli > %lli ", (long long int)scan_ts, (long long int)ts);
          scan_objbase = kogmo_rtdb_obj_histscan (db_h, -1, scan_objmeta_p, &sl, &fsl);
          if ( scan_objbase != NULL )
            {
              COPY_INT64_HIGHFIRST( scan_ts, scan_objbase -> committed_ts );
            }
          else
            {
              DBG("scan: eod");
              return -KOGMO_RTDB_ERR_NOTFOUND;
            }
        }
      break;
    case RTDBSEL_OLDER:
      COPY_INT64_HIGHFIRST( scan_ts, scan_objbase -> committed_ts );
      DBG("scan: find older for ts=%lli (first: %lli)",(long long int)ts,(long long int)scan_ts);
      while ( scan_ts >= ts  )
        {
          DBG("scan: still %lli > %lli ", (long long int)scan_ts, (long long int)ts);
          scan_objbase = kogmo_rtdb_obj_histscan (db_h, -1, scan_objmeta_p, &sl, &fsl);
          if ( scan_objbase != NULL )
            {
              COPY_INT64_HIGHFIRST( scan_ts, scan_objbase -> committed_ts);
            }
          else
            {
              DBG("scan: eod");
              return -KOGMO_RTDB_ERR_NOTFOUND;
            }
        }
      break;
    case RTDBSEL_YOUNGER:
      COPY_INT64_HIGHFIRST( scan_ts, scan_objbase -> committed_ts );
      next_scan_ts = scan_ts;
      next_scan_objbase = scan_objbase;
      DBG("scan: find younger for ts=%lli (first: %lli)",(long long int)ts,(long long int)scan_ts);
      if ( scan_ts <= ts )
        {
          DBG("scan: eod");
          return -KOGMO_RTDB_ERR_NOTFOUND;
        }
      while ( next_scan_ts > ts  )
        {
          DBG("scan: still %lli > %lli ", (long long int)scan_ts, (long long int)ts);
          scan_objbase = next_scan_objbase;
          scan_ts = next_scan_ts;
          next_scan_objbase = kogmo_rtdb_obj_histscan (db_h, -1, scan_objmeta_p, &sl, &fsl);
          if ( next_scan_objbase != NULL )
            {
              COPY_INT64_HIGHFIRST( next_scan_ts, next_scan_objbase -> committed_ts);
            }
          else
            {
              break;
            }
        }
      break;
    case RTDBSEL_DATATIME:
      COPY_INT64_HIGHFIRST( scan_ts, scan_objbase -> committed_ts );
      COPY_INT64_HIGHFIRST( scan_data_ts, scan_objbase -> data_ts );
      DBG("scan: find datatime for ts=%lli (first: %lli)",(long long int)ts,(long long int)scan_ts);
      while ( scan_data_ts > ts  )
        {
          DBG("scan: still %lli > %lli ", (long long int)scan_ts, (long long int)ts);
          scan_objbase = kogmo_rtdb_obj_histscan (db_h, -1, scan_objmeta_p, &sl, &fsl);
          if ( scan_objbase != NULL )
            {
              COPY_INT64_HIGHFIRST( scan_ts, scan_objbase -> committed_ts);
              COPY_INT64_HIGHFIRST( scan_data_ts, scan_objbase -> data_ts);
            }
          else
            {
              DBG("scan: eod");
              return -KOGMO_RTDB_ERR_NOTFOUND;
            }
        }
      break;
    case RTDBSEL_DATAOLDER:
      COPY_INT64_HIGHFIRST( scan_ts, scan_objbase -> committed_ts );
      COPY_INT64_HIGHFIRST( scan_data_ts, scan_objbase -> data_ts );
      DBG("scan: find data older for ts=%lli (first: %lli)",(long long int)ts,(long long int)scan_ts);
      while ( scan_data_ts >= ts  )
        {
          DBG("scan: still %lli > %lli ", (long long int)scan_ts, (long long int)ts);
          scan_objbase = kogmo_rtdb_obj_histscan (db_h, -1, scan_objmeta_p, &sl, &fsl);
          if ( scan_objbase != NULL )
            {
              COPY_INT64_HIGHFIRST( scan_ts, scan_objbase -> committed_ts);
              COPY_INT64_HIGHFIRST( scan_data_ts, scan_objbase -> data_ts);          
            }
          else
            {
              DBG("scan eod");
              return -KOGMO_RTDB_ERR_NOTFOUND;
            }
        }
      break;
    case RTDBSEL_DATAYOUNGER:
      COPY_INT64_HIGHFIRST( scan_ts, scan_objbase -> committed_ts );
      next_scan_ts = scan_ts;
      COPY_INT64_HIGHFIRST( scan_data_ts, scan_objbase -> data_ts );
      next_scan_data_ts = scan_data_ts;
      next_scan_objbase = scan_objbase;
      DBG("scan: find data younger for ts=%lli (first: %lli)",(long long int)ts,(long long int)scan_ts);
      if ( scan_data_ts <= ts )
        {
          DBG("scan: eod");
          return -KOGMO_RTDB_ERR_NOTFOUND;
        }
      while ( next_scan_data_ts > ts  )
        {
          DBG("scan: still %lli > %lli ", (long long int)scan_ts, (long long int)ts);
          scan_objbase = next_scan_objbase;
          scan_ts = next_scan_ts;
          scan_data_ts = next_scan_data_ts;
          next_scan_objbase = kogmo_rtdb_obj_histscan (db_h, -1, scan_objmeta_p, &sl, &fsl);
          if ( next_scan_objbase != NULL )
            {
              COPY_INT64_HIGHFIRST( next_scan_ts, next_scan_objbase -> committed_ts);
              COPY_INT64_HIGHFIRST( next_scan_data_ts, next_scan_objbase -> data_ts);
            }
          else
            {
              break;
            }
        }
      break;
    default:
      DBG("unknown mode");
      return -KOGMO_RTDB_ERR_NOTFOUND;
  }
  DBG("scan: found at commit-ts:%lli (data-ts:%lli)", (long long int)scan_ts, (long long int)scan_data_ts);

  if ( mode & RTDBSEL_PTR )
    {
      *(kogmo_rtdb_subobj_base_t**)data_p = scan_objbase;
      DBG("New direct-pointer: %p",*(char**)data_p);
      return scan_objbase -> size; // return real size
    }

  // from here: manage lockless copying..

  // fail if calling context too small?
  // ==> if ( objbase -> size > size ) return -KOGMO_RTDB_ERR_NOMEMORY;
  // NO. Better truncate it and return real size:
  // ==>
  avail_size = scan_objbase -> size <= size ? scan_objbase -> size : size;
  memcpy ( data_p, scan_objbase, avail_size );
  // update used size. otherwise a later dump or re-commit etc. will segfault!!
  ( (kogmo_rtdb_subobj_base_t*) data_p ) -> size = avail_size;

  COPY_INT64_LOWFIRST( final_ts, scan_objbase -> committed_ts );
  ((kogmo_rtdb_subobj_base_t *) data_p) -> committed_ts = final_ts;

  if ( final_ts != scan_ts )
    {
      DBG("copy data: history wrap-around! now %lli, should have been %lli", (long long int)final_ts, (long long int)scan_ts);
      return -KOGMO_RTDB_ERR_HISTWRAP;
    }

  if ( scan_objmeta_p -> flags.withhold_stale )
    {
      float max_cycle_time = MAX_CYCLETIME(scan_objmeta_p -> min_cycletime, scan_objmeta_p -> max_cycletime);
      float time_since_last_commit = kogmo_timestamp_diff_secs (scan_objbase->committed_ts, kogmo_rtdb_timestamp_now (db_h));
      DBGL (DBGL_MSG,"withhold-stale-check: commit-age %f > %f ?", time_since_last_commit, max_cycle_time);
      if ( time_since_last_commit > max_cycle_time )
        {
          DBGL (DBGL_MSG,"commit-age too old at withhold-stale-check for oid %d: %f > %f", scan_objmeta_p ->oid, time_since_last_commit, max_cycle_time);
          return -KOGMO_RTDB_ERR_TOOFAST; // TODO: choose better error code, but ESTALE already defined for HISTWRAP :-(
        }
    }

  return scan_objbase -> size; // return real size
}




kogmo_rtdb_objsize_t
_kogmo_rtdb_obj_readdata_waitnext_until (kogmo_rtdb_handle_t *db_h,
                            kogmo_rtdb_objid_t oid, kogmo_timestamp_t old_ts,
                            void *data_p, kogmo_rtdb_objsize_t size, kogmo_timestamp_t wakeup_ts,
                            int do_ptr)
{
  kogmo_rtdb_obj_info_t *scan_objmeta_p;
  kogmo_rtdb_obj_base_t  base_obj;
  kogmo_rtdb_objsize_t ret;
  int no_notifies;

  CHK_DBH("kogmo_rtdb_obj_readdata_waitnext(until)(ptr)",db_h,0);
  CHK_PTR(data_p);    

  IFDBGL (DBGL_API)
    {
      kogmo_timestamp_string_t tstr;
      if (old_ts)
        kogmo_timestamp_to_string(old_ts, tstr);
      DBGL (DBGL_API,"kogmo_rtdb_obj_readdata_waitnext(oid:%i,until:%s,ptr:%i)",
            oid, old_ts ? tstr : "0", do_ptr);
    }

  scan_objmeta_p = kogmo_rtdb_obj_findmeta_byid (db_h, oid);
  if ( scan_objmeta_p == NULL ) return -KOGMO_RTDB_ERR_NOTFOUND;

  no_notifies = scan_objmeta_p -> flags.no_notifies | db_h->localdata_p->flags.no_notifies;

  do
  {

  if ( ! no_notifies )
    kogmo_rtdb_obj_wait_notify_prepare (db_h, scan_objmeta_p);

  // for searching, reading the header is enough:
  ret = kogmo_rtdb_obj_readdata (db_h, oid, 0, &base_obj, sizeof(base_obj));
  DBG("kogmo_rtdb_obj_readdata_waitnext: kogmo_rtdb_obj_readdata()=%lli", (long long int)ret);

  if ( ret >=0 && old_ts < base_obj.base.committed_ts)
    {
      if ( ! no_notifies )     
        kogmo_rtdb_obj_wait_notify_done (db_h, scan_objmeta_p);
      break; // end wait-loop
    }

  if ( ret < 0 && ret != -KOGMO_RTDB_ERR_NOTFOUND )
    {
      if ( ! no_notifies )     
        kogmo_rtdb_obj_wait_notify_done (db_h, scan_objmeta_p);
      DBG("kogmo_rtdb_obj_readdata_waitnext: unknown error %i",-ret);
      return ret;
    }

  if ( ret == -KOGMO_RTDB_ERR_NOTFOUND )
    {
      if ( scan_objmeta_p -> deleted_ts != invalid_ts )
        {
          if ( ! no_notifies )     
            kogmo_rtdb_obj_wait_notify_done (db_h, scan_objmeta_p);
          DBG("kogmo_rtdb_obj_readdata_waitnext: object deleted");
          return -KOGMO_RTDB_ERR_NOTFOUND;
        }
      DBG("kogmo_rtdb_obj_readdata_waitnext: object has no data yet");
    }

  if ( ! no_notifies )
    {
      ret = kogmo_rtdb_obj_wait_notify (db_h, scan_objmeta_p, wakeup_ts);
      if ( ret == -KOGMO_RTDB_ERR_TIMEOUT )
        {
          DBG("timeout");
          kogmo_rtdb_obj_wait_notify_done (db_h, scan_objmeta_p);
          return ret;
        }
    }
  else
    {
      kogmo_timestamp_t poll_ts;
      float poll_time;
      poll_time = KOGMO_RTDB_NONOTIFIES_POLLTIME_FACTOR * MIN_CYCLETIME(scan_objmeta_p -> min_cycletime, scan_objmeta_p -> max_cycletime);
      poll_time = poll_time < KOGMO_RTDB_NONOTIFIES_POLLTIME_MAX ? poll_time : KOGMO_RTDB_NONOTIFIES_POLLTIME_MAX;
      poll_ts = kogmo_timestamp_add_secs ( kogmo_timestamp_now(), poll_time);
      if ( wakeup_ts != 0 && poll_ts > wakeup_ts )
        {
          DBG("poll-timeout");
          kogmo_rtdb_obj_wait_notify_done (db_h, scan_objmeta_p);
          return ret;
        }
      kogmo_rtdb_sleep_until (db_h, poll_ts);
    }

  DBG("kogmo_rtdb_obj_readdata_waitnext: wait for next change");
  } while (1);

  // found -> now read full object with latest data
  if ( do_ptr )
    ret = kogmo_rtdb_obj_readdata_ptr (db_h, oid, 0, data_p);
  else
    ret = kogmo_rtdb_obj_readdata (db_h, oid, 0, data_p, size);

   DBG("kogmo_rtdb_obj_readdata_waitnext: success: found new data. kogmo_rtdb_obj_readdata%s()=%lli", do_ptr ? "_ptr" : "", (long long int)ret);
   return ret;
}


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_waitnext_until (kogmo_rtdb_handle_t *db_h,
                            kogmo_rtdb_objid_t oid, kogmo_timestamp_t old_ts,
                            void *data_p, kogmo_rtdb_objsize_t size, kogmo_timestamp_t wakeup_ts)
{
  return _kogmo_rtdb_obj_readdata_waitnext_until (db_h, oid, old_ts, data_p, size, wakeup_ts, 0);
}


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_waitnext (kogmo_rtdb_handle_t *db_h,
                            kogmo_rtdb_objid_t oid, kogmo_timestamp_t old_ts,
                            void *data_p, kogmo_rtdb_objsize_t size)
{
  return _kogmo_rtdb_obj_readdata_waitnext_until (db_h, oid, old_ts, data_p, size, 0, 0);
}


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_waitnext_until_ptr (kogmo_rtdb_handle_t *db_h,
                            kogmo_rtdb_objid_t oid, kogmo_timestamp_t old_ts,
                            void *data_pp, kogmo_rtdb_objsize_t size, kogmo_timestamp_t wakeup_ts)
{
  return _kogmo_rtdb_obj_readdata_waitnext_until (db_h, oid, old_ts, data_pp, size, wakeup_ts, 1);
}


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_waitnext_ptr (kogmo_rtdb_handle_t *db_h,
                            kogmo_rtdb_objid_t oid, kogmo_timestamp_t old_ts,
                            void *data_pp, kogmo_rtdb_objsize_t size)
{
  return _kogmo_rtdb_obj_readdata_waitnext_until (db_h, oid, old_ts, data_pp, size, 0, 1);
}
