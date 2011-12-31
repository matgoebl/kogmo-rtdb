/*! \file kogmo_rtdb.hxx
 * \brief Object that holds a Connection to the Real-time Vehicle Database
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_CONN_HXX
#define KOGMO_RTDB_CONN_HXX

#include <string.h> // strerror()
#include <iostream>
#include <string>
#include <stdexcept>
#include <stdlib.h> // getenv()

namespace KogniMobil
{

/*! \defgroup kogmo_rtdb_cxx_conn C++ Class for Connection Handling
 * \brief Classes for a Real-time Database in Cognitive Automobiles.
 */
/*@{*/

class DBError : public std::runtime_error
{
  public:
    DBError (const std::string& what = "unknown error") : std::runtime_error (what) {}
    DBError (int err) : std::runtime_error (std::string(strerror(-err))) {}
};

// sorry, war lange auskommentiert in kogmo_rtdb_funcs.h aber schon immer da :-( (mg)
// -> temporaerer fix zur kompatibilitaet (ca. bis 10/2007)
#ifndef HAVE_DEFINED_setstatus
extern "C" {
int
kogmo_rtdb_setstatus (kogmo_rtdb_handle_t *db_h, uint32_t status, _const char* msg, uint32_t flags);
}
#endif
// und das auch noch zur kompatibilitaet, falls jemand nicht alles updated (verhindert aerger)
#define HAVE_DBC_SetStatus

/*! \brief Connection to Real-time Vehicle Database
 * Instantiating this class opens a Connection to the Database.
 * At present there is no handle, because the connection is
 * associated with the process ID.
 */

class RTDBConn
{
  friend class RTDBObj;
  public:
    kogmo_rtdb_handle_t *dbc;
  protected:
    bool connected; // als zaehler fuer copy-construktor
    Timestamp cycle_ts;
    float cycle_period;
    kogmo_rtdb_objid_t proc_id;
  private:
    void operator= ( const RTDBConn& src); // zuweisung sperren
  public:
    //! Open Connection to the Database
    RTDBConn (const char* procname, const float& cycletime = 0.033,
              const char *dbhost = "", const unsigned int& flags = 0)
      {
        kogmo_rtdb_connect_info_t dbinfo;
        int err = kogmo_rtdb_connect_initinfo (&dbinfo, dbhost, procname, cycletime);
        if ( err < 0 )
          throw DBError(err);
        dbinfo.flags = flags;
        dbinfo.version = getenv("KOGMO_REV") ? atoi(getenv("KOGMO_REV")) : 0;
        proc_id = kogmo_rtdb_connect (&dbc, &dbinfo);
        if ( proc_id < 0 )
          throw DBError(proc_id);
        connected = true;
        cycle_period = cycletime;
        cycle_ts.now();
      };
    RTDBConn (kogmo_rtdb_handle_t *db_h)
      {
        dbc = db_h;
        connected = false;
      };
    RTDBConn ( const RTDBConn& src)
      {
        dbc = src.dbc;
        connected = false;
        // achtung: die instanz, die RTDBConn() mit vollem parametersatz aufgerufen hat, schliesst die verbindung!
      };
    //! Close Connection (must be done at process exit)
    ~RTDBConn ()
      {
        if (!connected)
          return;
        int err = kogmo_rtdb_disconnect (dbc, NULL);
        if ( err < 0 )
          throw DBError(err);
      }
    kogmo_rtdb_objid_t getProcID () const // Process-ID, e.g. for Obj.RTDBSearch (...,DBC.getProcID(),...)
      {
        return proc_id;
      }
    kogmo_rtdb_objid_t getOID () const // Process-Object OID, e.g. for Obj.setParent(DBC.getOID())
      {
        kogmo_rtdb_objid_t proclistoid,procoid;
        proclistoid = kogmo_rtdb_obj_searchinfo (dbc, "processes", KOGMO_RTDB_OBJTYPE_C3_PROCESSLIST, 0, 0, 0, NULL, 1);
        if ( proclistoid < 0 )
          throw DBError(proclistoid);
        procoid = kogmo_rtdb_obj_searchinfo (dbc, "", KOGMO_RTDB_OBJTYPE_C3_PROCESS, proclistoid, proc_id, 0, NULL, 1);
        if ( procoid < 0 )
          throw DBError(procoid);
        return procoid;
      }
    void CycleDone (const unsigned int& flags = 0)
      {
        SetStatus(KOGMO_RTDB_PROCSTATUS_CYCLEDONE, "finished work for this cycle", flags);
      }
    void SetStatus_Ok(const char *msg="")
      {
        SetStatus(KOGMO_RTDB_PROCSTATUS_CYCLEDONE, msg);
      }
    void SetStatus_Waiting(const char *msg="")
      {
        SetStatus(KOGMO_RTDB_PROCSTATUS_WAITING, msg);
      }
    void SetStatus_Busy(const char *msg="")
      {
        SetStatus(KOGMO_RTDB_PROCSTATUS_BUSY, msg);
      }
    void SetStatus_Failure(const char *msg="")
      {
        SetStatus(KOGMO_RTDB_PROCSTATUS_FAILURE, msg);
      }
    void SetStatus_Warning(const char *msg="")
      {
        SetStatus(KOGMO_RTDB_PROCSTATUS_WARNING, msg);
      }
private:
    void SetStatus (const unsigned int& status, const char *msg, const unsigned int& flags = 0)
      {
        int err = kogmo_rtdb_setstatus(dbc, status, msg, flags);
        if ( err < 0 )
          throw DBError(err);
      }
public:
    void SleepUntil (Timestamp ts)
      {
        int err = kogmo_rtdb_sleep_until(dbc, ts);
        if ( err < 0 )
          throw DBError(err);
      }
    void Sleep (const double& secs)
      {
        Timestamp ts;
        ts.now();
        ts+=secs;
        int err = kogmo_rtdb_sleep_until(dbc, ts);
        if ( err < 0 )
          throw DBError(err);
      }
    void WaitNextCycle ()
      {
        SetStatus_Waiting("waiting for next cycle");
        cycle_ts+=cycle_period;
        SleepUntil (cycle_ts);
        cycle_ts.now();
        SetStatus_Busy("at work");
      }
    Timestamp getTimestamp ()
      {
        return Timestamp(kogmo_rtdb_timestamp_now(dbc));
      }
    void setTimestamp (Timestamp currTime)
      {
        kogmo_rtdb_timestamp_set(dbc, currTime.timestamp());
      }
    Timestamp getCycleTimestamp () const
      {
        return cycle_ts;
      }
    kogmo_rtdb_handle_t* getHandle (void) const
      {
        return dbc;
      };
};

const int RTDBConnFlag_NoWait = ( KOGMO_RTDB_CONNECT_FLAGS_TRYNOWAIT | KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR );


/*@}*/

}; /* namespace KogniMobil */

#endif /* KOGMO_RTDB_CONN_HXX */
