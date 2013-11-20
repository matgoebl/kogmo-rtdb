/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file kogmo_rtdb_helpers.c
 * \brief Helper Functions for the RTDB
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */
 
#include "kogmo_rtdb_internal.h"

 /* ******************** OBJECT MANAGEMENT HELPERS ******************** */

/*! \brief Find an Object by its Object-ID.
 * For internal use only.
 * \returns Pointer to its Metadata or NULL if not found.
 */
// No LOCKs in this function!

kogmo_rtdb_obj_info_t *
kogmo_rtdb_obj_findmeta_byid (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid )
{
  int i=-1;
  kogmo_rtdb_obj_info_t *scan_objmeta_p;

  // here: linear search.
  // we could think about indexing if reasonable and KOGMO_RTDB_OBJ_MAX is high
  do
    {
      if ( ++i >= KOGMO_RTDB_OBJ_MAX )
        {
          return NULL;
        }
      scan_objmeta_p = &db_h->localdata_p->objmeta[i];
    }
  while ( scan_objmeta_p->oid != oid );

  return scan_objmeta_p;
}

