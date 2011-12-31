/*! \file kogmo_rtdb_funcs.h
 * \brief Functions to Access the Real-time Vehicle Database (C-Interface).
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_FUNCS_H
#define KOGMO_RTDB_FUNCS_H

#include <errno.h>      /* error codes are based on them so far, see kogmo_rtdb_error */
#include <pthread.h>
#include "kogmo_rtdb_types.h"

#ifdef __cplusplus
 namespace KogniMobil {
 extern "C" {
 #define _const const   //!< constant argument for inclusion in C++
#else /* no C++ */
 #define _const         //!< constant argument, but irrelevant in C
#endif


/*! \defgroup kogmo_rtdb_conn C-Functions for Connection Handling
 */

/*! \defgroup kogmo_rtdb_meta C-Functions for Object Metadata Handling: Add, Remove, Search and Wait for Objects.
 *
 * \brief Using these functions you can create new objects within the database
 * and allocate space for actual data-block. You can also search for existing objects.
 *
 * See kogmo_rtdb_data on how to read and write object data.
 *
 * Those functions will be found mostly in the initialization section of applications.
 *
 * The object metadata is defined in kogmo_rtdb_obj_info_t.
 */
/*@{*/



/*! \brief Initialize Metadata for a new Object
 *
 * Zeros the metadata, fills it with defaults (e.g. the cycle time of the calling process)
 * and the given parameters.
 *
 * This function is for convenience and only helps to initialize the
 * metadata structure.
 * It does not yet create the object and does not modify the database.
 *
 * \param db_h        Database handle
 * \param metadata_p  Pointer to an Object-Metadata structure, that must be previously allocated by the user
 * \param name        Name for the new Object; non-empty,
 *                    allowed characters: a-z, A-Z, 0-9, "-", "_"; do not use "*","?" or "~"
 * \param otype       Type-Identifier for the new Object, see kogmo_rtdb_objtype for a list.
 * \param size_max    Maximum size for object data, use sizeof(your_object_t)
 * \returns           <0 on errors
 *
 * See also kogmo_rtdb_obj_info_t
 */
int
kogmo_rtdb_obj_initinfo (kogmo_rtdb_handle_t *db_h,
                         kogmo_rtdb_obj_info_t *metadata_p,
                         _const char* name, kogmo_rtdb_objtype_t otype,
                         kogmo_rtdb_objsize_t size_max);


/*! \brief Insert a new Object into the Database
 *
 * This inserts the metadata into the database and
 * allocates the space for the object data blocks within the database.
 *
 * \param db_h        Database handle
 * \param metadata_p  Pointer to previously set up Object-Metadata
 * \returns           <0 on errors, new Object-ID on success
 *                    \n(the new Object-ID will also be written into the given metadata_p).
 *  \retval -KOGMO_RTDB_ERR_INVALID    Invalid parameters within your metadata_p.
 *  \retval -KOGMO_RTDB_ERR_NOTUNIQ    There is already an (unique) object, that cannot coexist with your new (unique) object.
 *  \retval -KOGMO_RTDB_ERR_NOMEMORY   Not enough memory within the database for the new object and its history,
 *                                    restart the manager with a larger KOGMO_RTDB_HEAPSIZE.
 *  \retval -KOGMO_RTDB_ERR_OUTOFOBJ  Out of object slots (recompile the database with a higher KOGMO_RTDB_OBJ_MAX).
 *                   Be aware, that object slots are freed after their history_interval is expired.
 *                   This error also occurs when the OID reaches its maximum and a restart is required
 *                   (the OID is large enough that this should not happen within normal operation).
 */
kogmo_rtdb_objid_t
kogmo_rtdb_obj_insert (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_obj_info_t *metadata_p);


/*! \brief Delete an existing Object
 *
 * This removes an object, including its metadata and object data blocks.
 *
 * \note
 *   The object is only marked as deleted. Its history will still remain in the database for the time specified as
 *   history_interval. After this time the database manager process will actually delete it.
 *   - Only the metadata given at kogmo_rtdb_obj_insert() will be used!
 *     You cannot change the history_interval later.
 *   - If you want your objects to be removed immediately, create them with immediately_delete set.
 *   - If the deleted object has child objects with parent_delete set, they will also be deleted.
 * 
 * \param db_h        Database handle
 * \param metadata_p  Pointer to its Object-Metadata (only the Object-ID
 *                    will be used)
 * \returns           <0 on errors
 *  \retval -KOGMO_RTDB_ERR_INVALID    Invalid parameters (your metadata_p is NULL).
 *  \retval -KOGMO_RTDB_ERR_NOTFOUND   An object with the specified OID cannot be found or is already marked for deletion.
 *  \retval -KOGMO_RTDB_ERR_NOPERM     You are not allowed to delete this object.
 */
int
kogmo_rtdb_obj_delete (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_obj_info_t *metadata_p);


/*! \brief Read the Metadata of an Object given by its Object-ID.
 *
 * This function retrieves the metadata and copies it into the given metadata_p.
 *
 * \note
 *   Because you need only the Object-ID to read the object data,
 *   it is not necessary to retrieve the metadata first. 
 *
 * \param db_h        Database handle
 * \param oid         The Object-ID of the object you want to access
 * \param ts          Normally just 0 for "now"; with this timestamp you can specify
 *                    a point in time at which the Object was alive (and therefore not yet
 *                    deleted)
 * \param metadata_p  Pointer to a user-preallocated object metadata structure that will be filled with a copy
 *                    of the Metadata of the Object
 * \returns           <0 on errors
 *  \retval -KOGMO_RTDB_ERR_NOTFOUND   An object with the specified OID cannot be found or is already marked for deletion at the specified ts.
 */
int
kogmo_rtdb_obj_readinfo (kogmo_rtdb_handle_t *db_h,
                         kogmo_rtdb_objid_t oid,
                         kogmo_timestamp_t ts,
                         kogmo_rtdb_obj_info_t *metadata_p);


/*! \brief Dump the Metadata of a given Object into an ASCII-String
 *
 * This is useful if you want to show the Metadata to the user, e.g. for debugging.
 *
 * \param db_h        Database handle (needed to resolve e.g. process names)
 * \param metadata_p  Pointer to the object metadata structure with the data to be dumped.
 * \returns           Pointer to a string that will contain the dump in ASCII characters;
 *                    the user must FREE it by itself after usage!\n
 *                    NULL on errors
 *
 * Example: \code
 *   char *text_p = kogmo_rtdb_obj_dumpinfo_str (&objmeta);
 *   printf ("%s", text_p); free(text_p); \endcode
 */
char *
kogmo_rtdb_obj_dumpinfo_str (kogmo_rtdb_handle_t *db_h,
                             kogmo_rtdb_obj_info_t *metadata_p);


/*! \brief Find Objects by their Name, Parent, creating Process and Time.
 *
 * This function searches within the object metadata of all objects.
 * All specified fields must match (AND), a 0 or NULL means any value.
 *
 * \note
 *   Standard use case: There is at most one object, that matches your search criteria.
 *                    To find this object, set idlist=NULL and nth=1.
 *
 * \param db_h        Database handle
 * \param name        Look only for objects matching the given name; use NULL or "" for any name;
 *                    start with ~ to specify a regular expression, e.g. "~^foobar.*$"
 * \param otype       If !=0, find only objects with this type
 * \param parent_oid  If !=0, find only objects whose parent has this object-ID
 * \param proc_oid    If !=0, find only objects created by this process-ID,
 *                    see kogmo_rtdb_process
 * \param ts          Timestamp at which the Object must have been alive (and not
 *                    marked deleted); 0 means "now"
 * \param idlist      If not NULL, the resulting list of object-IDs will be written
 *                    into this user-preallocated array, terminated by an element with ID 0. The
 *                    maximum size is fixed, see kogmo_rtdb_objid_list.
 *                    When you want an idlist, you normally set nth=0.
 * \param nth         If !=0, stop the search after the n-th occurrence
 *                    (starting with 1).
 *                    So if you want the oid of the first hit, set idlist=NULL and nth=1.
 *                    Set nth=2 for the second hit, and so on.
 * \returns           <0 on errors,\n
 *                    if idlist=NULL: the object-ID of the nth result,\n
 *                    if an idlist!=NULL is given, the real number of matching objects (can be
 *                    higher than the length of idlist!)
 *  \retval -KOGMO_RTDB_ERR_NOTFOUND   There are no objects that match your search criteria.
 *  \retval -KOGMO_RTDB_ERR_INVALID    Invalid parameters (parent object does not exist (at that time), invalid regular expression).
 */
kogmo_rtdb_objid_t
kogmo_rtdb_obj_searchinfo (kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_timestamp_t ts,
                           kogmo_rtdb_objid_list_t idlist,
                           int nth);


/*! \brief Search and Wait until an Object specified by its Name, Parent, creating Process and Time exists.
 *
 * This function searches and waits if necessary until there is an object,
 * whose metadata matches your criteria.
 * If no object cannot be found, this functions blocks and waits until
 * another process creates an suitable object.
 * If there is such an object, the result is exactly the same as
 * kogmo_rtdb_obj_searchinfo(...,idlist=NULL,nth=1).
 *
 * This function searches only in the object metadata.
 *
 * \note
 *  You should use kogmo_rtdb_obj_searchinfo_wait() within the initialization stage
 *  of your software. So you don't need to arrange for a certain start order,
 *  as the needed objects are found as soon as they get created.
 *  \n If you want to check for new objects within your main loop,
 *  you might prefer kogmo_rtdb_obj_searchinfo() or kogmo_rtdb_obj_searchinfo_wait_until().
 *
 *
 * \param db_h        Database handle
 * \param name        Look only for objects matching the given name; use NULL or "" for any name;
 *                    start with ~ to specify a regular expression, e.g. "~^foobar.*$"
 * \param otype       If !=0, find only objects with this type
 * \param parent_oid  If !=0, find only objects whose parent has this object-ID
 * \param proc_oid    If !=0, find only objects created by this process-ID,
 *                    see kogmo_rtdb_process
 * \returns           <0 on errors, otherwise the object-ID of the first matching object is returned
 *  \retval -KOGMO_RTDB_ERR_INVALID    Invalid parameters (parent object does not exist (at that time)
 *                                     or has been deleted (to avoid waiting forever), invalid regular expression).
 */
kogmo_rtdb_objid_t
kogmo_rtdb_obj_searchinfo_wait(kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid);


/*! \brief Search and Wait until an Object specified by its Name, Parent, creating Process and Time exists, but return after a Timeout.
 *
 * This function searches and waits if necessary until there is an object,
 * whose metadata matches your criteria, exactly like kogmo_rtdb_obj_searchinfo_wait().
 * However it stops blocking and returns after a given timeout.
 *
 * \param db_h        Database handle
 * \param name        Look only for objects matching the given name; use NULL or "" for any name;
 *                    start with ~ to specify a regular expression, e.g. "~^foobar.*$"
 * \param otype       If !=0, find only objects with this type
 * \param parent_oid  If !=0, find only objects whose parent has this object-ID
 * \param proc_oid    If !=0, find only objects created by this process-ID,
 *                    see kogmo_rtdb_process
 * \param wakeup_ts   Absolute Time at which to wake up and return if there is still no object found,
 *                    0 means infinite (like kogmo_rtdb_obj_searchinfo_wait())
 * \returns           <0 on errors, otherwise the object-ID of the first matching object is returned
 *  \retval -KOGMO_RTDB_ERR_TIMEOUT    A timeout occurred.
 *  \retval -KOGMO_RTDB_ERR_INVALID    Invalid parameters (parent object does not exist (at that time)
 *                                     or has been deleted (to avoid waiting forever), invalid regular expression).
 */
kogmo_rtdb_objid_t
kogmo_rtdb_obj_searchinfo_wait_until(kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_timestamp_t wakeup_ts);


/*! \brief Watch for the Creation and Deletion of Objects matching certain criteria and Wait until there is a Change.
 *
 * When you call this function the first time, it returns a list of objects, that
 * match your search criteria (known_idlist). When you call it again, it compares
 * this list with the current situation and waits until
 * there is a change. Then the list (known_idlist) will be updated, and a list of the newly
 * added or deleted objects is also returned (added_idlist and deleted_idlist).
 *
 * This function searches only in the object metadata.
 *
 * \param db_h        Database handle
 * \param name        Look only for objects matching the given name; use NULL or "" for any name;
 *                    start with ~ to specify a regular expression, e.g. "~^foobar.*$"
 * \param otype       If !=0, find only objects with this type
 * \param parent_oid  If !=0, find only objects whose parent has this object-ID
 * \param proc_oid    If !=0, find only objects created by this process-ID,
 *                    see kogmo_rtdb_process
 * \param known_idlist The list of object-IDs already known and found in a previous call from kogmo_rtdb_obj_searchinfo().
 *                    This list will be automatically updated. You must initialize it with known_idlist[0]=0.
 * \param added_idlist    The list of newly created objects.
 * \param deleted_idlist  The list of newly deleted objects.
 * \returns           <0 on error, otherwise the sum of created plus deleted objects.
 *  \retval -KOGMO_RTDB_ERR_INVALID    Invalid parameters (parent object does not exist (at that time)
 *                                     or has been deleted (to avoid waiting forever), invalid regular expression).
 */
int
kogmo_rtdb_obj_searchinfo_waitnext(kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_rtdb_objid_list_t known_idlist,
                           kogmo_rtdb_objid_list_t added_idlist,
                           kogmo_rtdb_objid_list_t deleted_idlist);


/*! \brief Watch for the Creation and Deletion of Objects matching certain criteria and Wait until there is a Change, but return after a Timeout.
 *
 * This function works like kogmo_rtdb_obj_searchinfo_waitnext() with the distinction
 * that it stops blocking and returns after a given timeout.
 *
 * \param db_h        Database handle
 * \param name        Look only for objects matching the given name; use NULL or "" for any name;
 *                    start with ~ to specify a regular expression, e.g. "~^foobar.*$"
 * \param otype       If !=0, find only objects with this type
 * \param parent_oid  If !=0, find only objects whose parent has this object-ID
 * \param proc_oid    If !=0, find only objects created by this process-ID,
 *                    see kogmo_rtdb_process
 * \param known_idlist The list of object-IDs already known and found in a previous call from kogmo_rtdb_obj_searchinfo().
 *                    This list will be automatically updated. You must initialize it with known_idlist[0]=0
 * \param added_idlist    The list of newly created objects.
 * \param deleted_idlist  The list of newly deleted objects.
 * \param wakeup_ts   Absolute Time at which to wake up and return if there is still no change,
 *                    0 means infinite (like kogmo_rtdb_obj_searchinfo_waitnext())
 * \returns           <0 on error, otherwise the sum of created plus deleted objects.
 *  \retval -KOGMO_RTDB_ERR_TIMEOUT    A timeout occurred.
 *  \retval -KOGMO_RTDB_ERR_INVALID    Invalid parameters (parent object does not exist (at that time)
 *                                     or has been deleted (to avoid waiting forever), invalid regular expression).
 */
int
kogmo_rtdb_obj_searchinfo_waitnext_until(kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_rtdb_objid_list_t known_idlist,
                           kogmo_rtdb_objid_list_t added_idlist,
                           kogmo_rtdb_objid_list_t deleted_idlist,
                           kogmo_timestamp_t wakeup_ts);

/*@}*/







/*! \defgroup kogmo_rtdb_data C-Functions for Object Data Handling: Read and Write Object Data.
 *
 * \brief These functions access the actual contents of the data-block
 * of a certain object at a given point in time.
 *
 * Those functions will be found mostly in the main loop of applications.
 *
 * Every data-block of an object begins with kogmo_rtdb_subobj_base_t,
 * see kogmo_rtdb_objects for their definition.
 */
/*@{*/


/*! \brief Initialize a new data block for an existing object.
 *
 * Zeros the data structure, and fills it with defaults (e.g. the maximum size from the given kogmo_rtdb_obj_info_t).
 *
 * This function is for convenience and only helps to locally initialize the
 * passed object data structure (that always begins with kogmo_rtdb_subobj_base_t).
 * It does not yet write the object data into the database.
 *
 * \param db_h        Database handle
 * \param metadata_p  Pointer to the Object-Metadata structure as retrieved from the database
 * \param data_p      Pointer to an Object data structure, that has been previously allocated by the user
 * \returns           <0 on errors
 */
int
kogmo_rtdb_obj_initdata (kogmo_rtdb_handle_t *db_h,
                         kogmo_rtdb_obj_info_t *metadata_p,
                         void *data_p);



/*! \brief Write the local Data block of an Object to the Database.
 *
 * This function writes the Data-Block of an Object to the database;
 * this is used to update an object and is normally done at every cycle.
 *
 * \param db_h        Database handle
 * \param oid         Object-ID of the object to write
 * \param data_p      Pointer to an object data structure that will be copied into the database
 * \returns           <0 on errors
 *  \retval -KOGMO_RTDB_ERR_NOTFOUND   There is no object with the given object-id, the specified object has no space for data blocks (size=0).
 *  \retval -KOGMO_RTDB_ERR_INVALID    The data size not allowed (more than size_max in kogmo_rtdb_obj_info_t or less than kogmo_rtdb_subobj_base_t)
 *  \retval -KOGMO_RTDB_ERR_NOPERM     You are not allowed to write this object (see flags.write_allow)
 *  \retval -KOGMO_RTDB_ERR_TOOFAST    You are committing faster than the specified min_cycle_time and flags.cycle_watch is set)
 *
 * \note
 *  - The data_p should point to a structure, that starts with a sub-structure of
 *    kogmo_rtdb_subobj_base_t
 *  - Set your kogmo_rtdb_subobj_base_t.size to the correct size of your
 *    whole data-struct you want to commit
 *  - Set kogmo_rtdb_obj_subbase_t.data_ts accordingly.
 *    Do not commit data with timestamps old than in your last commit.\n
 *    (TODO: check for it, maybe refuse it)
 */
int
kogmo_rtdb_obj_writedata (kogmo_rtdb_handle_t *db_h,
                          kogmo_rtdb_objid_t oid,
                          void *data_p);


/*! \brief Read the latest Data of an Object from the Database
 *
 * This function reads the latest Data-Block of an Object from the database
 * and writes it into a local structure of the calling process.\n
 * It finds the block with a committed_ts <= ts.
 *
 * \param db_h        Database handle
 * \param oid         Object-ID of the desired object
 * \param ts          Timestamp at which the Object must be "the last committed";
 *                    0 for "now"
 * \param data_p      Pointer to a local object data structure where the data found in the rtdb will be copied to
 * \param size        Maximum size of the object data structure at data_p
 * \returns           <0 on errors, the real size of the object data found in the rtdb on success
 *  \retval -KOGMO_RTDB_ERR_NOTFOUND   There is no object with the given object-id for the specified timestamp, the specified object has no data block (not yet or size=0).
 *  \retval -KOGMO_RTDB_ERR_INVALID    The Pointer is NULL, The data size not allowed (more than size_max in kogmo_rtdb_obj_info_t or less than kogmo_rtdb_subobj_base_t)
 *  \retval -KOGMO_RTDB_ERR_NOPERM     You are not allowed to read this object (see flags.read_deny)
 *  \retval -KOGMO_RTDB_ERR_HISTWRAP   A history wrap-around occurred: While reading this object, the data has been overwritten.
 *                                     Either your reading process it too slow or the history_interval is to short.
 *  \retval -KOGMO_RTDB_ERR_TOOFAST    The data it stale (too old, now_ts-committed_ts > max_cycle_time) and
 *                                     this check is active (flags.withhold_stale is set).
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata (kogmo_rtdb_handle_t *db_h,
                         kogmo_rtdb_objid_t oid,
                         kogmo_timestamp_t ts,
                         void *data_p,
                         kogmo_rtdb_objsize_t size);



/*! \brief Read the Data of an Object committed before a given Timestamp
 *
 * This function finds the nearest data block of an object within the database with a committed_ts < ts.
 * \see kogmo_rtdb_obj_readdata()
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_older (kogmo_rtdb_handle_t *db_h, 
                               kogmo_rtdb_objid_t oid,
                               kogmo_timestamp_t ts,
                               void *data_p,
                               kogmo_rtdb_objsize_t size);


/*! \brief Read the Data of an Object committed after a given Timestamp
 *
 * This function finds the nearest data block of an object within the database with a committed_ts > ts.
 * \see kogmo_rtdb_obj_readdata()
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_younger (kogmo_rtdb_handle_t *db_h,
                                 kogmo_rtdb_objid_t oid,
                                 kogmo_timestamp_t ts,
                                 void *data_p,
                                 kogmo_rtdb_objsize_t size);


/*! \brief Read the Data of an Object from the Database that was valid for the given Data Timestamp.
 *
 * This function finds the nearest data block of an object within the database with a data_ts <= ts.
 * \see kogmo_rtdb_obj_readdata()
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_datatime (kogmo_rtdb_handle_t *db_h, 
                                  kogmo_rtdb_objid_t oid,
                                  kogmo_timestamp_t ts,
                                  void *data_p,
                                  kogmo_rtdb_objsize_t size);


/*! \brief Read the Data of Object that has an Data Timestamp older than a given Timestamp.
 *
 * This function finds the nearest data block of an object within the database with a data_ts < ts.
 * \see kogmo_rtdb_obj_readdata()
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_dataolder (kogmo_rtdb_handle_t *db_h,
                                   kogmo_rtdb_objid_t oid,
                                   kogmo_timestamp_t ts,
                                   void *data_p, 
                                   kogmo_rtdb_objsize_t size);


/*! \brief Read the Data of Object that has an Data Timestamp younger than a given Timestamp.
 *
 * This function finds the nearest data block of an object within the database with a data_ts > ts.
 * \see kogmo_rtdb_obj_readdata()
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_datayounger (kogmo_rtdb_handle_t *db_h,
                                     kogmo_rtdb_objid_t oid,
                                     kogmo_timestamp_t ts,
                                     void *data_p,
                                     kogmo_rtdb_objsize_t size);



/*! \brief Get the latest Data for an Object that has been committed after a given timestamp, and wait if there is no newer data
 *
 * This function waits until the Data-Block of an Object has an 
 * Commit-Timestamp greater than a given Timestamp (normally the last
 * known Timestamp).
 *
 * \param db_h    database handle
 * \param oid     Object-ID of the desired Object
 * \param old_ts  Timestamp to compare with the Object-Data,
 *                the commit-timestamp in the returned object will be greater than this one
 * \param data_p  Pointer to a Object-Data-Struct where the found Data will
 *                be copied
 * \param size    Maximum size the Object-Data-Struct at data_p can absorb
 * \returns       <0 on errors, size of found Object on success
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_waitnext (kogmo_rtdb_handle_t *db_h,
                                  kogmo_rtdb_objid_t oid, 
                                  kogmo_timestamp_t old_ts,
                                  void *data_p, 
                                  kogmo_rtdb_objsize_t size);


/*! \brief kogmo_rtdb_obj_readdata_waitnext() with pointer.
 *
 * \see kogmo_rtdb_obj_readdata_waitnext() and kogmo_rtdb_obj_readdata_ptr()
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_waitnext_ptr (kogmo_rtdb_handle_t *db_h,
                                      kogmo_rtdb_objid_t oid,
                                      kogmo_timestamp_t old_ts,
                                      void *data_p, 
                                      kogmo_rtdb_objsize_t size);



/*@}*/




/*! \defgroup kogmo_rtdb_dataptr C-Functions for Object Data Handling with directs Database Pointers: Do not use unless you need to.
 *
 * \brief These functions access the actual contents of the data-block
 * of a certain object.
 *
 * The difference to kogmo_rtdb_data is, that they work with pointers,
 * that point directly into the internal structures of the RTDB.
 * By misusing the pointer you get, you can severely damage the
 * internals of the RTDB.
 *
 * So please do not use those functions, unless you really need to.
 * With direct pointer operations you can save some time for objects
 * significantly larger than 10000 bytes.
 *
 */
/*@{*/


/*! \brief Get a Pointer to the latest Data of an Object within the Database
 *
 * This function returns a pointer, that points to the last object data
 * within the RTDB internals.\n
 * It finds the block with a committed_ts <= ts.
 *
 * At the end of your calculations you SHOULD call this function again and check
 * that your pointer is still the same and therefore valid!
 * If you get another pointer or an error, the data changed while you were using
 * it and your results are from corrupted data. Try it again or do an appropriate
 * error handling.
 *
 * \param db_h        Database handle
 * \param oid         Object-ID of the desired object
 * \param ts          Timestamp at which the Object must be "the last committed";
 *                    0 for "now"
 * \param data_pp     Pointer to a pointer, that will point to the object data structure for reading afterwards
 * \returns           <0 on errors, the real size of the object data found in the rtdb on success
 *
 * Example: \code
 *                kogmo_rtdb_obj_c3_blaobj_t *myobj_p;
 *                kogmo_rtdb_obj_readdata_ptr(..,&myobj_p); 
 *                access data with e.g. myobj_p->base.data_ts
 * \endcode
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_ptr (kogmo_rtdb_handle_t *db_h,
                             kogmo_rtdb_objid_t oid,
                             kogmo_timestamp_t ts,
                             void *data_pp);


/*! \brief Start pointer-based write (fast but dangerous)
 *
 * Call this function to receive a pointer where you can
 * write your data. If you are done, call kogmo_rtdb_obj_writedata_ptr_commit().
 *
 * \param db_h        Database handle
 * \param oid         Object-ID of the object to write
 * \param data_pp     Pointer to a pointer, that will point to the object data structure for writing afterwards
 * \returns           <0 on errors
 *
 *
 * \note
 *  - The object must be created and owned by the calling process.
 *  - The object must not be public writable.
 *  - The maximum length is objmeta.size_max.
 *  - You must manually set base.size before each write.
 *  - Be cautious! You can damage the whole database.
 *  - DO NOT TOUCH base.committed_ts !!!
 *  - Pointer operations are only useful for objects with sizes >= 10KB size.
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_writedata_ptr_begin (kogmo_rtdb_handle_t *db_h,
                                    kogmo_rtdb_objid_t oid,
                                    void *data_pp);


/*! \brief Finish pointer-based write and publish data
 *
 * Call this function when you are done writing your data to the area,
 * where you received a pointer from kogmo_rtdb_obj_writedata_ptr_begin().
 * DO NOT MODIFY THE DATA AFTERWARDS !!!
 *
 * \param db_h        Database handle
 * \param oid         Object-ID of the object to write
 * \param data_pp     Pointer to a pointer the object data structure, that you got from kogmo_rtdb_obj_writedata_ptr_begin()
 * \returns           <0 on errors
 *
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_writedata_ptr_commit (kogmo_rtdb_handle_t *db_h,
                                     kogmo_rtdb_objid_t oid,
                                     void *data_pp);




/*@}*/



/*! \defgroup kogmo_rtdb_until C-Functions for Waiting for Data with Timeouts (Data and Metadata)
 * These functions contain an additional parameter wakeup_ts that
 * specifies an *absolute* timestamp when to wakeup from a blocking call.
 * They return -KOGMO_RTDB_ERR_TIMEOUT in this case.
 * With 0 as wakeup_ts they behave like the function that doen't have
 * a trailing _until in its name. \n
 * kogmo_rtdb_obj_readdata_waitnext_until(..,0) = kogmo_rtdb_obj_readdata_waitnext()
 */
/*@{*/


/*! \brief kogmo_rtdb_obj_readdata_waitnext() with wakeup.
 * see: kogmo_rtdb_obj_readdata_waitnext()
 * \param wakeup_ts time at which to wake up if there is no data available.
 *        returns -KOGMO_RTDB_ERR_TIMEOUT in this case.
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_waitnext_until(
                            kogmo_rtdb_handle_t *db_h,
                            kogmo_rtdb_objid_t oid, kogmo_timestamp_t old_ts,
                            void *data_p, kogmo_rtdb_objsize_t size,
                            kogmo_timestamp_t wakeup_ts);

/*! \brief kogmo_rtdb_obj_readdata_waitnext_ptr() with wakeup.
 * see: kogmo_rtdb_obj_readdata_waitnext_ptr()
 * \param wakeup_ts time at which to wake up if there is no data available.
 *        returns -KOGMO_RTDB_ERR_TIMEOUT in this case.
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_waitnext_until_ptr(
                            kogmo_rtdb_handle_t *db_h,
                            kogmo_rtdb_objid_t oid, kogmo_timestamp_t old_ts,
                            void *data_p, kogmo_rtdb_objsize_t size,
                            kogmo_timestamp_t wakeup_ts);
/*@}*/



/*! \defgroup kogmo_rtdb_misc C-Functions to get Timestamp, etc..
 */   
/*! \brief Get absolute Timestamp for current Time within the given
 * database (can be different to kogmo_timestamp_now() when in
 * simulation mode.
 * \param db_h    database handle
 * \returns zero on errors
 */
/*@{*/
kogmo_timestamp_t
kogmo_rtdb_timestamp_now (kogmo_rtdb_handle_t *db_h);

/*! \brief Set current Time to the given absolute Timestamp.
 * This has only an effect if the rtdb is in simulation mode
 * (kogmo_rtdb_man -s) and the clients use kogmo_rtdb_timestamp_now()
 * instead of kogmo_timestamp_now()).
 * \param db_h    database handle
 * \param new_ts  new timestamp (0: give client the real current time, kogmo_timestamp_now())
 * \returns 0 for OK, -KOGMO_RTDB_ERR_NOPERM if RTDB is not in simulation mode.
 */
int
kogmo_rtdb_timestamp_set (kogmo_rtdb_handle_t *db_h,
                          kogmo_timestamp_t new_ts);
/*@}*/



/*! \addtogroup kogmo_rtdb_conn
 */
/*@{*/




  
/*! \brief Init Info for a new Database Connection
 * This does not connect. It's only for convenience.
 * See kogmo_rtdb_connect_info_t.
 *
 * \param conninfo  pointer to a Database Connection Info
 * \param dbhost    database specifier, "" for the local default database, not more than 25 characters
 * \param procname  choosen name, like a2_roadtracker
 * \param cycletime your internal cycle time in seconds (0 for system default);
 *                  you must signal your vitality every cycle with
 *                  kogmo_rtdb_cycle_done()
 * \returns         <0 on errors
 */
int
kogmo_rtdb_connect_initinfo (kogmo_rtdb_connect_info_t *conninfo,
                             _const char *dbhost,
                             _const char *procname, float cycletime);


/*! \brief Open Connection to Real-time Vehicle Database
 *
 * \param db_h      storage for the new database handle
 * \param conninfo  connection parameters and process infos
 * \returns         your process-ID (a positive number) on success,
 *                  <0 on errors
 *
 * Please note:
 *  - At process termination you should call kogmo_rtdb_disconnect()
 *  - By default there will be installed Signal-Handlers for SIG_TERM/QUIT/INT
 *    that call kogmo_rtdb_disconnect() automatically, but please don't
 *    rely on this
 */
kogmo_rtdb_objid_t
kogmo_rtdb_connect (kogmo_rtdb_handle_t **db_h,
                    kogmo_rtdb_connect_info_t *conninfo);


/* \brief Disconnect from Real-time Vehicle Database
 *
 * \param db_h      database handle
 * \param discinfo  normally NULL (might contain special flags later)
 * \returns         <0 on errors
 *
 * You should call this on process termination.
 */
int
kogmo_rtdb_disconnect (kogmo_rtdb_handle_t *db_h,
                       void *discinfo);

/* \brief Inform other Processes about your status and signal own Vitality to Real-time Vehicle Database
 *
 * \param db_h      database handle
 * \param status    your new status, one of kogmo_rtdb_obj_c3_process_status_t
 * \param msg       an arbitrary status message for humans, NULL to keep the last one,
 *                  at maximum KOGMO_RTDB_PROCSTATUS_MSG_MAXLEN characters.
 * \param flags     normally 0
 * \returns         <0 on errors
 *
 * You should call this regularly at the end of each of your cycles.
 */
int  
kogmo_rtdb_setstatus (kogmo_rtdb_handle_t *db_h, uint32_t status, _const char* msg, uint32_t flags);
// see kogmo_rtdb.hxx!
#define HAVE_DEFINED_setstatus

/* \brief Inform other Processes that you finished your cycle and signal own Vitality to Real-time Vehicle Database.
 * This is equal to kogmo_rtdb_setownstatus(db_h, KOGMO_RTDB_PROCSTATUS_CYCLEDONE, "", 0)
 *
 * \param db_h      database handle
 * \param flags     normally 0
 * \returns         <0 on errors
 *
 * You should call this (or kogmo_rtdb_setstatus()) regularly at the end of each of your cycles.
 */
int  
kogmo_rtdb_cycle_done (kogmo_rtdb_handle_t *db_h, uint32_t flags);



int
kogmo_rtdb_pthread_create(kogmo_rtdb_handle_t *db_h,
                          pthread_t *thread,
                          pthread_attr_t *attr,
                          void *(*start_routine)(void*), void *arg);
int
kogmo_rtdb_pthread_kill(kogmo_rtdb_handle_t *db_h,
                        pthread_t thread, int sig);
int
kogmo_rtdb_pthread_join(kogmo_rtdb_handle_t *db_h,
                        pthread_t thread, void **value_ptr);

// NOTE: kogmo_rtdb_sleep_until() in simulation mode:
//       => works only if simulation time runs slower than real-time! (true in most cases)
int
kogmo_rtdb_sleep_until(kogmo_rtdb_handle_t *db_h,
                       _const kogmo_timestamp_t wakeup_ts);

/*@}*/




/*! \defgroup kogmo_rtdb_error C-Functions Return Values and Debugging
 * \brief These values can be returned by Calls to the Database-API.
 * They are returned as an integer (int) or a type kogmo_rtdb_objid_t.\n
 * So far, they are based on errno.h for simplicity. \n
 *
 * Please note:
 *  - All error values returned are negative!
 */
/*@{*/

// angelehnt an /usr/include/asm/errno.h:
#define KOGMO_RTDB_ERR_NOPERM   EACCES //!< 13 Permission denied
#define KOGMO_RTDB_ERR_NOTFOUND ENOENT //!< 2  Object/Process not found
#define KOGMO_RTDB_ERR_NOMEMORY ENOMEM //!< 12 Out of Memory
#define KOGMO_RTDB_ERR_INVALID  EINVAL //!< 22 Invalid Argument(s)
#define KOGMO_RTDB_ERR_OUTOFOBJ EMFILE //!< 24 Out of Object/Process Slots
#define KOGMO_RTDB_ERR_NOTUNIQ  EEXIST //!< 17 Unique Object already exists
#define KOGMO_RTDB_ERR_UNKNOWN  EPERM  //!< 1  General/unspecified error (-1)
#define KOGMO_RTDB_ERR_CONNDENY ECONNREFUSED //!< 111 Connection refused           
#define KOGMO_RTDB_ERR_NOCONN   ENOTCONN //!< 107 Not connected to the RTDB, can be returned by any function with a database handle,
                                         //!  even if not explicitly specified as possible return value
#define KOGMO_RTDB_ERR_HISTWRAP ESTALE //!< 116 History wrap-around, stale data
#define KOGMO_RTDB_ERR_TOOFAST  EAGAIN //!< 11 Updates too fast, try again
#define KOGMO_RTDB_ERR_TIMEOUT  ETIMEDOUT //!< Waiting for data timed out

/// current debug-level (internal)
extern int kogmo_rtdb_debug;
/*@}*/










/*! \brief Temporary solution for simulation because of thread-blocking (boost problem?)
 * similar to kogmo_rtdb_obj_searchinfo_waitnext() but non-blocking.
 */
int
kogmo_rtdb_obj_searchinfo_lists_nonblocking(kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_rtdb_objid_list_t known_idlist,
                           kogmo_rtdb_objid_list_t added_idlist,
                           kogmo_rtdb_objid_list_t deleted_idlist);


/*! \brief (internal) Function to access object history slot-by-slot
 * mode: 0=get latest + init objslot (use this first)
 *       1=check old objslot + use offset for reading (use this afterwards and set offset)
 *      -1=use offset for reading (don't check old objslot - will not detect a wrap-around) 
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdataslot_ptr (kogmo_rtdb_handle_t *db_h,
                                 int32_t mode, int32_t offset,  
                                 kogmo_rtdb_obj_slot_t *objslot,
                                 void *data_pp);


/*! \brief Comfortable Function to access object history slot-by-slot
 * see examples/kogmo_rtdb_slotreader.c for an example
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdataslot_init (kogmo_rtdb_handle_t *db_h,
                                  kogmo_rtdb_obj_slot_t *objslot,
                                  kogmo_rtdb_objid_t oid);
 
/*! \brief Comfortable Function to access object history slot-by-slot
 * see examples/kogmo_rtdb_slotreader.c for an example
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdataslot_relative (kogmo_rtdb_handle_t *db_h,
                                      kogmo_rtdb_obj_slot_t *objslot,
                                      int offset,
                                      void *data_p,
                                      kogmo_rtdb_objsize_t size);

/*! \brief Comfortable Function to access object history slot-by-slot
 * 
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdataslot_relative_ptr (kogmo_rtdb_handle_t *db_h,
                                          kogmo_rtdb_obj_slot_t *objslot,
                                          int offset,  
                                          void *data_pp);

/*! \brief Comfortable Function to access object history slot-by-slot
 * 
 */
kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdataslot_check (kogmo_rtdb_handle_t *db_h,
                                   kogmo_rtdb_obj_slot_t *objslot);


#ifdef __cplusplus
 }; /* extern "C" */
 }; /* namespace KogniMobil */
#endif


#endif /* KOGMO_RTDB_FUNCS_H */
