/*! \file kogmo_rtdb_obj_base.h
 * \brief Basis Object (C-Interface) and Essential RTDB-Objects.
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */
#ifndef KOGMO_RTDB_OBJ_BASE_H
#define KOGMO_RTDB_OBJ_BASE_H

//#ifndef KOGMO_RTDB_H
//	#ifndef KOGMO_RTDB_DONT_INCLUDE_ALL_OBJECTS
//	#define KOGMO_RTDB_DONT_INCLUDE_ALL_OBJECTS //automatically don't include all objects, if an object file was included manually
//	#endif
#//endif

#include "kogmo_rtdb_types.h"

#ifdef __cplusplus
 namespace KogniMobil {
 extern "C" {
#endif

/*! \addtogroup kogmo_rtdb_objects C-Structures for Data Objects (Basis Object and Derivates)
 * \brief These Structs define the Data-Block of every Object.
 * The Data-Block ist the Part of the Object that normally
 * changes every cycle by issuing a commit.
 *
 * Every Object must start with a kogmo_rtdb_subobj_base_t.
 */
/*@{*/


/*! \brief Basis Object that every Data Object has to start with (has to inherit);
 * This is the Part of the Object that holds the basis data.
 * This Struct must be the Beginn of the Data-Block of every Object.
 *
 * Fields are marked as follows:
 *  - (U) Fields that can be filled by the user
 *  - (D) Fields that will be managed by the database and will be overwritten
 *
 * See kogmo_rtdb_obj_box_t for an example
 */

typedef PACKED_struct
{
  kogmo_timestamp_t	committed_ts;
    //!< (D) Time at which this Data has been committed
    //!< (set automatically on commit)
    // RTDB-internal usage:
    //  - 0 means, that this slot is empty or in preparation
    //  - if committed_ts changes, the slot has been overwritten
    //  - at the beginn of a commit committed_ts is set to 0,
    //    at the end committed_ts is set with the new value
    //    => as long as committed_ts is constant, the data is valid
  kogmo_rtdb_objid_t	committed_proc;
    //!< (D) Process-ID of the Process that commiting the data
    //!< (set automatically on commit)
  kogmo_timestamp_t	data_ts;
    //!< (U) Time at which this Data has been created;
    //!< will be filled with current Time if 0
  kogmo_rtdb_objsize_t	size;
    //!< (U) Size of the whole Data-Object, at least sizeof(kogmo_rtdb_subobj_base_t),
    //!< at maximum the size specified in the object-metadata;
    //!< This is necessary for variable-sized objects, e.g. containing
    //!< xml-data with knowledge descriptions
} kogmo_rtdb_subobj_base_t;

/*! \brief Full Object with only the basis data
 */
typedef PACKED_struct
{
  kogmo_rtdb_subobj_base_t base;
} kogmo_rtdb_obj_base_t;



/*! \brief An Objects that holds Information about the RTDB.
 * This is an essential object and is also used internally by the rtdb.
 */
typedef PACKED_struct
{
 kogmo_timestamp_t    started_ts;
 char                 dbhost[KOGMO_RTDB_OBJMETA_NAME_MAXLEN];
 uint32_t             version_rev; // 123
 char                 version_date[30]; // 2006-09-19 11:23:51 +01:00:00
 char                 version_id[402];
 uint32_t             objects_max;
 uint32_t             objects_free;
 uint32_t             processes_max;
 uint32_t             processes_free;
 kogmo_rtdb_objsize_t memory_max;
 kogmo_rtdb_objsize_t memory_free;
} kogmo_rtdb_subobj_c3_rtdb_t;

/*! \brief Full Object for RTDB Info
 */
typedef PACKED_struct
{
  kogmo_rtdb_subobj_base_t base;
  kogmo_rtdb_subobj_c3_rtdb_t rtdb;
} kogmo_rtdb_obj_c3_rtdb_t;




typedef enum
{
  KOGMO_RTDB_PROCSTATUS_UNKNOWN   = 0,
  //!< Default beim Start, bedeutet dass der Prozess noch keinen Status selbst gesetzt hat
  KOGMO_RTDB_PROCSTATUS_CYCLEDONE, // == "KOGMO_RTDB_PROCSTATUS_OK"
  //!< Prozess hat gerade einen Zyklus beendet, und alle relevanten Objekte geschrieben.
  //!< Hierauf koennen andere Prozesse triggern.
  KOGMO_RTDB_PROCSTATUS_BUSY,
  //!< Prozess rechnet
  KOGMO_RTDB_PROCSTATUS_WAITING,
  //!< Prozess wartet (auf Objekt, I/O, Benutzer, etc.)
  KOGMO_RTDB_PROCSTATUS_FAILURE,
  //!< Prozess hat einen schweren Fehler selbst festgestellt, haengt und kann nicht weiterrechnen.
  //!< Koennte man vor dem exit() aufrufen.
  KOGMO_RTDB_PROCSTATUS_WARNING,
  //!< Prozess hat eine Warnung
  //
  // hier koennte man weitere relevante Zustaende ergaenzen (bitte erst fragen)
  // man kann auch einen Zusatz zu seinem Status als ASCII-Text setzen (->kogmo_rtdb_setstatus())
} kogmo_rtdb_obj_c3_process_status_t;

/*! \brief This object contains information about a process connected to the rtdb.
 * This is an essential object and is also used internally by the rtdb.
 */
typedef PACKED_struct {
 kogmo_rtdb_objid_t proc_oid;
   //!< Process-ID that is unique during runtime on the local system;
   //!< TODO: inter-Vehicles - Communication between Processes;
   //!< Processes have their own ID-space (so far)!
   //!< (this data is from ipc-connection-management
   //!< and will be partly copied into the object metadata)
 uint32_t           pid;
 uint32_t           tid;
 uint32_t           flags;
 uint32_t           status;
#define KOGMO_RTDB_PROCSTATUS_MSG_MAXLEN 160
 char               statusmsg[KOGMO_RTDB_PROCSTATUS_MSG_MAXLEN];
 uint32_t           version;
 uint32_t           uid;
 // kogmo_rtdb_objid_t wait_oid;
} kogmo_rtdb_subobj_c3_process_t;


/*! \brief Full Object for Process Info.
 */
typedef PACKED_struct
{
  kogmo_rtdb_subobj_base_t base;
  kogmo_rtdb_subobj_c3_process_t process;
} kogmo_rtdb_obj_c3_process_t;




/*@}*/

#ifdef __cplusplus
 }; /* extern "C" */
 }; /* namespace KogniMobil */
#endif

#endif /* KOGMO_RTDB_OBJ_BASE_H */


/*!
 * \brief Recorder and Player Objects from C3
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_OBJ_C3_PLAYREC_H
#define KOGMO_RTDB_OBJ_C3_PLAYREC_H

//#include "kogmo_rtdb_obj_packing.h"
//#include "kogmo_rtdb_obj_base.h"

#ifdef __cplusplus
 extern "C" {
 namespace KogniMobil {
#endif

/*! \addtogroup kogmo_rtdb_objects */
/*@{*/


/*! \brief Control Object for the RTDB-Player
 */
typedef PACKED_struct
{
  struct
  {
    uint32_t log : 1;
    uint32_t pause : 1;
    uint32_t loop : 1;
    uint32_t keepcreated : 1;
    uint32_t scan : 1;
  } flags;
  float speed;
  kogmo_timestamp_t begin_ts; // begin playing here after loop
  kogmo_timestamp_t end_ts;   // end playing here (and loop if desired)
  kogmo_timestamp_t goto_ts; // != 0 => seek to this position
  kogmo_rtdb_objname_t frame_objname;
  int16_t frame_go; // -+X > 0 ==> play until X objects with name "frame_objname" have passed
} kogmo_rtdb_subobj_c3_playerctrl_t;

typedef PACKED_struct
{
  kogmo_rtdb_subobj_base_t base;
  kogmo_rtdb_subobj_c3_playerctrl_t playerctrl;
} kogmo_rtdb_obj_c3_playerctrl_t;


/*! \brief Status Object for the RTDB-Player
 */
typedef PACKED_struct
{
  kogmo_timestamp_t current_ts;
  kogmo_timestamp_t first_ts;
  kogmo_timestamp_t last_ts;
} kogmo_rtdb_subobj_c3_playerstat_t;

typedef PACKED_struct
{
  kogmo_rtdb_subobj_base_t base;
  kogmo_rtdb_subobj_c3_playerstat_t playerstat;
} kogmo_rtdb_obj_c3_playerstat_t;




/*! \brief Status Object for the RTDB-Recorder
 * TEMP! will be extended without notice!
 */
typedef PACKED_struct
{
  kogmo_timestamp_t begin_ts;
  uint64_t bytes_written;
  uint32_t events_written;
  uint32_t events_total;
  uint32_t events_lost;
  int16_t event_buffree;
  uint16_t event_buflen;
} kogmo_rtdb_subobj_c3_recorderstat_t;

/*! \brief Full Object with RTDB-Recorder Status
 */
typedef PACKED_struct
{
  kogmo_rtdb_subobj_base_t base;
  kogmo_rtdb_subobj_c3_recorderstat_t recorderstat;
} kogmo_rtdb_obj_c3_recorderstat_t;


/*! \brief Control Object for the RTDB-Recorder
 * ==> NOT IN USE YET!
 */
typedef PACKED_struct
{
#define C3_RECORDERCTRL_MAXLIST 50
  // include those objects
  kogmo_rtdb_objid_t     oid_list[C3_RECORDERCTRL_MAXLIST];
  kogmo_rtdb_objtype_t   tid_list[C3_RECORDERCTRL_MAXLIST];
  char                   name_list[C3_RECORDERCTRL_MAXLIST][KOGMO_RTDB_OBJMETA_NAME_MAXLEN];
  // exclude those objects
  kogmo_rtdb_objid_t     xoid_list[C3_RECORDERCTRL_MAXLIST];
  kogmo_rtdb_objtype_t   xtid_list[C3_RECORDERCTRL_MAXLIST];
  char                   xname_list[C3_RECORDERCTRL_MAXLIST][KOGMO_RTDB_OBJMETA_NAME_MAXLEN];
  // diese werden als "richtige" bilder ins avi geschrieben (kann waehrend der aufnahme nicht geaendert werden)
  kogmo_rtdb_objid_t     oid_stream[10];
  struct
  {
    uint32_t select_all : 1;
    uint32_t recorder_on : 1;
//    uint32_t record_pause : 1; // gibt's noch nicht. wird das file nicht schliessen
    uint32_t logging_on : 1;
  } switches;
} kogmo_rtdb_subobj_c3_recorderctrl_t;

/*! \brief Full Object with RTDB-Recorder Control
 */
typedef PACKED_struct
{
  kogmo_rtdb_subobj_base_t base;
  kogmo_rtdb_subobj_c3_recorderctrl_t recorderctrl;
} kogmo_rtdb_obj_c3_recorderctrl_t;





/*! \brief Plain ASCII-Text Object
 */
typedef struct
{
 // String does not need to be null-terminated, but it can be.
 // DO NOT use the last character! The reader might use it
 // for its own null-termination.
#define C3_TEXT_MAXCHARS (1024*100)
 char data[C3_TEXT_MAXCHARS];
} kogmo_rtdb_subobj_c3_text_t;

typedef struct
{
  kogmo_rtdb_subobj_base_t base;
  kogmo_rtdb_subobj_c3_text_t text;
} kogmo_rtdb_obj_c3_text_t;

/*! \brief Small (1k) Plain ASCII-Text Object
 * (can share the same type-id with kogmo_rtdb_obj_c3_text_t)
 */

typedef struct
{
 char data[1024];
} kogmo_rtdb_subobj_c3_text1k_t;

typedef struct
{
  kogmo_rtdb_subobj_base_t base;
  kogmo_rtdb_subobj_c3_text1k_t text;
} kogmo_rtdb_obj_c3_text1k_t;


/*! \brief Object with Integer Values without specific Meaning (for Examples and quick Tests)
 */
typedef struct
{
 int32_t intval[256];
} kogmo_rtdb_subobj_c3_ints256_t;

typedef struct
{
  kogmo_rtdb_subobj_base_t base;
  kogmo_rtdb_subobj_c3_ints256_t ints;
} kogmo_rtdb_obj_c3_ints256_t;


/*! \brief Object with Float Values without specific Meaning (for Examples and quick Tests)
 */
typedef struct
{
 int32_t floatval[256];
} kogmo_rtdb_subobj_c3_floats256_t;

typedef struct
{
  kogmo_rtdb_subobj_base_t base;
  kogmo_rtdb_subobj_c3_floats256_t floats;
} kogmo_rtdb_obj_c3_floats256_t;




/*! \brief Ein Videobild
 * (aehnlich OpenCV)
 * Aendert sich mit jedem Zyklus.
 * Ein Kamerabild hat als Vater ein kogmo_rtdb_obj_a2_camera_t.
 */
typedef struct
{
  int width;            //!< Breite, Nullpunkt links   [bitte keine negativen Werte mehr verwendent, fruehere Festlegung: "negative Werte: Bild horizontal gespiegelt, Nullpunkt rechts")
                        // Die Breite sollte durch 4 teilbar sein, sonst kann man die KogMo-RTDB-AVIs evtl nicht mit einem anderen AVI-Player abspielen (macht aber nichts)
  int height;           //!< Hoehe, Nullpunkt oben     [bitte keine negativen Werte mehr verwendent, fruehere Festlegung: "negative Werte: Bild vertikal gespiegelt, Nullpunkt unten")
  int depth;            // Bits/Pixel pro Channel, diese heissen A2_IMAGE_DEPTH_8U usw. und entsprechen opencv/cxtypes.h (s.u.)
  int channels;         // number of color channels, 1 for greyscale, 3 for rgb (nach opencv)
  int colormodell;      // 0=rgb interleaved, .. (*heul* schreibfehler! muesste "colormodel" heissen:-()
                        // Farb-Byte-Order: B-G-R, wie Windows-Bitmap (=>AVI-Standard) und wie fuer cvShowImage(), Achtung: das PPM-Format hat R-G-B!
  int widthstep;        // Breite einer Zeile in Bytes einschliesslich Alignment (see opencv)
                        // Muss durch 4 teilbar sein, damit ein aufgezeichnetes AVI von Standardplayern abspielbar ist.
  unsigned char data[0];// nur, um die Adresse des ersten Pixels zu bekommen
                        // => size = sizeof(kogmo_rtdb_subobj_a2_image_t) + height*widthstep !!
} kogmo_rtdb_subobj_a2_image_t;

// Definierte Werte fuer depth (entsprechen den jeweiligen OpenCV/IPL-Definitionen, siehe opencv/cxtypes.h)
// KEINE EIGENEN MODI ohne Absprache mit A2 erfinden!
#define A2_IMAGE_DEPTH_1U           1  // 1 Bits/Pixel, unsigned char  (==IPL_DEPTH_1U)
#define A2_IMAGE_DEPTH_8U           8  // 8 Bits/Pixel, unsigned char  (==IPL_DEPTH_8U)
#define A2_IMAGE_DEPTH_16U         16  // 16 Bits/Pixel, unsigned word (==IPL_DEPTH_16U)
#define A2_IMAGE_DEPTH_32F         32  // 32 Bits/Pixel, float         (==IPL_DEPTH_32F)
#define A2_IMAGE_DEPTH_8S  0x80000008  // 8 Bits/Pixel, signed char    (==IPL_DEPTH_8S==IPL_DEPTH_SIGN|IPL_DEPTH_8U)
#define A2_IMAGE_DEPTH_16S 0x80000016  // 16 Bits/Pixel, signed char    (==IPL_DEPTH_16S==IPL_DEPTH_SIGN|IPL_DEPTH_16U)
#define A2_IMAGE_COLORMODEL_RGB    0   // Bei 1 Kanal: Graustufenbild, bei 3 Kanaelen: RGB
#define A2_IMAGE_COLORMODEL_YUV411 1   // YUV411
#define A2_IMAGE_COLORMODEL_YUV422 2   // YUV422
#define A2_IMAGE_COLORMODEL_YUV444 3   // YUV444
#define A2_IMAGE_COLORMODEL_RGBJPEG 16 // wie 0, aber JPEG komprimiert

/*! \brief Vollstaendiges Objekt fuer ein Videobild
 */
typedef struct
{
  kogmo_rtdb_subobj_base_t base;
  kogmo_rtdb_subobj_a2_image_t image;
} kogmo_rtdb_obj_a2_image_t;






/*@}*/

#ifdef __cplusplus
 }; /* namespace KogniMobil */
 }; /* extern "C" */
#endif

#endif /* KOGMO_RTDB_OBJ_C3_PLAYREC_H */
