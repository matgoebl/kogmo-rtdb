/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2009 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */


#ifndef __RTSMALL__
// Time Interval between Index Entries (e.g. every 0.25 s, this is the seek granularity)
#define TIMEIDXINTERVAL_DEFAULT (0.1)
// Length of Index (will be extended when required)
#define TIMEIDXENTRIES_DEFAULT ((int)( 10 * 60 / TIMEIDXINTERVAL_DEFAULT ))
#else
#define TIMEIDXINTERVAL_DEFAULT (1.0)
#define TIMEIDXENTRIES_DEFAULT ((int)( 60 / TIMEIDXINTERVAL_DEFAULT ))
#endif

#define _FILE_OFFSET_BITS 64
// off_t
#include <sys/types.h>
#include <unistd.h>


typedef PACKED_struct {
  kogmo_timestamp_t ts;
  off_t off;
} timeidx_entry_t;

typedef PACKED_struct {
  char fcc[4];
  uint8_t idxtype;
  uint8_t unused;
  struct
    {
      uint16_t with_timestamps : 1;
      uint16_t changed : 1;
    } flags;
  kogmo_rtdb_objid_t   oid;
  kogmo_rtdb_objname_t name;
  kogmo_rtdb_objtype_t otype;

  float interval;
  kogmo_timestamp_t first_ts;
  kogmo_timestamp_t last_ts;

  uint32_t last_entry;
  uint32_t max_entries;
  timeidx_entry_t entry[0];
} timeidx_info_t;

#define timeidx_info_size (sizeof(timeidx_info_t))
#define timeidx_entry_size (sizeof(timeidx_entry_t))
#define IDXTYPE_GLOBALTIME 1

extern timeidx_info_t *timeidx_p;
extern int timeidx_debug;

void timeidx_init (char *filename);

void timeidx_write (char *filename);

kogmo_timestamp_t timeidx_next_needed (void);

kogmo_timestamp_t timeidx_get_first (void);

kogmo_timestamp_t timeidx_get_last (void);

int timeidx_add (kogmo_timestamp_t ts, off_t off);

off_t timeidx_lookup (kogmo_timestamp_t ts);
