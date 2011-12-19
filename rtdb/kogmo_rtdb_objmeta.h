/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */

#ifndef KOGMO_RTDB_OBJMETA_H
#define KOGMO_RTDB_OBJMETA_H

int
kogmo_rtdb_obj_purgeslot (kogmo_rtdb_handle_t *db_h,
                          int slot);

int
kogmo_rtdb_obj_delete_imm (kogmo_rtdb_handle_t *db_h,
                           kogmo_rtdb_obj_info_t *metadata_p,
                           int immediately_delete);

kogmo_rtdb_objid_t
kogmo_rtdb_obj_searchinfo_deleted (kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,  
                           kogmo_timestamp_t ts,
                           kogmo_rtdb_objid_list_t idlist,
                           int nth);

kogmo_rtdb_objid_t
kogmo_rtdb_obj_changeinfo (kogmo_rtdb_handle_t *db_h,
                           kogmo_rtdb_objid_t oid,   
                           kogmo_rtdb_obj_info_t *metadata_p);


#endif /* KOGMO_RTDB_OBJMETA_H */
