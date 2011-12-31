/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file kogmo_rtdb_objmeta.c
 * \brief Management Functions for Object Metadata in the RTDB
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */
 
#include "kogmo_rtdb_internal.h"

 /* ******************** OBJECT MANAGEMENT ******************** */


int
kogmo_rtdb_obj_initinfo (kogmo_rtdb_handle_t *db_h,
                         kogmo_rtdb_obj_info_t *metadata_p,
                         char* name, kogmo_rtdb_objtype_t otype,
                         kogmo_rtdb_objsize_t size_max)
{
  CHK_DBH("objmeta_init",db_h,0);
  DBGL (DBGL_API,"objmeta_init(%p, %s, %i)", metadata_p, name, otype);
  CHK_PTR(metadata_p);
  memset (metadata_p, 0, sizeof(kogmo_rtdb_obj_info_t));
  strncpy(metadata_p->name,name,sizeof(metadata_p->name));
  metadata_p->otype=otype;
  metadata_p->min_cycletime = metadata_p->max_cycletime =
    db_h->procobjmeta.min_cycletime;
  metadata_p->history_interval = db_h->localdata_p->default_history_interval;
  metadata_p->size_max = size_max;
  return 0;
}


kogmo_rtdb_objid_t
_kogmo_rtdb_obj_searchinfo(kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_timestamp_t ts,
                           kogmo_rtdb_objid_list_t idlist,
                           int nth, int nolock, int includedeleted)
{
  int i=-1;
  kogmo_rtdb_obj_info_t *scan_objmeta_p;
  int regex = 0;
  regex_t re;
  int nfound = 0;
  kogmo_rtdb_objid_t oid = 0;
  char searchname[KOGMO_RTDB_OBJMETA_NAME_MAXLEN];

  DBGL (DBGL_API,"kogmo_rtdb_obj_searchinfo(%s, 0x%llX, %lli, %lli, %lli,, %i)",
                 name, (long long)otype, (long long)parent_oid,
                 (long long)proc_oid, (long long)ts, nth);

  if ( name != NULL && name[0] == '~' )
    {
      regmatch_t re_match[1];
      oid = 0;

      // a direct OID can be given as: ~(42) ~BlaBla(42) ~(0x2A) ~(052)
      if ( regcomp (&re, "\\(([0-9][0-9]*|0[xX]([0-9a-fA-F][0-9a-fA-F]*))\\) *$", REG_EXTENDED) != 0)
        return -KOGMO_RTDB_ERR_INVALID;
      if ( regexec(&re, name, (size_t) 1, re_match, 0) == 0 && re_match[0].rm_so >= 0 )
        oid = strtoll(&name[re_match[0].rm_so+1],NULL,0);
      regfree(&re);  
      if ( oid )
        {
          if ( kogmo_rtdb_obj_findmeta_byid (db_h, oid ) == NULL )
            return -KOGMO_RTDB_ERR_NOTFOUND;
          nfound = 1;
          if ( idlist == NULL && nth == nfound )
            return oid;
          if ( idlist == NULL && nth != nfound )
            return -KOGMO_RTDB_ERR_NOTFOUND;
          if ( KOGMO_RTDB_OBJIDLIST_MAX >= 1 ) // we do not assume its >= 2, will be optimized away by the compiler
            idlist[0] = oid;
          if ( KOGMO_RTDB_OBJIDLIST_MAX >= 2 ) // dito
            idlist[1] = 0;
          return nfound;
        }

      // a Type-ID can be given as: ~BlaBla#42 ~BlaBla#0xC30003
      if ( regcomp (&re, "\\#([0-9][0-9]*|0[xX]([0-9a-fA-F][0-9a-fA-F]*)) *$", REG_EXTENDED) != 0)
        return -KOGMO_RTDB_ERR_INVALID;
      if ( regexec(&re, name, (size_t) 1, re_match, 0) == 0 && re_match[0].rm_so >= 0 )
        {
          otype = strtoll(&name[re_match[0].rm_so+1],NULL,0);
          strncpy(searchname,name,sizeof(searchname));
          searchname[re_match[0].rm_so]='\0';
          name = searchname;
          printf("type: %i %x s.%s.\n", otype,otype,name);
        }
      regfree(&re);  

      if ( regcomp (&re, &name[1], REG_EXTENDED|REG_NOSUB|REG_ICASE) != 0)
        return -KOGMO_RTDB_ERR_INVALID;
      regex = 1;
    }

  if ( kogmo_rtdb_debug && !regex && name && getenv ("KOGMO_RTDB_NAMEREMAP") ) // syntax: KOGMO_RTDB_NAMEREMAP=origname=newname,origname2=newname2,...
    {
      char *q, *p = getenv ("KOGMO_RTDB_NAMEREMAP");
      int namelen = strlen ( name );
      int len;
      while ( p != NULL && p[0] != '\0' ) // until terminating NUL
        {
          q = strchr( p, '=' );
          if ( !q )
            break;
          q++; // now after '='
          if ( namelen == q-p-1 && strncmp ( name, p, namelen ) == 0 ) // names have the same len and are equal
            {
              p = strchr( q, ',' );
              if ( p )
                len = p-q;
              else
                len = strlen ( q );
              if ( len > sizeof(searchname) )
                len = sizeof(searchname); // error! remapped name to long!
              strncpy(searchname, q, len); 
              searchname[len] = '\0'; // ensure NUL termination
              name = searchname;
              break;
            }
          p = strchr( q, ',' );
          if ( p ) p++;
        }
    }

  if (!nolock)
    kogmo_rtdb_objmeta_lock(db_h);

  while ( ++i < KOGMO_RTDB_OBJ_MAX )
    {
      scan_objmeta_p = &db_h->localdata_p -> objmeta[i];

      // skip empty slots
      if ( scan_objmeta_p -> oid == 0 || scan_objmeta_p -> created_ts == 0 )
          continue;

      // skip objects with wrong type if type-parameter is set
      if ( scan_objmeta_p -> otype != otype && otype != 0 )
          continue;

      // skip objects with wrong parent if parent-parameter is set
      if ( scan_objmeta_p -> parent_oid != parent_oid && parent_oid != 0 )
          continue;

      // skip objects with wrong creator if creator-parameter is set
      if ( scan_objmeta_p -> created_proc != proc_oid && proc_oid != 0 )
          continue;

      // skip deleted objects if time-parameter is not set
      // skip deleted objects if time-parameter is set and
      // the object didn't exist at the given time
      // (warning: don't use "deleted_ts <= ts"! otherwise a trace reader won't find
      // a deleted object!)
      if ( scan_objmeta_p -> deleted_ts != 0 && !includedeleted )
        if ( scan_objmeta_p -> deleted_ts < ts || ts == 0 )
          continue;

      // skip not yet created objects if time-parameter is set
      if ( scan_objmeta_p -> created_ts > ts && ts != 0 )
          continue;

      // skip if regex doesn't match and regex-parameter is set
      if ( regex &&
           ( regexec(&re, scan_objmeta_p->name, (size_t) 0, NULL, 0) != 0 ) )
          continue;

      // skip if name doesn't match and no regex and name is not empty
      if ( !regex && name != NULL && name[0] != '\0' &&
           ( strncmp (name, scan_objmeta_p->name, KOGMO_RTDB_OBJMETA_NAME_MAXLEN) != 0 ) )
          continue;

      // found!!! this is a match
      oid = scan_objmeta_p -> oid;
      nfound++;

      // insert match into result-list if given and there's space
      if ( idlist != NULL && nfound < KOGMO_RTDB_OBJIDLIST_MAX )
        idlist[nfound-1] = oid;

      // if nth > 0 then stop after the nth. match
      if ( nth == nfound )
        break;
    }
  if ( regex )
    regfree(&re);
  if (!nolock)
    kogmo_rtdb_objmeta_unlock(db_h);
  if ( nfound == 0 )
    {
      kogmo_rtdb_obj_info_t *scan_objmeta_p;
      if ( idlist != NULL )
        idlist[0] = 0;

      // return invalid if parent-id does not exist
      if ( parent_oid )
        {
          scan_objmeta_p = kogmo_rtdb_obj_findmeta_byid (db_h, parent_oid);
          if ( scan_objmeta_p == NULL ) return -KOGMO_RTDB_ERR_INVALID;
          if ( scan_objmeta_p -> deleted_ts != 0 )
            if ( scan_objmeta_p -> deleted_ts > ts || ts == 0 )
              return -KOGMO_RTDB_ERR_INVALID;
        }

      // otherwise return not-found
      return -KOGMO_RTDB_ERR_NOTFOUND;
    }
  if ( idlist == NULL && nth == nfound )
    return oid;
  if ( idlist != NULL )
    {
      idlist [ nfound > KOGMO_RTDB_OBJIDLIST_MAX ?
                KOGMO_RTDB_OBJIDLIST_MAX : nfound ] = 0;
      return nfound;
    }
  return -KOGMO_RTDB_ERR_NOTFOUND;
}

kogmo_rtdb_objid_t
kogmo_rtdb_obj_searchinfo (kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_timestamp_t ts,
                           kogmo_rtdb_objid_list_t idlist,
                           int nth)
{
  CHK_DBH("kogmo_rtdb_obj_searchinfo",db_h,0);
  return _kogmo_rtdb_obj_searchinfo(db_h, name, otype, parent_oid, proc_oid,
                           ts, idlist, nth, 0, 0);
}

kogmo_rtdb_objid_t
kogmo_rtdb_obj_searchinfo_nolock (kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_timestamp_t ts,
                           kogmo_rtdb_objid_list_t idlist,
                           int nth)
{
  CHK_DBH("kogmo_rtdb_obj_searchinfo_nolock",db_h,0);
  return _kogmo_rtdb_obj_searchinfo(db_h, name, otype, parent_oid, proc_oid,
                           ts, idlist, nth, 1, 0);
}

kogmo_rtdb_objid_t
kogmo_rtdb_obj_searchinfo_deleted (kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_timestamp_t ts,
                           kogmo_rtdb_objid_list_t idlist,
                           int nth)
{
  CHK_DBH("kogmo_rtdb_obj_searchinfo_deleted",db_h,0);
  return _kogmo_rtdb_obj_searchinfo(db_h, name, otype, parent_oid, proc_oid,
                           ts, idlist, nth, 0, 1);
}




kogmo_rtdb_objid_t
kogmo_rtdb_obj_insert (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_obj_info_t *metadata_p)
{
  int i;
  int found_slot = -1, purge_retry = 0;
  float cycle_time;
  kogmo_rtdb_obj_info_t *scan_objmeta_p, *tmp_objmeta_p;
  kogmo_rtdb_objid_t free_oid,scan_oid,tmp_oid;
  kogmo_timestamp_t ts,scan_delete_ts;
  kogmo_rtdb_objsize_t new_allocated_heap_idx=0;

  CHK_DBH("kogmo_rtdb_obj_insert",db_h,0);
  CHK_PTR(metadata_p);

  ts = kogmo_rtdb_timestamp_now (db_h);

//  DxBGL (DBGL_API,"obj_insert(%p = %s, db_h=%p,%p)", metadata_p, metadata_p->name,db_h,&db_h->localdata_p->objmeta[0]);
//  DxBG("obj_insert: handle is at %p and points to %p",&db_h,db_h);
  DBGL (DBGL_API,"obj_insert(%p = %s, %i)", metadata_p, metadata_p->name, metadata_p -> size_max);

  if ( metadata_p -> size_max != 0 && metadata_p -> size_max < sizeof(kogmo_rtdb_subobj_base_t) )
    return -KOGMO_RTDB_ERR_INVALID;

  if ( metadata_p == NULL )
    {
      DBGL (DBGL_DB,"null pointer for new object");
      return -KOGMO_RTDB_ERR_INVALID;
    }

  metadata_p -> name[KOGMO_RTDB_OBJMETA_NAME_MAXLEN-1] = '\0';
  if ( metadata_p -> name[0] == '\0' )
    {
      DBGL (DBGL_DB,"name for new object is empty");
      return -KOGMO_RTDB_ERR_INVALID;
    }
  // TODO: check for allowed characters in name!

  // Apply minimal history size restrictions
  if ( kogmo_rtdb_debug && getenv("KOGMO_RTDB_MINHIST") )
    {
      float minhist=0;
      long int minhistmaxosize=0;
      char *p;
      minhist = strtof(getenv("KOGMO_RTDB_MINHIST"),&p);
      if(*p == '/')
       minhistmaxosize = strtol(++p,NULL,0);
      DBGL (DBGL_DB,"Histsize adjust: %f < %f && %lli < %lli",
         metadata_p -> history_interval, minhist,
         (long long int) metadata_p -> size_max, (long long int) minhistmaxosize);
      if ( metadata_p -> history_interval < minhist
         && ( minhistmaxosize == 0 || metadata_p -> size_max <= minhistmaxosize ))
        metadata_p -> history_interval = minhist;
    }

  // Apply maximal history size restrictions
  if ( kogmo_rtdb_debug && getenv("KOGMO_RTDB_MAXHIST") )
    {
      float maxhist=0;
      long int maxhistmaxosize=0;
      char *p;
      maxhist = strtof(getenv("KOGMO_RTDB_MAXHIST"),&p);
      if(*p == '/')
       maxhistmaxosize = strtol(++p,NULL,0);
      DBGL (DBGL_DB,"Histsize adjust: %f > %f && %lli > %lli",
         metadata_p -> history_interval, maxhist,
         (long long int) metadata_p -> size_max, (long long int) maxhistmaxosize);
      if ( metadata_p -> history_interval > maxhist
         && ( maxhistmaxosize == 0 || metadata_p -> size_max >= maxhistmaxosize ))
        metadata_p -> history_interval = maxhist;
    }

  // Init Meta Data
  metadata_p->oid = 0;
  metadata_p -> created_ts = metadata_p -> lastmodified_ts = ts;
  metadata_p -> created_proc = metadata_p -> lastmodified_proc = db_h->ipc_h.this_process.proc_oid;
  metadata_p -> deleted_ts = metadata_p -> deleted_proc = 0;
  if ( metadata_p -> history_interval <= 0 )
    metadata_p -> history_interval = 0.001;
  if ( metadata_p -> max_cycletime <= 0 )
    metadata_p -> max_cycletime = 0.001;
  if ( metadata_p -> min_cycletime <= 0 )
    metadata_p -> min_cycletime = 0.001;
  #ifdef AUTO_CORRECT_CYCLETIMES
  if ( metadata_p -> min_cycletime > metadata_p -> max_cycletime )
    {
      float tmp;
      tmp = metadata_p -> max_cycletime;
      metadata_p -> max_cycletime = metadata_p -> min_cycletime;
      metadata_p -> min_cycletime = tmp;
    }
  #endif
  cycle_time =  MIN_CYCLETIME(metadata_p -> min_cycletime, metadata_p -> max_cycletime);
  metadata_p -> history_size = UNSIGNED_CEIL( metadata_p -> history_interval / cycle_time + 1 );
  if ( metadata_p -> size_max == 0 )
    metadata_p -> history_size = 0;
  metadata_p -> history_slot = -1;
  metadata_p -> buffer_idx = 0;
  if ( metadata_p -> size_max & 1 )
    metadata_p -> size_max++; // make size even


  kogmo_rtdb_objmeta_lock(db_h);

  // Try to find an object metadata slot
  // - First try: reuse an unused keep-alloc slot of this process
  //  => Then we need not allocate new memory
  for(i=0;i<KOGMO_RTDB_OBJ_MAX;i++)
    {
      scan_objmeta_p = &db_h->localdata_p -> objmeta[i];
      if ( !scan_objmeta_p->oid )
        continue; // used
      if ( !scan_objmeta_p->deleted_ts )
        continue; // not deleted
      if ( scan_objmeta_p->created_proc != db_h->ipc_h.this_process.proc_oid )
        continue; // not ours
      if ( !scan_objmeta_p->flags.keep_alloc )
        continue; // no keep-alloc
      if ( scan_objmeta_p->size_max * scan_objmeta_p -> history_size !=
           metadata_p -> size_max * metadata_p -> history_size)
        continue; // different size
      // matching object-type-ids are not necessary, relevant is only the total memory size

      scan_delete_ts = kogmo_timestamp_add_secs(
               scan_objmeta_p -> deleted_ts,
               scan_objmeta_p -> history_interval > db_h->localdata_p->default_keep_deleted_interval ?
               scan_objmeta_p -> history_interval : db_h->localdata_p->default_keep_deleted_interval);
      if ( scan_delete_ts < ts ) // reached deletion time or is 0
        {
          // the slot: has an id + is deleted + was owned by this proc + is flagged
          // + has the same total size + reached deletion time
          found_slot = i;
          scan_oid = scan_objmeta_p->oid;
          metadata_p -> buffer_idx = scan_objmeta_p->buffer_idx;
          DBGL (DBGL_DB,"reusing pre-allocated metadata slot %d with old "
                        "oid %lli and bufferindex %lli",
                    found_slot, (long long int) scan_oid, (long long int)
                    scan_objmeta_p->buffer_idx);
          break;
        }
    }


  // Found no preallocated slot
  if ( found_slot < 0 )
    {
      // this loop must be entered with lock held
      do
        { // non-real-time: when we are out of memory or slots run a purge and retry once
          #ifdef KOGMO_RTDB_HARDREALTIME
            purge_retry = 1; // do never purge and retry in hard real-time
          #else
            if ( purge_retry )
              {
               // Third try: reuse an old deleted slot, that the housekeeper hasn't purged yet (this respects priorities when the system load it too high)
                kogmo_rtdb_objmeta_unlock(db_h); // purge sets its own lock
                kogmo_rtdb_objmeta_purge_objs ( db_h );
                kogmo_rtdb_objmeta_lock(db_h);
              }
          #endif

          // need data blocks and found no preallocated object?
          if ( metadata_p -> size_max != 0 )
            {
              kogmo_rtdb_objmeta_unlock(db_h); // do not block other keep-alloc-inserts by memory allocation (using heap_lock)

              // allocate memory for object data, this may fail if there is insufficient space
              new_allocated_heap_idx = kogmo_rtdb_obj_mem_alloc (db_h,
                             metadata_p -> size_max * metadata_p -> history_size );
              DBG("alloc returned index %i",new_allocated_heap_idx);
              if ( new_allocated_heap_idx < 0 )
                {
                  if ( !purge_retry )
                    {
                      purge_retry = 1;
                      kogmo_rtdb_objmeta_lock(db_h);
                      continue; // retry
                    }
                  ERR("no more memory for object data in local heap");
                  return -KOGMO_RTDB_ERR_NOMEMORY;
                }
              metadata_p -> buffer_idx = new_allocated_heap_idx;

              kogmo_rtdb_objmeta_lock(db_h); // re-acquire lock. Warning: global metadata might have changed in the meantime!
            }

          // find free slot
          for(i=0;i<KOGMO_RTDB_OBJ_MAX;i++)
            {
              scan_objmeta_p = &db_h->localdata_p -> objmeta[i];
              if ( scan_objmeta_p->oid == 0 )
                {
                  found_slot = i;
                  scan_oid = scan_objmeta_p->oid;
                  db_h->localdata_p->objmeta_free--;
                  DBGL (DBGL_DB,"using empty object metadata slot %d", i);
                  break; // leave retry-loop
                }
             }

          // here: found no free slot!
          if ( !purge_retry )
            {
              purge_retry = 1;
              continue; // retry
            }
          else
            {
              kogmo_rtdb_objmeta_unlock(db_h);
              if ( new_allocated_heap_idx ) // allocated memory that is useless now
                kogmo_rtdb_obj_mem_free (db_h, new_allocated_heap_idx,
                                         metadata_p -> size_max * metadata_p -> history_size);
              DBGL (DBGL_DB,"found no free object metadata slot");
              return -KOGMO_RTDB_ERR_OUTOFOBJ;
            }
        } while(0); // retry-loop
    } // found no slot


  // Check whether there is already an unique object with this typeid and name,
  // or this unique object shall be unique, but there is already another object with this typeid and name
  for(i=0;i<KOGMO_RTDB_OBJ_MAX;i++)
    {
      tmp_oid = kogmo_rtdb_obj_searchinfo_nolock(db_h, NULL, metadata_p -> otype, 0, 0, 0, NULL, i+1);
      if ( tmp_oid <= 0 )
        break;
      tmp_objmeta_p = kogmo_rtdb_obj_findmeta_byid (db_h, tmp_oid);
      if ( tmp_objmeta_p == NULL )
        continue;
      if ( ( strncmp (metadata_p->name, tmp_objmeta_p->name, KOGMO_RTDB_OBJMETA_NAME_MAXLEN) == 0 )
           && ( tmp_objmeta_p -> flags.unique || metadata_p -> flags.unique) )
          {
            kogmo_rtdb_objmeta_unlock(db_h);
            if ( new_allocated_heap_idx ) // allocated memory that is useless now
              kogmo_rtdb_obj_mem_free (db_h, new_allocated_heap_idx,
                                       metadata_p -> size_max * metadata_p -> history_size);
            DBGL (DBGL_DB,"unique object already exists");
            return -KOGMO_RTDB_ERR_NOTUNIQ;
          }
    }


  DBGL (DBGL_DB,"using object metadata slot %d", found_slot);

  // copy metadata with oid still 0
  memcpy (scan_objmeta_p, metadata_p, sizeof(kogmo_rtdb_obj_info_t));

  // get new oid
  free_oid = db_h->localdata_p->objmeta_oid_next++;
  if ( free_oid <=0 )
    {
      if ( metadata_p -> buffer_idx != 0 )
        kogmo_rtdb_obj_mem_free (db_h, metadata_p -> buffer_idx,
                     metadata_p -> size_max * metadata_p -> history_size );
      kogmo_rtdb_objmeta_unlock(db_h);
      ERR("OUT OF OBJECT-IDs!!!");
      return -KOGMO_RTDB_ERR_OUTOFOBJ;
    }

  // if this is not the root object, make sure that parent_oid!=0
  if(!metadata_p->parent_oid && free_oid>1) scan_objmeta_p->parent_oid=metadata_p->parent_oid=1;

#ifdef KOGMO_RTDB_IPC_DO_POLLING
  kogmo_rtdb_obj_unlock (db_h, scan_objmeta_p);
#endif

  // set oid in db -> object activated
  scan_objmeta_p -> oid = free_oid;

  DBGL (DBGL_DB,"object metadata inserted with new oid %lli",
        (long long int)free_oid );

  kogmo_rtdb_objmeta_unlock_notify(db_h);

  IFDBGL (DBGL_DB+DBGL_VERBOSE)
    {
      char *p;
      p=kogmo_rtdb_obj_dumpinfo_str (db_h, scan_objmeta_p);
      printf("{\n%s}\n",p);
      free(p);
    }

  // set oid in local context
  metadata_p -> oid = free_oid;

  kogmo_rtdb_obj_trace_send (db_h, metadata_p -> oid, metadata_p -> created_ts, KOGMO_RTDB_TRACE_INSERTED,
        kogmo_rtdb_obj_slotnum (db_h, metadata_p), -1);

  return metadata_p->oid;
}


// internal!
// must be called after kogmo_rtdb_objmeta_lock(db_h)!
int
kogmo_rtdb_obj_purgeslot (kogmo_rtdb_handle_t *db_h,
                          int slot)
{
  kogmo_rtdb_obj_info_t *objmeta_p;
  objmeta_p = &db_h -> localdata_p -> objmeta[slot];
  DBGL (DBGL_DB,"purging object metadata slot %d with old oid %lli",
        slot, (long long int) objmeta_p->oid);
  if ( objmeta_p -> buffer_idx != 0 )
    kogmo_rtdb_obj_mem_free (db_h, objmeta_p -> buffer_idx,
                             objmeta_p -> size_max *
                             objmeta_p -> history_size );
  objmeta_p -> oid = 0;
  db_h->localdata_p->objmeta_free++;
  return 0;
}

int
kogmo_rtdb_obj_delete_imm (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_obj_info_t *metadata_p,
                       int immediately_delete)
{
  kogmo_rtdb_obj_info_t *used_objmeta_p;
  kogmo_rtdb_obj_info_t child_objmeta;
  kogmo_rtdb_objid_list_t child_objlist;
  int err;
  int i;

  CHK_DBH("kogmo_rtdb_obj_delete",db_h,0);
  CHK_PTR(metadata_p);
  DBGL (DBGL_API,"obj_delete(%p = %s(%d))", metadata_p,
        metadata_p->name, metadata_p->oid);

  if ( metadata_p == NULL )
    {
      DBGL (DBGL_DB,"null pointer for object");
      return -KOGMO_RTDB_ERR_INVALID;
    }

  kogmo_rtdb_objmeta_lock(db_h);

  used_objmeta_p = kogmo_rtdb_obj_findmeta_byid (db_h, metadata_p -> oid );

  if ( used_objmeta_p == NULL)
    {
      DBGL (DBGL_DB,"object metadata not found for oid %d", metadata_p -> oid);
      kogmo_rtdb_objmeta_unlock(db_h);
      return -KOGMO_RTDB_ERR_NOTFOUND;
    }

  if ( !used_objmeta_p -> flags.write_allow
       && used_objmeta_p -> created_proc != db_h->ipc_h.this_process.proc_oid
       && !this_process_is_admin (db_h) )
    {
      DBGL (DBGL_MSG,"delete permission denied for oid %d", used_objmeta_p -> oid);
      kogmo_rtdb_objmeta_unlock(db_h);
      return -KOGMO_RTDB_ERR_NOPERM;
    }

  // kogmo_rtdb_housekeeping.c:kogmo_rtdb_objmeta_purge_procs() may add immediately_delete
  if ( used_objmeta_p -> deleted_ts && !used_objmeta_p ->flags.immediately_delete && !immediately_delete )
    {
      DBGL (DBGL_MSG,"object with oid %d already deleted", used_objmeta_p -> oid);
      kogmo_rtdb_objmeta_unlock(db_h);
      return -KOGMO_RTDB_ERR_NOTFOUND;
    }

  // find objects depending on this object
  child_objlist[0]=0;
  err = kogmo_rtdb_obj_searchinfo_nolock (db_h, "", 0, metadata_p->oid, 0, 0, child_objlist, 0);
  if (err < 0 )
    child_objlist[0]=0;

  // mark as deleted
  used_objmeta_p -> deleted_proc = db_h->ipc_h.this_process.proc_oid;
  used_objmeta_p -> deleted_ts = kogmo_rtdb_timestamp_now (db_h);

  // clear data in return context
  // bad. better update it (add deleted_time etc..). unique oid will stay invalid forever -> no problem
  // memset (metadata_p, 0, sizeof(kogmo_rtdb_obj_info_t));
  // metadata_p -> oid = 0;

  // update local context
  memcpy (metadata_p, used_objmeta_p, sizeof(kogmo_rtdb_obj_info_t));

  // don't wait for purge if immediately_delete is set
  if ( used_objmeta_p ->flags.immediately_delete || immediately_delete )
    {
      kogmo_rtdb_obj_purgeslot(db_h, kogmo_rtdb_obj_slotnum(db_h, used_objmeta_p));
    }

  kogmo_rtdb_objmeta_unlock_notify(db_h);

  // inform listeners
  kogmo_rtdb_obj_do_notify_prepare(db_h, used_objmeta_p);
  kogmo_rtdb_obj_do_notify (db_h, used_objmeta_p);

  kogmo_rtdb_obj_trace_send (db_h, metadata_p -> oid, metadata_p -> deleted_ts, KOGMO_RTDB_TRACE_DELETED,
        kogmo_rtdb_obj_slotnum (db_h, metadata_p), -1);               

  // delete all objects that depend on this as parent
  for (i=0; child_objlist[i] != 0; i++ )
    {
      err = kogmo_rtdb_obj_readinfo (db_h, child_objlist[i], 0, &child_objmeta );
      if ( err < 0 )
        continue;   
      if ( child_objmeta.flags.parent_delete )
        {
          DBG("deleting depending object '%s'",child_objmeta.name);
          kogmo_rtdb_obj_delete(db_h, &child_objmeta);
          continue;
        }
    }
  return 0;
}

int
kogmo_rtdb_obj_delete (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_obj_info_t *metadata_p)
{
  return kogmo_rtdb_obj_delete_imm(db_h, metadata_p, 0);
}



int
kogmo_rtdb_obj_readinfo (kogmo_rtdb_handle_t *db_h, 
                         kogmo_rtdb_objid_t id, kogmo_timestamp_t ts,
                         kogmo_rtdb_obj_info_t *metadata_p)
{
  kogmo_rtdb_obj_info_t *used_objmeta_p;
  CHK_DBH("kogmo_rtdb_obj_readinfo",db_h,0);
  CHK_PTR(metadata_p);
  DBGL (DBGL_API,"obj_selectmeta_byid(%lli, %lli)", (long long int)id, (long long int)ts);

  kogmo_rtdb_objmeta_lock(db_h);

  used_objmeta_p = kogmo_rtdb_obj_findmeta_byid (db_h, id);

  if ( used_objmeta_p == NULL)
    {
      DBGL (DBGL_DB,"object metadata not found for oid %d", metadata_p -> oid);
      kogmo_rtdb_objmeta_unlock(db_h);
      return -KOGMO_RTDB_ERR_NOTFOUND;
    }

  if ( used_objmeta_p -> created_ts > ts && ts != 0 )
    {
      DBGL (DBGL_DB,"object metadata of oid %d does not yet exist", metadata_p -> oid);
      kogmo_rtdb_objmeta_unlock(db_h);
      return -KOGMO_RTDB_ERR_NOTFOUND;
    }
  if ( used_objmeta_p -> deleted_ts != 0 )
    {
      if ( used_objmeta_p -> deleted_ts < ts || ts == 0 )
        {
          DBGL (DBGL_DB,"object metadata of oid %d is deleted", metadata_p -> oid);
          kogmo_rtdb_objmeta_unlock(db_h);
          return -KOGMO_RTDB_ERR_NOTFOUND;
        }
    }

  // copy data in return context
  memcpy (metadata_p, used_objmeta_p, sizeof(kogmo_rtdb_obj_info_t));

  kogmo_rtdb_objmeta_unlock(db_h);
  return 0;
}







char* kogmo_rtdb_obj_dumpinfo_str (kogmo_rtdb_handle_t *db_h,
                                        kogmo_rtdb_obj_info_t *metadata_p)
{
  kogmo_timestamp_string_t tstr;
  kogmo_rtdb_obj_c3_process_info_t pi;

  NEWSTR(buf);
  STRPRINTF(buf,"* INFO:\n");
  STRPRINTF(buf,"Object: %s (%d)  ", metadata_p->name, metadata_p->oid);
  STRPRINTF(buf,"Type: 0x%04X  ", metadata_p->otype);
  STRPRINTF(buf,"Max.Size: %d  ", metadata_p->size_max);
  STRPRINTF(buf,"Parent: %d\n", metadata_p->parent_oid);

  kogmo_timestamp_to_string(metadata_p->created_ts, tstr);
  kogmo_rtdb_obj_c3_process_getprocessinfo (db_h, metadata_p->created_proc,
                                            0, pi);
  STRPRINTF(buf,"Created: %s by %s\n", tstr, pi);

/*
  kogmo_timestamp_to_string(metadata_p->lastmodified_ts, tstr);
  kogmo_rtdb_obj_c3_process_getprocessinfo (db_h,
      metadata_p->lastmodified_proc, metadata_p->lastmodified_ts, pi);
  STRPRINTF(buf,"Modified: %s by %s\n", tstr, pi);
*/

  if ( metadata_p->deleted_ts )
    {
      kogmo_timestamp_to_string(metadata_p->deleted_ts, tstr);
      kogmo_rtdb_obj_c3_process_getprocessinfo (db_h, metadata_p->deleted_proc,
                                                metadata_p->deleted_ts, pi);
      STRPRINTF(buf,"Deleted: %s by %s\n", tstr, pi);
    }
  else
    {
      STRPRINTF(buf,"Deleted: no\n");
    }


  STRPRINTF(buf,"History: %f secs with %f s cycle time min. (%f max.), slot %d/%d ",
   metadata_p->history_interval, metadata_p->min_cycletime, metadata_p->max_cycletime,
   metadata_p->history_slot+1, metadata_p->history_size );

  STRPRINTF(buf,"Buffer %lli\n", (long long int)metadata_p->buffer_idx );

  return buf;
}



kogmo_rtdb_objid_t
kogmo_rtdb_obj_searchinfo_wait_until(
                           kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_timestamp_t wakeup_ts)
{
  kogmo_rtdb_objid_t ret;

  CHK_DBH("kogmo_rtdb_obj_searchinfo_wait(until)",db_h,0);

  do
    {
      kogmo_rtdb_obj_wait_metanotify_prepare (db_h);
      ret = kogmo_rtdb_obj_searchinfo_nolock(db_h, name, otype, parent_oid,
                                             proc_oid, 0, NULL, 1);
      if ( ret < 0 && ret != -KOGMO_RTDB_ERR_NOTFOUND)
        {
          kogmo_rtdb_obj_wait_metanotify_done (db_h);
          DBG("kogmo_rtdb_obj_searchinfo_wait unknown error %i",-ret);
          return ret;
        }

      if ( ret >= 0 )
        {
          kogmo_rtdb_obj_wait_metanotify_done (db_h);
          DBG("kogmo_rtdb_obj_searchinfo_wait found it");
          return ret;
        }

      ret = kogmo_rtdb_obj_wait_metanotify (db_h, wakeup_ts);
      if ( ret == -KOGMO_RTDB_ERR_TIMEOUT )
        {
          kogmo_rtdb_obj_wait_metanotify_done (db_h);
          DBG("timeout");
          return ret;
        }
      DBG("kogmo_rtdb_obj_wait_meta next round");
    }
  while (1);
}

kogmo_rtdb_objid_t
kogmo_rtdb_obj_searchinfo_wait(
                           kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid)
{
  return kogmo_rtdb_obj_searchinfo_wait_until(db_h, name, otype, parent_oid, proc_oid, 0);
}




int
kogmo_rtdb_obj_searchinfo_waitnext_until(
                           kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,   
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_rtdb_objid_list_t known_idlist,    
                           kogmo_rtdb_objid_list_t added_idlist,   
                           kogmo_rtdb_objid_list_t deleted_idlist,
                           kogmo_timestamp_t wakeup_ts)
{
  kogmo_rtdb_objid_t num_added = 0;
  kogmo_rtdb_objid_t num_deleted = 0;
  kogmo_rtdb_objid_t ret;
  int i,j,found;
  kogmo_rtdb_objid_list_t newidlist;

  CHK_DBH("kogmo_rtdb_obj_searchinfo_waitnext",db_h,0);

  do
    {
      kogmo_rtdb_obj_wait_metanotify_prepare (db_h);
      ret = kogmo_rtdb_obj_searchinfo_nolock(db_h, name, otype, parent_oid,
                                             proc_oid, 0, newidlist, 0);
      if ( ret < 0 && ret != -KOGMO_RTDB_ERR_NOTFOUND)
        {
          kogmo_rtdb_obj_wait_metanotify_done (db_h);
          DBG("kogmo_rtdb_obj_searchinfo_wait unknown error %i",-ret);
          return ret;
        }

      i=-1;
      while ( newidlist[++i] )
        {
          found=0;
          j=-1;
          while ( known_idlist[++j] )
            {
              if ( known_idlist[j] == newidlist[i] )
                {
                  found = 1;
                  break;
                }
            }
          if ( !found )
            {
              added_idlist[num_added++] = newidlist[i];
            }
        }
      added_idlist[num_added]=0;

      i=-1;
      while ( known_idlist[++i] )
        {
          found=0;
          j=-1;
          while ( newidlist[++j] )
            {
              if ( newidlist[j] == known_idlist[i] )
                {
                  found = 1;
                  break;
                }
            }
          if ( !found )
            {
              deleted_idlist[num_deleted++] = known_idlist[i];
            }
        }
      deleted_idlist[num_deleted]=0;

      if ( num_added > 0 || num_deleted > 0 )
        {
          kogmo_rtdb_obj_wait_metanotify_done (db_h);
          i=-1;
          while ( newidlist[++i] )
            known_idlist[i] = newidlist[i];
          known_idlist[i]=0;
          DBG("kogmo_rtdb_obj_searchinfo_waitnext found changes");
          return num_added + num_deleted;
        }

      ret = kogmo_rtdb_obj_wait_metanotify (db_h, wakeup_ts);
      if ( ret == -KOGMO_RTDB_ERR_TIMEOUT )
        {
          kogmo_rtdb_obj_wait_metanotify_done (db_h);
          DBG("timeout");
          return ret;
        }
      DBG("kogmo_rtdb_obj_wait_meta next round");
    }
  while (1);
}


int
kogmo_rtdb_obj_searchinfo_waitnext(
                           kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_rtdb_objid_list_t known_idlist,
                           kogmo_rtdb_objid_list_t added_idlist,
                           kogmo_rtdb_objid_list_t deleted_idlist)
{
  return kogmo_rtdb_obj_searchinfo_waitnext_until(db_h, name, otype, parent_oid, proc_oid, known_idlist, added_idlist, deleted_idlist, 0);
}



int
kogmo_rtdb_obj_searchinfo_lists_nonblocking(kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,   
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_rtdb_objid_list_t known_idlist,    
                           kogmo_rtdb_objid_list_t added_idlist,   
                           kogmo_rtdb_objid_list_t deleted_idlist)
{
  kogmo_rtdb_objid_t num_added = 0;
  kogmo_rtdb_objid_t num_deleted = 0;
  kogmo_rtdb_objid_t ret;
  int i,j,found;
  kogmo_rtdb_objid_list_t newidlist;

  CHK_DBH("kogmo_rtdb_obj_searchinfo_waitnext",db_h,0);

//  do
//    {
      kogmo_rtdb_obj_wait_metanotify_prepare (db_h);
      ret = kogmo_rtdb_obj_searchinfo_nolock(db_h, name, otype, parent_oid,
                                             proc_oid, 0, newidlist, 0);
      if ( ret < 0 && ret != -KOGMO_RTDB_ERR_NOTFOUND)
        {
          kogmo_rtdb_obj_wait_metanotify_done (db_h);
          DBG("kogmo_rtdb_obj_searchinfo_wait unknown error %i",-ret);
          return ret;
        }

      i=-1;
      while ( newidlist[++i] )
        {
          found=0;
          j=-1;
          while ( known_idlist[++j] )
            {
              if ( known_idlist[j] == newidlist[i] )
                {
                  found = 1;
                  break;
                }
            }
          if ( !found )
            {
              added_idlist[num_added++] = newidlist[i];
            }
        }
      added_idlist[num_added]=0;

      i=-1;
      while ( known_idlist[++i] )
        {
          found=0;
          j=-1;
          while ( newidlist[++j] )
            {
              if ( newidlist[j] == known_idlist[i] )
                {
                  found = 1;
                  break;
                }
            }
          if ( !found )
            {
              deleted_idlist[num_deleted++] = known_idlist[i];
            }
        }
      deleted_idlist[num_deleted]=0;

//      if ( num_added > 0 || num_deleted > 0 )
//        {
          kogmo_rtdb_obj_wait_metanotify_done (db_h);
          i=-1;
          while ( newidlist[++i] )
            known_idlist[i] = newidlist[i];
          known_idlist[i]=0;
//          xDBG("kogmo_rtdb_obj_searchinfo_waitnext found changes");
          return num_added + num_deleted;
//        }

//      kogmo_rtdb_obj_wait_metanotify (db_h);
//      xDBG("kogmo_rtdb_obj_wait_meta next round");
//    }
//  while (1);
}




kogmo_rtdb_objid_t
kogmo_rtdb_obj_changeinfo (kogmo_rtdb_handle_t *db_h,
                           kogmo_rtdb_objid_t oid,
                           kogmo_rtdb_obj_info_t *metadata_p)
{
  kogmo_rtdb_obj_info_t *used_objmeta_p;
  kogmo_timestamp_t ts;

  CHK_DBH("kogmo_rtdb_obj_changeinfo",db_h,0);
  CHK_PTR(metadata_p);

  if ( ! this_process_is_admin (db_h) )  // maybe: db_h->localdata_p->flags.simmode
    return -KOGMO_RTDB_ERR_NOPERM;

  ts = kogmo_rtdb_timestamp_now (db_h);

  DBGL (DBGL_API,"obj_changeinfo(%i: %p = %s)", oid, metadata_p, metadata_p->name);

  kogmo_rtdb_objmeta_lock(db_h);

  used_objmeta_p = kogmo_rtdb_obj_findmeta_byid ( db_h, oid );

  if ( used_objmeta_p == NULL)
    {
      DBGL (DBGL_DB,"object metadata not found for oid %d", metadata_p -> oid);
      kogmo_rtdb_objmeta_unlock(db_h);
      return -KOGMO_RTDB_ERR_NOTFOUND;
    }

  used_objmeta_p -> lastmodified_proc = db_h->ipc_h.this_process.proc_oid;
  used_objmeta_p -> lastmodified_ts = ts;

  metadata_p -> name[KOGMO_RTDB_OBJMETA_NAME_MAXLEN-1] = '\0';
  if ( metadata_p -> name[0] != '\0' )
    strncpy(used_objmeta_p->name,metadata_p->name,sizeof(metadata_p->name));

  // here: allow change to 0?..

  if ( metadata_p -> otype )
    used_objmeta_p -> otype = metadata_p -> otype;

  if ( metadata_p -> parent_oid )
    used_objmeta_p -> parent_oid = metadata_p -> parent_oid;

  //if ( metadata_p -> flags ) // hmm.. does not allow setting to 0.. (TODO)
  //  used_objmeta_p -> flags = metadata_p -> flags;

  if ( metadata_p -> max_cycletime > 0 )
    used_objmeta_p -> max_cycletime = metadata_p -> max_cycletime;
  if ( metadata_p -> min_cycletime > 0 )
    used_objmeta_p -> min_cycletime = metadata_p -> min_cycletime;
  if ( metadata_p -> history_interval > 0 )
    used_objmeta_p -> history_interval = metadata_p -> history_interval;

  kogmo_rtdb_objmeta_unlock_notify(db_h);

  IFDBGL (DBGL_DB+DBGL_VERBOSE)
    {
      char *p;
      p=kogmo_rtdb_obj_dumpinfo_str (db_h, used_objmeta_p);
      printf("{\n%s}\n",p);
      free(p);
    }

  kogmo_rtdb_obj_trace_send (db_h, used_objmeta_p -> oid, used_objmeta_p -> lastmodified_ts, KOGMO_RTDB_TRACE_CHANGED,
        kogmo_rtdb_obj_slotnum (db_h, used_objmeta_p), -1);

  return used_objmeta_p->oid;
}
