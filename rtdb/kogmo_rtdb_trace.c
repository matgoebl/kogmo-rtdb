/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file kogmo_rtdb_trace.c
 * \brief Trace Functions for the RTDB
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */
 
#include "kogmo_rtdb_internal.h"

 /* ******************** TRACE MANAGEMENT ******************** */

int
kogmo_rtdb_obj_trace_send (kogmo_rtdb_handle_t *db_h,
                           kogmo_rtdb_objid_t oid,
                           kogmo_timestamp_t ts,
                           enum kogmo_rtdb_trace_event event,
                           int32_t obj_slot, int32_t hist_slot)
{
  if ( db_h->localdata_p->rtdb_tracebufsize == 0 )
    return -KOGMO_RTDB_ERR_INVALID;
  if ( !db_h->localdata_p->rtdb_trace)
    return -KOGMO_RTDB_ERR_INVALID;
  struct kogmo_rtdb_trace_msg tracemsg;
  DBG("TRACE: @%lli OID %lli: %i", (long long int)ts, (long long int)oid, event);
  tracemsg.oid = oid;
  tracemsg.ts = ts;
  tracemsg.event = event;
  tracemsg.obj_slot = obj_slot;
  tracemsg.hist_slot = hist_slot;
  kogmo_rtdb_ipc_mq_send(db_h->ipc_h.tracefd, &tracemsg, sizeof(tracemsg));
  return 0;
}

int
kogmo_rtdb_obj_trace_activate (kogmo_rtdb_handle_t *db_h,
                           int active, int *tracebufsize)
{
  struct kogmo_rtdb_trace_msg tracemsg;
  int i;
  if ( db_h->localdata_p->rtdb_tracebufsize == 0 )
    return -KOGMO_RTDB_ERR_INVALID;
  DBG("kogmo_rtdb_obj_trace_activate := %i", active);
  if ( active )
    {
      kogmo_rtdb_ipc_mq_receive_init(&db_h->ipc_h.rxtracefd, db_h->ipc_h.shmid);
      db_h->localdata_p->rtdb_trace = 1;
      // try to clear queue
      for(i=0;i<KOGMO_RTDB_TRACE_BUFSIZE;i++) {
       if ( kogmo_rtdb_ipc_mq_curmsgs(db_h->ipc_h.rxtracefd) <= 0 )
        break;
       kogmo_rtdb_ipc_mq_receive(db_h->ipc_h.rxtracefd, &tracemsg, sizeof(tracemsg));
      }
    }
  else
    {
      db_h->localdata_p->rtdb_trace = 0;
      db_h->ipc_h.rxtracefd = 0;
    }
  if ( tracebufsize != NULL )
    *tracebufsize = db_h->localdata_p->rtdb_tracebufsize;
  return 0;
}

int
kogmo_rtdb_obj_trace_receive (kogmo_rtdb_handle_t *db_h,
                           kogmo_rtdb_objid_t *oid,
                           kogmo_timestamp_t *ts,
                           /*enum kogmo_rtdb_trace_event*/ int *event,
                           int32_t *obj_slot, int32_t *hist_slot)
{
  int ret,msgcount1,msgcount2;
  struct kogmo_rtdb_trace_msg tracemsg;
  if ( db_h->localdata_p->rtdb_tracebufsize == 0 )
    return -KOGMO_RTDB_ERR_INVALID;
  if ( !db_h->localdata_p->rtdb_trace || !db_h->ipc_h.rxtracefd )
    return -KOGMO_RTDB_ERR_INVALID;

  msgcount1 = kogmo_rtdb_ipc_mq_curmsgs(db_h->ipc_h.rxtracefd);
  ret = kogmo_rtdb_ipc_mq_receive(db_h->ipc_h.rxtracefd, &tracemsg, sizeof(tracemsg));
  msgcount2 = kogmo_rtdb_ipc_mq_curmsgs(db_h->ipc_h.rxtracefd);
  DBG("* TRACE: %d bytes, @%lli OID %lli: %i", ret, (long long int)tracemsg.ts, (long long int)tracemsg.oid, tracemsg.event);
  *oid = tracemsg.oid;
  *ts = tracemsg.ts;
  *event = tracemsg.event;
  if (obj_slot)
    *obj_slot = tracemsg.obj_slot;
  if (hist_slot)
    *hist_slot = tracemsg.hist_slot;
  DBG("GOT TRACE: @%lli OID %lli: %i", (long long int)*ts, (long long int)*oid, *event);
  //return (msgcount1>=KOGMO_RTDB_TRACE_BUFSIZE || msgcount2>=KOGMO_RTDB_TRACE_BUFSIZE) ? 1 : 0;
  if(msgcount2>msgcount1) msgcount1=msgcount2;
  return db_h->localdata_p->rtdb_tracebufsize - msgcount1;
}
