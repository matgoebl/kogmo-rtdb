/*! \file kogmo_rtdb_types.h
 * \brief Types for the contained Objects in the Real-time Vehicle Database
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_TYPES_H
#define KOGMO_RTDB_TYPES_H

#include <inttypes.h>   /* (u)int??_t */

// fallback for me (Matthias), please look at kogmo_objects/kogmo_rtdb_obj_packing.h !
#if defined(__GNUC__) && !defined(PACKED_struct)
 #define PACKED_struct struct __attribute__((packed))
#endif

#ifndef KOGMO_RTDB_REV
 #define KOGMO_RTDB_REV 540
#endif

   
#ifdef __cplusplus
 namespace KogniMobil {
 extern "C" {
#endif


/*! \defgroup kogmo_rtdb_types C-Types and Structures for Database Objects
 * \brief These Types are used as Parameters in Calls to the Database-API
 * and should be used in Database Objects if possible.
 */
/*@{*/


/*! \brief ID for Objects within the Database that are unique during database
 * runtime.
 * IDs are not unique between database restarts (so far).
 *
 * Please note:
 *  - This could become int64_t some day
 *  - So please use this type and print it with
 *    \code printf("%lli",(long long int)oid) \endcode
 *  - 0 is always invalid and may represent unused entries or
 *    "not found" as return value
 *  - <0 represents errors as return value
 *
 * TODO:
 *  - Decide what to for Object-Interchange between Vehicles:
 *    - Define separate ID-Ranges
 *    - When synchronizing (and security checking!) foreign objects, use ID mapping
 */
typedef int32_t kogmo_rtdb_objid_t;


/*! \brief Type-Identifier for Objects within the Database that is unique
 * forever.
 * This is to avoid missinterpretation when using old data sets.
 *
 * Please note:
 *  - This could become int64_t some day
 *  - So please use this type and print it with
 *    \code printf("%lli",(long long int)otype) \endcode
 *  - 0 is always invalid and may represent unused entries or
 *    "not found" as return value. As search parameter it means "any type".
 *  - 1 is "unknown object" (only basedata ist specified, length
 *    of additional data is free
 *  - <0 represents errors as return value
 *
 * TODO:
 *  - Define
 *  - Define Ranges for separate Sub-Projects
 *  - Define Version-Information
 */
typedef int32_t kogmo_rtdb_objtype_t;


/*! \brief Size-Specification for Objects within the Database.
 *
 * Please note:
 *  - This could become int64_t some day
 *  - So please use this type and print it with
 *    \code printf("%lli",(long long int)osize) \endcode
 *  - <0 represents errors as return value
 *
 * TODO:
 *  - use consequently
 */
typedef int32_t kogmo_rtdb_objsize_t;




//! see kogmo_rtdb_obj_info_t
#define KOGMO_RTDB_DEFAULT_HISTORY_INTERVAL 10.0
//! see kogmo_rtdb_obj_info_t
#define KOGMO_RTDB_DEFAULT_MAX_CYCLETIME    0.030
//! how long to keep deleted objects if history is shorter
//! (the manager has an additional purge interval of 0.25 sec)
#define KOGMO_RTDB_KEEP_DELETED_INTERVAL    0.25

//! maximum size for object names in kogmo_rtdb_obj_info_t
//! (including terminating null-byte) 
#define KOGMO_RTDB_OBJMETA_NAME_MAXLEN      32

//! \brief Type for a Object-Name
//! (see kogmo_rtdb_obj_info_t)
typedef char kogmo_rtdb_objname_t [KOGMO_RTDB_OBJMETA_NAME_MAXLEN];

//! see 
#define KOGMO_RTDB_OBJIDLIST_MAX 1000
/*! \brief Type for a pre-allocated Array to receive the results of
 * kogmo_rtdb_obj_readdatameta().
 * It is terminated by a 0-element.
 */
typedef kogmo_rtdb_objid_t kogmo_rtdb_objid_list_t [KOGMO_RTDB_OBJIDLIST_MAX+1];



/*! \brief Data Block that contains Object-Metadata.
 * The Metadata of an Object should not (or only very slowly) change
 * during its livetime.
 *
 * \note
 *  - So far, metadata-updates are forbidden
 *  - If something changes, put it into the data-section
 *  - metadata-updates would require additional triggers for metadata-changes,
 *    that would increase complexibility
 *  - So far, the parent-ID is fixed, because it is included in the metadata.
 *    Until now, if the parent changes, the object has to be deleted and
 *    inserted with a new parent. Applications will show if this works.
 *
 * Fields are marked as follows:
 *  - (U) Fields that can be filled by the user
 *  - (D) Fields that will be managed by the database and will be overwritten
 *  - (I) Fields that are used internally by the database and should be ignored
 */

typedef PACKED_struct
{
  kogmo_rtdb_objid_t    oid;
    //!< (D) Unique Object-ID, will be automatically created and overwritten

  kogmo_rtdb_objname_t  name;
    //!< (U) Name of the Object, doesn't need to be unique

  kogmo_timestamp_t     created_ts;
    //!< (D) Time of Object creation

  kogmo_rtdb_objid_t    created_proc;
    //!< (D) Process-ID of the Process, that created the Object

  kogmo_timestamp_t     lastmodified_ts;
    //!< (D) (unused so far)
  kogmo_rtdb_objid_t    lastmodified_proc;
    //!< (D) (unused so far)

  kogmo_timestamp_t     deleted_ts;
    //!< (D) Time of Object deletion; metadata has to be kept until
    //!< the last Object dataset is beyond history limit
  kogmo_rtdb_objid_t    deleted_proc;
    //!< (D) Process that deleted the Object (so far only the owner)

  kogmo_rtdb_objid_t    parent_oid;
    //!< (U) Object-ID of the Parent-Object within the Scene-Tree;
    //!< Could be 0 if unwanted.

  kogmo_rtdb_objtype_t  otype;
    //!< (U) Type of the Object;
    //!< Default: KOGMO_RTDB_DEFAULT_TYPE
  
  kogmo_rtdb_objsize_t  size_max;
    //!< (U) Maximum Objectdata-Size that can be used in a commit

  float                 history_interval;  
    //!< (U) Time Interval in Seconds the History of Object-Data should
    //!< be kept;
    //!< 0: use Default

  // Ugly, but for compatibility:
  #define min_cycletime avg_cycletime
  // NEW! Reused, was avg_cycletime
  float                 min_cycletime;
    //!< (U) Minimum time between Object-Changes (commits).
    //!< Bad things might happen if exceeded (so far, the history will be
    //!< shorter). Can be enforced by setting the cycle_watch flag;
    //!< \n Note 1: If min_cycletime > max_cycletime will be automatically swapped internaly when used,
    //!< but the kogmo_rtdb_obj_info_t will not be touched so far.
    //!< \n Note 2: The meaning of max_cycletime / min_cycletime are swapped since
    //!< the very first release, because I mixed up max_cycletime and max_cyclefreq.
    //!< \n 0: use Default
  float                 max_cycletime;
    //!< (U) Maximum time between Object-Changes (commits);
    //!< If exceeded, the latest data is regarded stale.
    //!< If withhold_stale is set, readers will not get this stale data.
    //!< See also min_cycletime (=avg_cycletime) for notes.

  struct
    {
      uint32_t read_deny : 1;
        //!< (U) 1: Object Data only readable by own handle;
        //!< 0: default: public readable
      uint32_t write_allow : 1;
        //!< (U) 1: Object writable by others, they can update and delete the object;
        //!< 0: default: not public writable
      uint32_t keep_alloc : 1;
        //!< (U) 1: Allocated Memory for Object will be kept after deletion,
        //!< and will be re-used for the next new object of the same type
        //!< created by the same process.
        //!< Warning: This is only a hint to the rtdb, don't assume that
        //!< you'll get the same ids and pointers. Just assume that you
        //!< get a totally new object, but expect less time for creation.
        //!< 0: default: free all memory after deletion
      uint32_t unique : 1;
        //!< (U) 1: Object must be unique - a second object with the same name and
        //!< object type (otype) will be refused;
        //!< 0: default: not public writable
      uint32_t cycle_watch : 1;
        //!< (U) 1: Object Data Updates cannot occur at a higher rate than max_cycletime,
        //1< The committing process will be delayed if it tries;
        //!< 0: default: do not watch and enforce the maximum update rate;
      uint32_t persistent : 1;
        //!< (U) 1: Object will remain in RTDB even if the creating process dies.
        //!< It's recommended to also set write_allow.
        //!< 0: default: not persistent
      uint32_t parent_delete : 1;
        //!< (U) 1: Object will be deleted if the parent object is deleted.
        //!< It's useful in combination with persistent and write_allow.
        //!< 0: default: survive parent.
      uint32_t no_notifies : 1;
        //!< (U) 1: Do not send notifies on object writes.
        //!< This makes commiting even faster and is only useful for very high frequent
        //!< real-time processes. Other processes that use kogmo_rtdb_obj_readdata_waitnext()
        //!< on this object, will automatically poll for changes (with about 0.1*avg_cycletime).
        //!< So by concept you get a new value every time you use kogmo_rtdb_obj_readdata().
        //!< 0: default: send notifies.
      uint32_t immediately_delete : 1;
        //!< (U) 1: Object will be deleted immediate. Use this only for huge rawdata objects.
        //!< 0: default: delete after 0.25-0.5 seconds, so an old version of this objects can
        //!< be used for error recovery.
      uint32_t withhold_stale : 1;
        //!< (U) 1: If the age of the latest object data is below the specified max_cycletime,
        //   a read request for the latest date will fail.
        //!< 0: default: a read request always gives out data, even if it's stale.
        //!< be used for error recovery.
    } flags;

  int32_t               history_size;
    //!< (I) Current calculated amount history slots
  int32_t               history_slot;
    //!< (I) Latest history slot
  kogmo_rtdb_objsize_t  buffer_idx;
    //!< (I) Pointer to Object-Data within the Heap

} kogmo_rtdb_obj_info_t;


/*@}*/




/*! \defgroup kogmo_rtdb_slot C-Functions for direct History Data Slot Access
 * \brief These Types are used as Parameters for some specializes Calls to the Database-API.
 */
/*@{*/


/*! \brief Specification of a certain history slot of an object for fast access.
 */

typedef PACKED_struct
{
  int32_t               state; // reserved
  kogmo_rtdb_objid_t    oid;
  kogmo_timestamp_t     committed_ts;
  int32_t               object_slot;
  int32_t               history_slot;
  int32_t               reserved0;
  int32_t               reserved1;
} kogmo_rtdb_obj_slot_t;


/*@}*/






/*! \addtogroup kogmo_rtdb_conn
 */
/*@{*/

/*! \brief Data Block for requesting a Database-Connection
 * This is used only once for establishing the Database Connection.
 * By using a struct, we can add more fields later.
 */

typedef struct
{
  _const char *procname;
    //!< choosen name for the process that connects to the database;
    //!< example: a2_roadtracker (should begin with subproject number)
  float cycletime;
    //!< your internal cycle time (0 for system default);
    //!< you must signal your vitality every cycle with kogmo_rtdb_cycle_done();
    //!< example: 0.033 seconds for a video-process with 30 frames/s
  uint32_t flags;
    //!< normally 0
  _const char *dbhost;
    //!< set to NULL or "" for default local database;
    //!< (for future extendability, more local databases, network, etc.)
  uint32_t version;
    //!< your version identification (SVN revision or similar)
} kogmo_rtdb_connect_info_t;


/*! \brief Flags to modify RTDB Connections
 * These have to be ORed and set to kogmo_rtdb_connect_info_t.flags
 * or as argument to RTDBConn ().
 * Please do not try other values!
 */

#define KOGMO_RTDB_CONNECT_FLAGS_NOHANDLERS 0x0001
 //!< Normally, every process using the rtdb-libraries, will get signal-handlers
 //!< to make sure that kogmo_rtdb_disconnect() is called at programm termination.
 //!< By setting this flag you can prevent this, but PLEASE call kogmo_rtdb_disconnect(),
 //!< otherwise the programms object won't get removed immediately.
#define KOGMO_RTDB_CONNECT_FLAGS_TRYNOWAIT  0x0004
 //!< Normally, if there's no database to connect to,
 //!< kogmo_rtdb_connect() just waits until it finds one.
 //!< This way, you can start your applications before the
 //!< database.
 //!< By setting this flag kogmo_rtdb_connect() returns
 //!< without waiting, if there's no database.
 //!< Depending on KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR
 //!< your application terminates or gets a negative return code.
#define KOGMO_RTDB_CONNECT_FLAGS_SILENT     0x0008
 //!< Do not output error messages, useful in combination with
 //!< TRYNOWAIT and LIVEONERR.
#define KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR  0x0010
 //!< Normally, if there are problems connecting to
 //!< the database, your application terminates.
 //!< By setting this flag your application just
 //!< gets a negative return code for kogmo_rtdb_connect(),
 //!< an lives on.
#define KOGMO_RTDB_CONNECT_FLAGS_IMMEDIATELYDELETE 0x0020
 //!< If this process dies, immediately delete all its objects.
 //!< Please use this only for processes that need much memory for
 //!< raw data acquisition (and for the player).
#define KOGMO_RTDB_CONNECT_FLAGS_REALTIME   0x0100
 //!< (internal, ask matthias.goebl*goebl.net before use)

/*@}*/


#ifndef KOGMO_RTDB_DONT_DEFINE_HANDLE
typedef struct {
  char dummy[80];
} kogmo_rtdb_handle_t;
#endif




#ifdef __cplusplus
 }; /* extern "C" */
 }; /* namespace KogniMobil */
#endif

#endif /* KOGMO_RTDB_TYPES_H */
