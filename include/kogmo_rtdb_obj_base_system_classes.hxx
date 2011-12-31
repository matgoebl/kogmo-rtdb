/*! \file kogmo_rtdb_obj_base_system_classes.hxx
 * \brief Objects from C3
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_OBJ_BASE_SYSTEM_CLASSES_HXX
#define KOGMO_RTDB_OBJ_BASE_SYSTEM_CLASSES_HXX

#include "kogmo_rtdb.hxx"
#include <sstream> // std::ostringstream
#include <cmath>   // double std::abs(double)
#include <stdio.h> // snprintf
using namespace std;

namespace KogniMobil {

/*! \addtogroup kogmo_rtdb_objects_cxx
 */
/*@{*/





typedef RTDBObj_T < kogmo_rtdb_subobj_c3_playerstat_t, KOGMO_RTDB_OBJTYPE_C3_PLAYERSTAT, RTDBObj > C3_PlayerStat_T;
class C3_PlayerStat : public C3_PlayerStat_T
{
  public:
    C3_PlayerStat(class RTDBConn& DBC, const char* name = "") : C3_PlayerStat_T (DBC, name)
      {
      }
    std::string dump (void) const
      {
        std::ostringstream ostr;
        ostr << "* Player Status :" << std::endl
             << "Begin of Recording:   " << Timestamp(subobj_p->first_ts).string() << std::endl
             << "Latest Time in Recording:    " << Timestamp(subobj_p->last_ts).string() << std::endl
             << "Current Time Playing: " << Timestamp(subobj_p->current_ts).string()
             << " ( " << (Timestamp(subobj_p->current_ts)-Timestamp(subobj_p->first_ts)) << " secs )" << std::endl;
        return RTDBObj::dump() + ostr.str();
      };
};

typedef RTDBObj_T < kogmo_rtdb_subobj_c3_playerctrl_t, KOGMO_RTDB_OBJTYPE_C3_PLAYERCTRL, RTDBObj > C3_PlayerCtrl_T;
class C3_PlayerCtrl : public C3_PlayerCtrl_T
{
  public:
    C3_PlayerCtrl(class RTDBConn& DBC, const char* name = "") : C3_PlayerCtrl_T (DBC, name)
      {
      }
    void RTDBWrite (int32_t size = 0, Timestamp ts = 0)
      {
        // In Simulation Mode the Time is stopped when in Pause,
        // so we have to alter the timestamp to notify the player:
        Timestamp LastTS = getDataTimestamp();
        setDataTimestamp();
        if ( LastTS == getDataTimestamp() )
          setDataTimestamp( 1 + LastTS );
        RTDBObj::RTDBWrite (size,ts);
      }
    std::string dump (void) const
      {
        std::ostringstream ostr;
        ostr << "* Player Control :" << std::endl
             << "Pause:      " << (subobj_p->flags.pause ? "ON" : "off") << std::endl
             << "Scanning:   " << (subobj_p->flags.scan ? "ON" : "off") << std::endl
             << "Logging:    " << (subobj_p->flags.log ? "ON" : "off") << std::endl
             << "Loop:       " << (subobj_p->flags.loop ? "ON" : "off") << std::endl
             << "KeepCreated:" << (subobj_p->flags.keepcreated ? "ON" : "off") << std::endl
             << "Speed      :" << (subobj_p->speed*100.0) << "%" << std::endl
             << "Loop Start: " << Timestamp(subobj_p->begin_ts).string() << std::endl
             << "Loop End:   " << Timestamp(subobj_p->end_ts).string() << std::endl
             << "GoTo Time:  " << Timestamp(subobj_p->goto_ts).string() << std::endl
             << "By-Frame Go:" << subobj_p->frame_go << " " << subobj_p->frame_objname << std::endl;
        return RTDBObj::dump() + ostr.str();
      };
};

typedef RTDBObj_T < kogmo_rtdb_subobj_c3_playerctrl_t, KOGMO_RTDB_OBJTYPE_C3_PLAYERCTRL, RTDBObj > C3_PlayerCtrl_T;
class C3_PlayerCtrlConvenient : public C3_PlayerCtrl
{
  public:
    C3_PlayerCtrlConvenient(class RTDBConn& DBC, const char* name = "playerctrl") : C3_PlayerCtrl (DBC, name)
      {
      }
    void Update (void)
      {
        RTDBSearch();
        RTDBRead();
        // These are immediate commands
        subobj_p->goto_ts = 0;
        subobj_p->frame_go = 0;
      }
    void Pause ( bool pause = 1 )
      {
        Update();
        subobj_p->flags.pause = pause;
        RTDBWrite();
      }
    void Speed ( float speed )
      {
        Update();
        subobj_p->speed = speed;
        RTDBWrite();
      }
    void ScanTo ( Timestamp ts, bool pause = 1 )
      {
        Update();
        subobj_p->goto_ts = ts;
        subobj_p->flags.scan = 1;
        subobj_p->flags.pause = pause;
        subobj_p->flags.loop = 1;
        subobj_p->flags.keepcreated = 1;
        RTDBWrite();
      }
    void JumpTo ( Timestamp ts, bool pause = 1 )
      {
        Update();
        subobj_p->goto_ts = ts;
        subobj_p->flags.scan = 0;
        subobj_p->flags.pause = pause;
        subobj_p->flags.loop = 1;
        subobj_p->flags.keepcreated = 1;
        RTDBWrite();
      }
    void FrameGo ( int count, const std::string name = "", bool pause = 1)
      {
        Update();
        if ( name.length() )
          {
            strncpy (subobj_p->frame_objname, name.c_str(), KOGMO_RTDB_OBJMETA_NAME_MAXLEN);
            subobj_p->frame_objname[KOGMO_RTDB_OBJMETA_NAME_MAXLEN-1] = '\0';
          }
        subobj_p->frame_go = count;
        subobj_p->flags.pause = pause;
        RTDBWrite();
      }
    void Loop ( Timestamp start, Timestamp end )
      {
        Update();
        subobj_p->begin_ts = start;
        subobj_p->end_ts = end;
        subobj_p->flags.loop = 1;
        subobj_p->flags.keepcreated = 1;
        RTDBWrite();
      }
    Timestamp currPlayTime (void)
      {
        RTDBConn DBC(getDBC());
        C3_PlayerStat Stat(DBC);
        Stat.RTDBSearch("playerstat");
        Stat.RTDBRead();
        return Stat.subobj_p->current_ts;
      }
};

typedef RTDBObj_T < kogmo_rtdb_subobj_c3_recorderstat_t, KOGMO_RTDB_OBJTYPE_C3_RECORDERSTAT, RTDBObj > C3_RecorderStat_T;
class C3_RecorderStat : public C3_RecorderStat_T
{
  public:
    C3_RecorderStat(class RTDBConn& DBC, const char* name = "") : C3_RecorderStat_T (DBC, name)
      {
      }
    std::string dump (void) const
      {
        std::ostringstream ostr;
        ostr << "* Recorder Status :" << std::endl;
        if ( subobj_p -> begin_ts == 0 || objbase_p -> committed_ts == 0 || subobj_p -> begin_ts == objbase_p -> committed_ts )
          {
            ostr << "(not running)" << std::endl;
          }
        else
          {
            float runTime= kogmo_timestamp_diff_secs ( subobj_p -> begin_ts, objbase_p -> committed_ts );
            char text[100];
            snprintf(text,sizeof(text),
                     "%.3f MB %i / %i Ev in %.2f secs (%.3f MB/s %.3f kEv/s) %i %s",
                    (double)(subobj_p -> bytes_written)/1024/1024,
                    subobj_p -> events_written, subobj_p -> events_total,
                    runTime,
                    (double)(subobj_p -> bytes_written)/1024/1024 / runTime,
                    (double)(subobj_p -> events_written)/1000 / runTime,
                    subobj_p -> events_lost,
                    subobj_p -> events_lost > 0 ? "LOST!" : "lost");
            ostr << text;
          }
        return RTDBObj::dump() + ostr.str();
      };
};

class C3_RecorderStatConvenient : public C3_RecorderStat
{
  kogmo_timestamp_t last_ts;
  uint64_t last_bytes_written;
  uint32_t last_events_written;
  uint32_t last_events_total;
  uint32_t last_events_lost; 
  public:
    C3_RecorderStatConvenient(class RTDBConn& DBC, const char* name = "recorderstat") : C3_RecorderStat (DBC, name)
      {
          last_ts=0;
          last_bytes_written=0;
          last_events_written=0;
          last_events_total=0;
          last_events_lost=0;
      }
    void Update (void)
      {
        try {
          last_ts = objbase_p -> committed_ts;
          last_bytes_written = subobj_p -> bytes_written;
          last_events_written = subobj_p -> events_written;
          last_events_total = subobj_p -> events_total;
          last_events_lost = subobj_p -> events_lost;
          RTDBRead();
        } catch (DBError err) {
          last_ts=0;
          last_bytes_written=0;
          last_events_written=0;
          last_events_total=0;
          last_events_lost=0;
          RTDBSearch();
          RTDBRead();
        }
      }
    float runTime (void) const
      {
        if ( subobj_p -> begin_ts == 0 || objbase_p -> committed_ts == 0)
          return 0;
        return kogmo_timestamp_diff_secs ( subobj_p -> begin_ts, objbase_p -> committed_ts );
      }
    int lagBuf (void) const
      {
        return subobj_p -> event_buflen - subobj_p -> event_buffree;
      }
    float lagTime (void) const
      {
        return kogmo_timestamp_diff_secs ( objbase_p -> data_ts, objbase_p -> committed_ts);
      }

    float deltaTime (void) const
      {
        if ( last_ts == 0 || objbase_p -> committed_ts == 0)
          return 0;
        return kogmo_timestamp_diff_secs ( last_ts, objbase_p -> committed_ts );
      }

    std::string currInfo (void) const
      {
        if ( ! deltaTime() )
          return "(no current info)";
        char text[100];
        snprintf(text,sizeof(text),
                "%.3f MB/s %.1f Ev/s (Lag: %.3f ms %i Ev) %i Ev %s",
                (double)(subobj_p -> bytes_written - last_bytes_written)/1024/1024 / deltaTime(),
                (double)(subobj_p -> events_written - last_events_written) / deltaTime(),
                lagTime() * 1000, lagBuf(), subobj_p -> events_lost - last_events_lost,
                subobj_p -> events_lost - last_events_lost > 0 ? "LOST!" : "lost");
        return text;
      }

    std::string totalInfo (void) const
      {
        if ( ! runTime() )
          return "(no total info)";
        char text[100];
        snprintf(text,sizeof(text),
                 "%.3f MB %i / %i Ev in %.2f secs (%.3f MB/s %.3f kEv/s) %i %s",
                (double)(subobj_p -> bytes_written)/1024/1024,
                subobj_p -> events_written, subobj_p -> events_total,
                runTime(),
                (double)(subobj_p -> bytes_written)/1024/1024 / runTime(),
                (double)(subobj_p -> events_written)/1000 / runTime(),
                subobj_p -> events_lost,
                subobj_p -> events_lost > 0 ? "LOST!" : "lost");
        return text;
      }

    std::string dump (void) const
      {
        return RTDBObj::dump() + totalInfo() + currInfo();
      };
};



/*!
 * \brief Objects that are necessary for the base object (this list must not grow to long!!)
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

class C3_RTDB : public RTDBObj
{
  protected:
    kogmo_rtdb_subobj_c3_rtdb_t *objrtdb_p;
    C3_RTDB ( const C3_RTDB& ); // disable copy-constructor
  public:
    C3_RTDB (class RTDBConn& DBC,
               const char* name = "",
               const int& otype = KOGMO_RTDB_OBJTYPE_C3_RTDB,
               const int32_t& child_size = 0, char** child_dataptr = NULL)
        : RTDBObj (DBC, name, otype, sizeof(kogmo_rtdb_subobj_c3_rtdb_t) +
                   child_size, (char**)&objrtdb_p)
      {
        // Pass a Pointer pointing right after the base data to our child 
        if ( child_size && child_dataptr != NULL )
          *child_dataptr = (char*)objrtdb_p + sizeof (kogmo_rtdb_subobj_c3_rtdb_t);
      };

    Timestamp getStartedTimestamp () const
      {
        return objrtdb_p->started_ts;
      }

    std::string getDBHost () const
      {
        return std::string (objrtdb_p->dbhost);
      }

    uint32_t getVersionRevision () const
      {
        return objrtdb_p->version_rev;
      }

    std::string getVersionDate () const
      {
        return std::string (objrtdb_p->version_date);
      }

    std::string getVersionID () const
      {
        return std::string (objrtdb_p->version_id);
      }

    uint32_t getObjectsMax () const
      {
        return objrtdb_p->objects_max;
      }

    uint32_t getObjectsFree () const
      {
        return objrtdb_p->objects_free;
      }

    uint32_t getProcessesMax () const
      {
        return objrtdb_p->processes_max;
      }

    uint32_t getProcessesFree () const
      {
        return objrtdb_p->processes_free;
      }

    uint32_t getMemoryMax () const
      {
        return objrtdb_p->memory_max;
      }

    uint32_t getMemoryFree () const
      {
        return objrtdb_p->memory_free;
      }

    std::string dump (void) const
      {
        std::ostringstream ostr;
        ostr << "* RTDB Runtime Information:" << std::endl
             << "VersionID: " << getVersionID() << std::endl
             << "Revision of Database: " << getVersionRevision() << " (" << getVersionDate() << ")" << std::endl
             << "Database Host: " << getDBHost() << std::endl
             << "Timestamp of Database Start: " << getStartedTimestamp() << std::endl
             << "Database Runtime: " << (float)((getCommittedTimestamp()-getStartedTimestamp())/60) << " minutes"<< std::endl
             << "Timestamp of this Information: " << getCommittedTimestamp() << std::endl
             << "Last Update of this Information: " << (float)((Timestamp().now()-getCommittedTimestamp())) << " seconds"<< std::endl
             << "Memory free: " << getMemoryFree() << "/" << getMemoryMax() << std::endl
             << "Objects free: " << getObjectsFree() << "/" << getObjectsMax() << std::endl
             << "Processes free: " << getProcessesFree() << "/" << getProcessesMax() << std::endl;
        return RTDBObj::dump() + ostr.str();
      };
};


/* Dieses Objekt ist nur da, um vorhandene Prozesse zu finden, bitte *keine* eigenen Prozesse eintragen! */
class C3_Process : public RTDBObj
{
  protected:
    kogmo_rtdb_subobj_c3_process_t *objprocess_p;
  public:
    C3_Process (class RTDBConn& DBC,
               const char* name = "" )
        : RTDBObj (DBC, name, KOGMO_RTDB_OBJTYPE_C3_PROCESS,
                   sizeof(kogmo_rtdb_subobj_c3_process_t), (char**)&objprocess_p)
      {
        if ( name != NULL && name[0] != '\0' )
          {
            // Sauberer waere folgendes, z.Zt. werden aber in der Wiedergabe die Process-Objekte nicht an ProcList gehaengt
            // RTDBObj ProcList(DBC);
            // ProcList.RTDBSearch("processes",0,0,0,1,KOGMO_RTDB_OBJTYPE_C3_PROCESSLIST);
            // RTDBSearchWait(name,ProcList.getOID());
            RTDBSearchWait(name);
            RTDBReadWaitNext();
          }
      };

    void RTDBWrite (int32_t size = 0, Timestamp ts = 0)
      {
        throw("Please do not manually write Process Objects!");
        int __attribute__ ((unused)) dummy = ts / size; // da __attribute__ ((unused)) in methodenkopf nicht unter GCC 3.2 geht :-(
      }

    kogmo_rtdb_objid_t getProcOID () const
      {
        return objprocess_p->proc_oid;
      }

    int getPID () const
      {
        return objprocess_p->pid;
      }

    int getUID () const
      {
        return objprocess_p->uid;
      }

    int getStatus () const
      {
        return objprocess_p->status;
      }

    const char *getStatusTxt () const
      {
        switch (objprocess_p->status)
          {
            case KOGMO_RTDB_PROCSTATUS_UNKNOWN: return "unknown"; break;
            case KOGMO_RTDB_PROCSTATUS_CYCLEDONE: return "cycle done"; break;
            case KOGMO_RTDB_PROCSTATUS_BUSY: return "busy"; break;
            case KOGMO_RTDB_PROCSTATUS_WAITING: return "waiting"; break;
            case KOGMO_RTDB_PROCSTATUS_FAILURE: return "failure"; break;
          }
        return "? (unknown status id)";
      }

    char *getStatusMsg () const
      {
        return objprocess_p->statusmsg;
      }

    std::string getStatusInfo () const
      {
        return std::string(getStatusTxt()) + " (" + std::string(getStatusMsg()) + ")";
      }

#ifdef HAVE_DBC_SetStatus
    // das liefert genau den darauffolgenden zyklus des prozesses (also nicht den juengsten)
    // -> dies ist damit genau das, was die simulationsleute wollen
    void WaitSuccessiveCycleDone ()
      {
        RTDBConn DBC(db_h);
        std::ostringstream info;
        info << "waiting for process " << getName() << " (" << getProcOID() << ")";
        DBC.SetStatus_Waiting(info.str().c_str());
        do
          {
            try
              {
                RTDBReadWaitSuccessor();
              }
            catch(DBError err)  
              { // History wrap-around abfangen (Fehlermeldung?)
                RTDBRead();
              }
          }
        while ( getStatus() != KOGMO_RTDB_PROCSTATUS_CYCLEDONE );
        DBC.SetStatus_Busy();
      }

    // dies wartet auf den naechsten zyklus, wenn zyklen verpasst wurde, liefert es den letzten
    void WaitCycleDone ()
      {
        WaitSuccessiveCycleDone(); // TODO! waitSuccessiveCycleDone verhaelt sich aehnlich..
      }
#endif

    std::string dump (void) const
      {
        std::ostringstream ostr;
        ostr << "* Process Information:" << std::endl
             << "Process-OID: " << objprocess_p->proc_oid << std::endl
             << "PID: " << objprocess_p->pid << std::endl
             << "TID: " << objprocess_p->tid << std::endl
             << "Status: " << std::string(getStatusTxt()) << " (" << objprocess_p->status << ")" << std::endl
             << "Status Message: " << std::string(objprocess_p->statusmsg) << std::endl
             << "Version: " << objprocess_p->version << std::endl
             << "User-ID: " << objprocess_p->uid << std::endl;
        return RTDBObj::dump() + ostr.str();
      };
};







typedef RTDBObj_T < kogmo_rtdb_subobj_c3_text_t, KOGMO_RTDB_OBJTYPE_C3_TEXT > C3_Text_T;
class C3_Text : public C3_Text_T
{
  public:
    C3_Text(class RTDBConn& DBC, const char* name = "") : C3_Text_T (DBC, name)
      {
        setMinSize( - (int)sizeof(char) * C3_TEXT_MAXCHARS );
        deleteText();
      }

    void deleteText (void)
      {
        setSize( getMinSize() );
      }

    int getLength (void) const
      {
        return getSize()-getMinSize();
      }

    void addText (std::string text)
      {
        if ( getLength() + text.length() >= C3_TEXT_MAXCHARS )
          throw DBError("Text too long, try to increase C3_TEXT_MAXCHARS");
        strncpy (&subobj_p->data[getLength()], text.c_str(), C3_TEXT_MAXCHARS - getLength() );
        setSize( getMinSize() + getLength() + text.length() );
      };

    void setText (std::string text)
      {
        deleteText();
        addText(text);
      };

    std::string getText () const
      {
        std::string text = "";
        if ( getLength() > 0 )
          text.append ( subobj_p->data, getLength() );
        return text;
      };
        
    std::string dump(void) const
    {
        std::ostringstream ostr;
        ostr << "* Text:" << std::endl
             << getText() << std::endl;
        return RTDBObj::dump() + ostr.str();
      }; 
};



typedef RTDBObj_T < kogmo_rtdb_subobj_c3_ints256_t, KOGMO_RTDB_OBJTYPE_C3_INTS > C3_Ints_T;
class C3_Ints : public C3_Ints_T
{
  public:
    C3_Ints(class RTDBConn& DBC, const char* name = "") : C3_Ints_T (DBC, name)
      { 
      }

    int getLength (void) const
      {
        return 256; // todo: dynamic
      }

    void setInt (const int& n, const int& i)
      {
        if ( n >= getLength() )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
        subobj_p->intval[n] = i;
      }

    int getInt (const int& n) const
      {
        if ( n >= getLength() )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
        return subobj_p->intval[n];
      }

    std::string dump (void) const
      {
        std::ostringstream ostr;
        ostr << "* Integer Values:" << std::endl;
        for (int i=0; i < getLength(); i++)
          {
            ostr << i << ": " << getInt(i) << std::endl;
          }
        return RTDBObj::dump() + ostr.str();
      };
};



// for an example: see kogmo_rtdb_obj_a2_classes.hxx: template A2_Image
template < int width = 1024, int height = 768, int channels = 1, int bits=8 > class A2_Image
         : public RTDBObj_T < kogmo_rtdb_subobj_a2_image_t, KOGMO_RTDB_OBJTYPE_A2_IMAGE, RTDBObj, width*height*channels*bits/8 >
{
  private:
    bool use_read_ptr;
    kogmo_rtdb_obj_a2_image_t *read_ptr;
    Timestamp read_ptr_ts;
  public:
    A2_Image(class RTDBConn& DBC, const char* name = "")
            : RTDBObj_T < kogmo_rtdb_subobj_a2_image_t, KOGMO_RTDB_OBJTYPE_A2_IMAGE, RTDBObj, width*height*channels*bits/8 > (DBC, name)
      {
        this->subobj_p->width=width;
        this->subobj_p->height=height;
        this->subobj_p->channels=channels;
        this->subobj_p->colormodell=0;
        this->subobj_p->widthstep=abs(width)*channels*bits/8;

        switch (bits)
         {
          case 16:
            this->subobj_p->depth=A2_IMAGE_DEPTH_16U;
            break;
          case 32:
            this->subobj_p->depth=A2_IMAGE_DEPTH_32F;
            break;
          case 8:
          default:
            this->subobj_p->depth=A2_IMAGE_DEPTH_8U;
            break;
         }
        use_read_ptr = false;
        read_ptr = NULL;
        read_ptr_ts = 0;
      }
    unsigned char * getData(Timestamp committed_ts = 0)
      {
        if ( !use_read_ptr )
          return &this->subobj_p->data[0];
        else
         {  
          if (!committed_ts)
            committed_ts = this->getCommittedTimestamp();
          if ( read_ptr_ts != committed_ts )
           {
            kogmo_rtdb_objsize_t osize;
            osize = kogmo_rtdb_obj_readdata_ptr (this->db_h, this->objinfo_p -> oid, committed_ts,
                                             &read_ptr);
            if ( osize < *(this->objsize_min_p) )
                throw DBError(osize);
            if ( read_ptr->base.committed_ts != (kogmo_timestamp_t) committed_ts )
                throw DBError(-KOGMO_RTDB_ERR_HISTWRAP);
            read_ptr_ts = this->getCommittedTimestamp();
           }
          return read_ptr->image.data;
         }
      }
    bool isDataValid( Timestamp committed_ts = 0 )
      {
        if ( !use_read_ptr )
          return true;
        read_ptr_ts = 0; // force re-read
        try {
          getData(committed_ts);
          return true;
        } catch(DBError err) {
          return false;
        }
      }  
    void setPointerRead(bool yes = true)
      {
        use_read_ptr = yes;
        if ( use_read_ptr )
          this->setMinSize( sizeof(kogmo_rtdb_obj_a2_image_t) );
        else
          this->setMinSize( this->getMaxSize() );
      }
    int getWidth(void)
      {
        return abs(this->subobj_p->width);
      }
    int getHeight(void)
      {
        return abs(this->subobj_p->height);
      }
    int getChannels(void)
      {
        return this->subobj_p->channels;
      }
    int getDepth(void)
      {
        return this->subobj_p->depth;
      }
    int getWidthstep(void)
      {
        return this->subobj_p->widthstep;
      }
    int getColormodel(void)
      {
        return this->subobj_p->colormodell;
      }
    int calcImgSize(void)
      {
        return getWidthstep() * getHeight();
      }
    std::string dump(void) const
    {
        std::ostringstream ostr;
        const char *colormodels[]={"RGB/Grayscale", "YUV411", "YUV422", "YUV444", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "RGB-JPEG"};
        ostr << "* Image:" << std::endl
                << "Size (width x height): " << this->subobj_p->width << " x " << this->subobj_p->height << " Pixel" << std::endl
                << "Width-Step: " << this->subobj_p->widthstep << std::endl
                << "Color Channels [1 for greyscale, 3 for color]: " << this->subobj_p->channels << std::endl
                // print colormodel as text from array and check array bounds
                << "Color Modell: " << ( this->subobj_p->colormodell < (signed)(sizeof(colormodels)/sizeof(char*)) && this->subobj_p->colormodell >= 0 ? colormodels[this->subobj_p->colormodell] : "?" ) << " (" << this->subobj_p->colormodell << ")" << std::endl
                << "Depth: " << this->subobj_p->depth << " Bits/Pixel" <<std::endl;
        return RTDBObj::dump() + ostr.str();
      }; 
};

//old, but generic: typedef RTDBObj_T < kogmo_rtdb_subobj_a2_image_t, KOGMO_RTDB_OBJTYPE_A2_IMAGE, RTDBObj,  640*480 > A2_Image_Gray640x480;
//better:
typedef A2_Image <  640,  480, 1 > A2_Image_Gray640x480;
typedef A2_Image < 1024,  768, 1 > A2_Image_Gray1024x768;

typedef A2_Image <  768,  400, 1 > A2_Image_Gray768x400;
typedef A2_Image <  748,  400, 1 > A2_Image_Gray748x400;

typedef A2_Image <  640,  480, 3 > A2_Image_RGB640x480;
typedef A2_Image < 1024,  768, 3 > A2_Image_RGB1024x768;

typedef A2_Image <  640,  480, 1, 16 > A2_Image_Gray640x480x16;
typedef A2_Image <  640,  480, 3, 16 > A2_Image_RGB640x480x16; 
typedef A2_Image <  640,  480, 1, 32 > A2_Image_Gray640x480x32;
typedef A2_Image <  640,  480, 3, 32 > A2_Image_RGB640x480x32; 


typedef A2_Image_RGB1024x768 A2_BiggestImage; // groesstes zu erwartendes bild (bei bedarf vergroessern)

class A2_AnyImage : public A2_BiggestImage
{
  public:
    A2_AnyImage(class RTDBConn& DBC, const char* name = "") : A2_BiggestImage (DBC, name)
      {
        setMinSize( - calcImgSize() );
      }
};



/*@}*/
}; /* namespace KogniMobil */

#endif /* KOGMO_RTDB_OBJ_SYSTEM_CLASSES_HXX */




