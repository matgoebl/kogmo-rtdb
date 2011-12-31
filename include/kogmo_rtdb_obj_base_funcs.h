/*! \file kogmo_rtdb_obj_base_funcs.h
 * \brief Functions for Basis Object (C-Interface).
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_OBJ_BASE_FUNCS_H
#define KOGMO_RTDB_OBJ_BASE_FUNCS_H


#ifdef __cplusplus
 namespace KogniMobil {
 extern "C" {
#endif

/*! \addtogroup kogmo_rtdb_objects
 */
/*@{*/

/*! \brief Dump the Basisdata of an Object into a string
 *
 * \param db_h        database handle
 * \param data_p      Pointer to a Object-Data-Struct
 * \returns           Pointer to a string that will contain the dump in ASCII;
 *                    YOU have to FREE it after usage!\n
 *                    NULL on errors
 *
 * Example: \code
 *   char *p = kogmo_rtdb_obj_dumpbase_str (&objdata);
 *   printf ("%s", p); free(p); \endcode
 */
char*
kogmo_rtdb_obj_dumpbase_str (kogmo_rtdb_handle_t *db_h, void *data_p);


/*! \brief Dump the Data of an Object as Hex to a string
 *
 * \param db_h        database handle
 * \param data_p      Pointer to a Object-Data-Struct
 * \returns           Pointer to a string that will contain the dump in ASCII;
 *                    YOU have to FREE it after usage!\n
 *                    NULL on errors
 *
 * Example: \code
 *   char *p = kogmo_rtdb_obj_dumphex_str (&objdata);
 *   printf ("%s", p); free(p); \endcode
 */
char*
kogmo_rtdb_obj_dumphex_str (kogmo_rtdb_handle_t *db_h, void *data_p);



/*!
 * \brief Functions for C3 Objects (C-Interface).
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */


// Functions for Process-Object (internal):

kogmo_rtdb_objid_t
kogmo_rtdb_obj_c3_process_searchprocessobj (kogmo_rtdb_handle_t *db_h,
                                            kogmo_timestamp_t ts,
                                            kogmo_rtdb_objid_t proc_oid);

/*! \brief Type of a pre-allocated String to receive the Output of
 * kogmo_rtdb_obj_c3_process_getprocessinfo()
 */
typedef char kogmo_rtdb_obj_c3_process_info_t[KOGMO_RTDB_OBJMETA_NAME_MAXLEN+100];

/*! \brief This gives more information about a process into a
 * struct kogmo_rtdb_obj_c3_process_info_t
 *
 * \returns       <0 on errors, on errors the string is set to "?".
 */
int
kogmo_rtdb_obj_c3_process_getprocessinfo (kogmo_rtdb_handle_t *db_h,
                                          kogmo_rtdb_objid_t proc_oid,
                                          kogmo_timestamp_t ts,
                                          kogmo_rtdb_obj_c3_process_info_t str);


/*@}*/

#ifdef __cplusplus
 }; /* extern "C" */
 }; /* namespace KogniMobil */
#endif

#endif /* KOGMO_RTDB_OBJ_BASE_FUNCS_H */
