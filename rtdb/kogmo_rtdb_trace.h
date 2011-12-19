/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */

#ifndef KOGMO_RTDB_TRACE_H
#define KOGMO_RTDB_TRACE_H


// For the RTDB-Recorder:
struct kogmo_rtdb_trace_msg {
 kogmo_rtdb_objid_t oid;
 kogmo_timestamp_t ts;
 uint32_t event;
 int32_t obj_slot;
 int32_t hist_slot;
};
enum kogmo_rtdb_trace_event {
 KOGMO_RTDB_TRACE_INSERTED = 1,
 KOGMO_RTDB_TRACE_DELETED,
 KOGMO_RTDB_TRACE_UPDATED,
 KOGMO_RTDB_TRACE_CHANGED,
};
int
kogmo_rtdb_obj_trace_send (kogmo_rtdb_handle_t *db_h,
                           kogmo_rtdb_objid_t oid,
                           kogmo_timestamp_t ts,
                           enum kogmo_rtdb_trace_event event,
                           int32_t obj_slot, int32_t hist_slot);
int
kogmo_rtdb_obj_trace_activate (kogmo_rtdb_handle_t *db_h,
                           int active, int *tracebufsize);
int
kogmo_rtdb_obj_trace_receive (kogmo_rtdb_handle_t *db_h,
                           kogmo_rtdb_objid_t *oid,
                           kogmo_timestamp_t *ts,
                           /*enum kogmo_rtdb_trace_event*/ int *event,
                           int32_t *obj_slot, int32_t *hist_slot);

#define KOGMO_RTDB_TRACE_BUFSIZE_DESIRED 10000
#define KOGMO_RTDB_TRACE_BUFSIZE 300
#define KOGMO_RTDB_TRACE_BUFSIZE_MIN 10
// (getenv("MQUEUE_MSG_MAX")?atoi(getenv("MQUEUE_MSG_MAX")):10)

#endif /* KOGMO_RTDB_TRACE_H */
