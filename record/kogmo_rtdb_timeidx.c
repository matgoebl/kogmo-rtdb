/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2009 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */

// Time-Index Handling: Problem: Recordings with time-gaps!

// welcher index (global time, single object name+type)
// global time granularity fuer schnell springen, aber dennoch dann time checken -> rekursiv eingrenzen
// anzahl eintraege im index
// ASSERT(size==16 bytes)!

#define _FILE_OFFSET_BITS 64

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include "kogmo_rtdb_internal.h"

#include "kogmo_rtdb_timeidx.h"

#undef DIE
#define DIE(msg...) do { \
 fprintf(stderr,"%i DIED in %s line %i: ",getpid(),__FILE__,__LINE__); fprintf(stderr,msg); fprintf(stderr,"\n"); exit(1); } while (0)

#define DBGIDX(args...) if(timeidx_debug) do{printf("%% timeidx: "args);putchar(10);}while(0)

timeidx_info_t *timeidx_p = NULL;
int timeidx_debug = 0;

void timeidx_init (char *filename)
{
  if ( filename )
    {
      struct stat fstat;
      FILE *fp;
      int size, ret;
      DBGIDX("trying to read index from file '%s'",filename);
      if ( stat (filename, &fstat) != 0 )
        {
          timeidx_init(NULL);
          //xDIE("cannot read file information of index file '%s': %s", filename, strerror(errno));
          return;
        }
      fp = fopen (filename, "r");
      if ( fp == NULL )
        DIE("cannot open index file '%s': %s",filename, strerror(errno));
      size = fstat.st_size;
      timeidx_p = malloc (size);
      if ( timeidx_p == NULL )
        DIE("cannot allocate %i bytes of memory for index file '%s'", size, filename);
      ret = fread (timeidx_p, 1, size, fp);
      if ( ret != size )
        DIE("cannot read full index file '%s', get only %i of %i bytes",filename, ret, size);
      fclose (fp);
      if ( timeidx_p->idxtype != IDXTYPE_GLOBALTIME )
        DIE("wrong index type %i for index file '%s'", timeidx_p->idxtype, filename);
      if ( timeidx_p->flags.with_timestamps == 0 )
        DIE("wrong flags in index file '%s'", filename);
      timeidx_p->max_entries = (size - timeidx_info_size) / timeidx_entry_size; // must be correct
      if ( timeidx_p->last_entry > timeidx_p->max_entries - 1 )
        timeidx_p->last_entry = timeidx_p->max_entries - 1;
      timeidx_p->flags.changed = 0;
      DBGIDX("successfully read index file '%s' with %i entries", filename, timeidx_p->last_entry + 1);
    }
  else
    {
      int size;
      size = timeidx_info_size + timeidx_entry_size * TIMEIDXENTRIES_DEFAULT;
      timeidx_p = calloc (1, size);
      if ( timeidx_p == NULL )
        DIE("cannot allocate %i bytes of memory for initial index", size);
      memcpy(timeidx_p->fcc,"RDBX",4);
      timeidx_p->max_entries = TIMEIDXENTRIES_DEFAULT;
      timeidx_p->idxtype = IDXTYPE_GLOBALTIME;
      timeidx_p->flags.with_timestamps = 1;
      timeidx_p->flags.changed = 0;
      timeidx_p->interval = TIMEIDXINTERVAL_DEFAULT;
      timeidx_p->last_entry = -1;
      DBGIDX("initial index: %i entries.", timeidx_p->max_entries);
    }
  return;
}

void timeidx_write (char *filename)
{
  if ( filename && timeidx_p->flags.changed == 1 )
    {
      FILE *fp;
      int ret, size;
      size = timeidx_info_size + timeidx_entry_size * ( timeidx_p->last_entry + 1 );
      DBGIDX("writing index with %i entries (%i bytes) to file '%s'",timeidx_p->last_entry + 1,size,filename);
      timeidx_p->max_entries = timeidx_p->last_entry + 1;
      fp = fopen (filename, "w");
      if ( fp == NULL )
        DIE("cannot open index file '%s' for writing: %s",filename, strerror(errno));
      ret = fwrite (timeidx_p, 1, size, fp);
      if ( ret != size )
        DIE("cannot write full index to file '%s', only %i of %i bytes written",filename, ret, size);
      fclose (fp);
    }
}





kogmo_timestamp_t timeidx_next_needed (void)
{
  if ( timeidx_p == NULL )
    return 0; // (should not happen)
  return kogmo_timestamp_add_secs ( timeidx_p->last_ts, timeidx_p->interval);
}

kogmo_timestamp_t timeidx_get_first (void)
{
  if ( timeidx_p == NULL )
    return 0; // (should not happen)
  return timeidx_p->first_ts;
}

kogmo_timestamp_t timeidx_get_last (void)
{
  if ( timeidx_p == NULL )
    return 0; // (should not happen)
  return timeidx_p->last_ts;
}

int timeidx_add (kogmo_timestamp_t ts, off_t off)
{
  float diff_to_last;
  int idx;
  unsigned i;

  if ( ts <= 0 || off <= 0 || timeidx_p == NULL )
    return -1; // (should not happen)

  diff_to_last = kogmo_timestamp_diff_secs ( timeidx_p->last_ts, ts);

  if ( diff_to_last < timeidx_p->interval && timeidx_p->first_ts != 0 )
    return 0; // too early to add another entry (and not in initialisation)

  if ( timeidx_p->first_ts == 0 )
    {
      timeidx_p->first_ts = ts;
      timeidx_p->last_ts = ts;
      timeidx_p->last_entry = 0;
      timeidx_p->entry[0].ts = ts;
      timeidx_p->entry[0].off = off;
      return 0;
    }

  idx = (int) ( kogmo_timestamp_diff_secs ( timeidx_p->first_ts, ts ) / timeidx_p->interval );
  if ( idx < 0 )
    return -1; // (should not happen)

  //DBGIDX("setting index number %i at %lli to %lli",idx,ts,off);

  for ( i = timeidx_p->last_entry + 1; i <= (unsigned)idx; i++ )
    {
      if ( i >= timeidx_p->max_entries - 1 )
        {
          int size;
          timeidx_p->max_entries += TIMEIDXENTRIES_DEFAULT;
          size = timeidx_info_size + timeidx_entry_size * timeidx_p->max_entries;
          DBGIDX("extending index size to %i bytes",size);
          timeidx_p = realloc (timeidx_p, size);
          if ( timeidx_p == NULL )
            DIE("cannot reallocate %i bytes of memory for index", size);
        }
      timeidx_p->entry[i].ts = ts;
      timeidx_p->entry[i].off = off;
   }

  timeidx_p->last_ts = ts;
  timeidx_p->last_entry = idx;

  timeidx_p->flags.changed = 1;
  return 0;
}


off_t timeidx_lookup (kogmo_timestamp_t ts)
{
  int idx;
  DBGIDX("loopup: %lli", (long long int)ts);
  if ( timeidx_p == NULL || ts > timeidx_p->last_ts )
    return -1; // // not found

  idx = (int) ( kogmo_timestamp_diff_secs ( timeidx_p->first_ts, ts ) / timeidx_p->interval );
  if ( idx < 0 )
    return -1; // (should not happen)

  if ( idx > (int)timeidx_p->last_entry )
    idx = timeidx_p->last_entry;

  DBGIDX("loopup found: %i. =  %lli", idx, (long long int)timeidx_p->entry[idx].off);
  return timeidx_p->entry[idx].off;
}

#if 0
off_t timeidx_func (kogmo_timestamp_t ts, off_t pos /* 0:query -1:set_initial_reference */ )
{
  // TODO: also use realloc() here
  #define MAX_TIMEIDX (TIMEIDXMINUTES*60/TIMEIDXINTERVAL)
  #define DBGIDX(args...) //printf(args)
// int ein mmapped struct packen..
  static off_t timeidx_pos[(int)MAX_TIMEIDX];
  static kogmo_timestamp_t timeidx_ref = 0;
  static int timeidx_last=-1;
  int i, idx;

  if ( pos == -1 ) // set initial timestamp
    {
      DBGIDX("timeidx_func: set_initial %lli\n",ts);
      timeidx_ref = ts;
      timeidx_last = 0;
      return -1;
    }
  if ( !timeidx_ref )
    return -1; // cannot work without reference (should not happen)

  idx = (int) (kogmo_timestamp_diff_secs ( timeidx_ref, ts ) / TIMEIDXINTERVAL );
  if ( pos ) // Populate Time-to-Position-Index
    {
      DBGIDX("timeidx_func: set pos %i = %lli\n",idx,ts);
      if ( idx > timeidx_last && idx < MAX_TIMEIDX -1 )
        {
          for ( i = timeidx_last+1; i <= idx; i ++ )
            {
              timeidx_pos[i] = pos;
            }
          timeidx_last = idx;
        }
      return -1;
    }

  // default: query (pos==0)
  if ( idx >= 0 && idx <= timeidx_last )
    {
      DBGIDX("timeidx_func: query %lli (pos %i): %lli\n",ts,idx,timeidx_pos[idx]);
      return timeidx_pos[idx];
    }
  DBGIDX("timeidx_func: query %lli (pos %i): not found\n",ts,idx);
  return -1; // not found
}
#endif
