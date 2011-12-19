/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */
/*! \file kogmo_rtdb_stream.h
 * \brief Structures for RTDB Streams
 *
 * Copyright (c) 2003-2006 Matthias Goebl <mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_STREAM_H
#define KOGMO_RTDB_STREAM_H


#include <inttypes.h>

struct kogmo_rtdb_stream_chunk_t // 4+4+8+4+4=24 bytes
{
  char               fcc[4]; // "RTDB"
  uint32_t           cb;
  kogmo_timestamp_t  ts;
  kogmo_rtdb_objid_t oid;
  uint32_t           type;
};

#define KOGMO_RTDB_STREAM_TYPE_ADDOBJ 1
#define KOGMO_RTDB_STREAM_TYPE_DELOBJ 2
#define KOGMO_RTDB_STREAM_TYPE_UPDOBJ 3
#define KOGMO_RTDB_STREAM_TYPE_UPDOBJ_NEXT 4
#define KOGMO_RTDB_STREAM_TYPE_RFROBJ 5
#define KOGMO_RTDB_STREAM_TYPE_CHGOBJ 6
#define KOGMO_RTDB_STREAM_TYPE_ERROR  7

#endif /* KOGMO_RTDB_STREAM_H */
