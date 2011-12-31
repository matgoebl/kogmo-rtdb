/*! \file kogmo_rtdb_obj_base_classes.hxx
 * \brief Base-Class that every Object has to inherit from (C++ Interface).
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_OBJ_BASE_CLASSES_HXX
#define KOGMO_RTDB_OBJ_BASE_CLASSES_HXX

#ifndef KOGMO_RTDB_H
	#ifndef KOGMO_RTDB_DONT_INCLUDE_ALL_OBJECTS
	#define KOGMO_RTDB_DONT_INCLUDE_ALL_OBJECTS //automatically don't include all objects, if an object file was included manually
	#endif
#endif

#include "kogmo_rtdb.hxx"

namespace KogniMobil {

/*! \addtogroup kogmo_rtdb_objects_cxx C++ Classes for Data Objects.
 * \brief These Classes wrap the Structs with the Data-Block of every Object
 * and provide Methods to access them. \n
 * The Data-Block ist the Part of the Object that normally
 * changes every cycle by issuing a commit.
 *
 * Every Object must inhertit from DBObj,
 * see kogmo_rtdb_obj_example.hxx for an example
 *
 * The Base-Object also includes Metadata-Handling (Creation/Deletion) and
 * Commit/Select, that is inherited by all other objects.
 */
/*@{*/

typedef kogmo_rtdb_objid_t ObjID;  
typedef kogmo_rtdb_objtype_t ObjType;
typedef kogmo_rtdb_objsize_t ObjSize;


/*! \brief Base-Classes for a Real-time Database Object
 */
class RTDBObj
{
  public: // TODO: protected:
    // use getDBC()/getObjInfo()/getObjData() instead!
    kogmo_rtdb_handle_t *db_h;
    kogmo_rtdb_obj_info_t *objinfo_p;
    kogmo_rtdb_subobj_base_t *objbase_p;
    kogmo_rtdb_objsize_t *objsize_p; 
    kogmo_rtdb_objsize_t *objsize_min_p;
  private:
    void operator= ( const RTDBObj& src); // zuweisung sperren (bitte Copy() benutzen)
    int *reference_counter;
  public:
    // This is the Basis-Constructor. Don't look at it.
    // See A2_RoadKloth for a good example how to use it.
    RTDBObj (const class RTDBConn& DBC,
               const char* name = "",
               const int& otype = 0, // bei der suche "match"t es auf jeden typen
               const int32_t& child_size = 0, char** child_dataptr = NULL)
      {
    		//std::cout << "RTDBObj constructor" << std::endl;
        db_h=DBC.getHandle();
        // Allocate Space for Object Info
        objinfo_p = new kogmo_rtdb_obj_info_t;

        // Allocate Space for Object Data
		    objsize_p = new kogmo_rtdb_objsize_t; 
		    objsize_min_p = new kogmo_rtdb_objsize_t;
        (*objsize_p) = sizeof (kogmo_rtdb_subobj_base_t) + child_size;
        char *ptr = new char [(*objsize_p)];
        objbase_p = (kogmo_rtdb_subobj_base_t*) ptr;
        (*objsize_min_p) = (*objsize_p);

        // Pass a Pointer pointing right after the base data to our child
        if ( child_size && child_dataptr != NULL )
          *child_dataptr = ptr + sizeof (kogmo_rtdb_obj_base_t);

        // Init Object Info
        int err = kogmo_rtdb_obj_initinfo (db_h, objinfo_p, name, otype, (*objsize_p));
        if ( err < 0 )
          throw DBError(err);

        // Init Object Data
        err = kogmo_rtdb_obj_initdata (db_h, objinfo_p, objbase_p);
        if ( err < 0 )
          throw DBError(err);

	    	// Reference Counter for Copy Constructor
	    	reference_counter = new int;
	    	(*reference_counter) = 1;
      };

			//!< Copy Constructor. Makes a shallow copy. Pointers and database-handles are shared with original object
			RTDBObj (const RTDBObj& src)
			{
				//std::cout << "RTDBObj copy constructor" << std::endl;
				db_h = src.db_h;
				objinfo_p = src.objinfo_p;
				objbase_p = src.objbase_p;
		    objsize_p = src.objsize_p; 
		    objsize_min_p = src.objsize_min_p;
				reference_counter = src.reference_counter; 
	    	(*reference_counter)++;
      };

    virtual ~RTDBObj ()
      {
    		//std::cout << "RTDBObj destructor" << std::endl;
	    	(*reference_counter)--;
	    	if ((*reference_counter) == 0) {
    			//std::cout << "-->freeing memory" << std::endl;
	      	delete reference_counter;
			    delete objsize_p; 
			    delete objsize_min_p;
	        delete objbase_p;
	        delete objinfo_p;
	    	}
      };

    // Hier eine "Kopier"-Methode, da der normale Copy-Konstruktor
    // - rekursiv die subobj-pointer aller kind-objekte veraendert (schlecht)
    // - das datenbank-handle mitkopiert
    bool Copy ( const RTDBObj& src, int force = 0)
      {
        bool ok = true;
        if ( objinfo_p->otype && objinfo_p->otype != src.objinfo_p->otype ) // verschiedene typen
          ok = false;
        if ( *(src.objsize_p) < (*objsize_min_p) ) // objekt ist zu klein fuer zielobjekt-mindestgroesse
          ok = false;
        if ( *(src.objsize_min_p) > (*objsize_p) ) // zielobjekt ist zu klein fuer objekt
          ok = false;
        if ( !ok && !force )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);

        (*objsize_p) = *(src.objsize_p) <= (*objsize_p) ? *(src.objsize_p) : (*objsize_p);
        (*objsize_min_p) = *(src.objsize_min_p) <= (*objsize_min_p) ? *(src.objsize_min_p) : (*objsize_min_p); // TODO: check!

        memcpy ( objinfo_p, src.objinfo_p, sizeof(kogmo_rtdb_obj_info_t) );
        if ( db_h != src.db_h )
          objinfo_p -> oid = 0; // in eine andere datenbank darf man nicht mit der gleichen id kopieren (man sollte nicht mehrere verbindungen zur gleichen datenbank haben)
        memcpy ( objbase_p, src.objbase_p, (*objsize_p) > *(src.objsize_p) ? *(src.objsize_p) : (*objsize_p) );
        memset ( objbase_p + (*objsize_p), 0, (*objsize_p) > *(src.objsize_p) ? *(objsize_p)-*(src.objsize_p) : 0 );
        return ok;
      };

    // These methods should be used with caution:

    kogmo_rtdb_handle_t *getDBC() const
      {
	return db_h;
      };

    //RTDBConn getDBConn() const
    //  {
    //    RTDBConn DBC( getDBC() );
    //	return DBC;
    //  };

    kogmo_rtdb_obj_info_t *getObjInfo() const
      {
	return objinfo_p;
      };

    kogmo_rtdb_subobj_base_t *getObjData() const
      {
	return objbase_p;
      };


    // Methods to access the Object-Metadate in the Infoblock
    // Please not: You can only change the data, as long the Object
    // is not yet inserted into the database.
    // You cannot change metadata of object that are already in the database!
    // (objinfo_p->oid is initialized to 0 and set after insert or search)

    void RTDBInsert (void)
      {
        // prevent duplicates:
        if ( objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
        int err = kogmo_rtdb_obj_insert (db_h, objinfo_p);
        if ( err < 0 )
          throw DBError(err);
      }

    void RTDBDelete (void)
      {
        if ( ! objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
        int err = kogmo_rtdb_obj_delete (db_h, objinfo_p);
        if ( err < 0 )
          throw DBError(err);
      }

    void setTimes (float avg_cycletime,   //!< Expected average time between Object-Changes (commits). If unknown set to max_cycletime.
                   float max_cycletime,   //!< Minimum time between Object-Changes (commits).
                   float history_interval //!< Time Interval in Seconds the History of Object-Data should be kept.
                   //!< History-Size(buffer depth) will be history_size = history_interval / max_cycletime) + 1
                  )
      {
        if ( objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
	objinfo_p->avg_cycletime = avg_cycletime;
	objinfo_p->max_cycletime = max_cycletime;
	objinfo_p->history_interval = history_interval;
      }

    float getAvgCycleTime () const
      {
        return objinfo_p->avg_cycletime;
      }

    float getMaxCycleTime () const
      {
        return objinfo_p->max_cycletime;
      }

    float getHistoryInterval () const
      {
        return objinfo_p->history_interval;
      }

    kogmo_rtdb_objid_t getOID () const
      {
	return objinfo_p->oid;
      };

    void newObject ()
      {
	objinfo_p->oid = 0;
      };

    std::string getName () const
      {
	return std::string(objinfo_p->name);
      };

    void setName (std::string name)
      {
        if ( objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
        strncpy (objinfo_p->name, name.c_str(), KOGMO_RTDB_OBJMETA_NAME_MAXLEN);
        objinfo_p->name[KOGMO_RTDB_OBJMETA_NAME_MAXLEN-1] = '\0';
      };

    kogmo_rtdb_objtype_t getType () const
      {
	return objinfo_p->otype;
      };

    void setType (kogmo_rtdb_objtype_t type)
      {
        if ( objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
	objinfo_p->otype = type;
      };

    kogmo_rtdb_objid_t getParent () const
      {
	return objinfo_p->parent_oid;
      };

    void setParent (kogmo_rtdb_objid_t oid)
      {
        if ( objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
	objinfo_p->parent_oid = oid;
      };

    void setAllowPublicRead(bool yes = true)
      {
        if ( objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
	objinfo_p->flags.read_deny = yes ? 0 : 1;
      };

    void setAllowPublicWrite(bool yes = true)
      {
        if ( objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
	objinfo_p->flags.write_allow = yes ? 1 : 0;
      };

    void setForceUnique(bool yes = true)
      {
        if ( objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
	objinfo_p->flags.unique = yes ? 1 : 0;
      };

    void setCycleWatch(bool yes = true)
      {
        if ( objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
	objinfo_p->flags.cycle_watch = yes ? 1 : 0;
      };

    void setPersistent(bool yes = true)
      {
        if ( objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
	objinfo_p->flags.persistent = yes ? 1 : 0;
      };

    void setParentDelete(bool yes = true)
      {
        if ( objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
	objinfo_p->flags.parent_delete = yes ? 1 : 0;
      };

    Timestamp getCreatedTimestamp () const
      {
        return Timestamp(objinfo_p -> created_ts);
      }

    Timestamp getDeletedTimestamp () const
      {
        return Timestamp(objinfo_p -> deleted_ts);
      }

    bool isDeleted () const
      {
        return objinfo_p -> deleted_ts ? true : false;
      }

    kogmo_rtdb_objid_t getDeletededProcOID () const
      {
	return objinfo_p->deleted_proc;
      };

    kogmo_rtdb_objid_t getCreatedProcOID () const
      {
	return objinfo_p->created_proc;
      };

    std::string getDeletededProcInfo () const
      {
        kogmo_rtdb_obj_c3_process_info_t pi;
        kogmo_rtdb_obj_c3_process_getprocessinfo (db_h,
                                                  objinfo_p->deleted_proc,
                                                  objinfo_p->deleted_ts, pi);
        std::string info = std::string (pi);
        return info;
      }

    std::string getCreatedProcInfo () const
      {
        kogmo_rtdb_obj_c3_process_info_t pi;
        kogmo_rtdb_obj_c3_process_getprocessinfo (db_h,
                                                  objinfo_p->created_proc,
                                                  objinfo_p->created_ts, pi);
        std::string info = std::string (pi);
        return info;
      }

     kogmo_rtdb_objsize_t getMaxSize () const
      {
        return objinfo_p -> size_max;
      }

     // NO setMaxSize(), because maxsize is used at object creation for malloc()!
     // maxsize must be set large enough at object construction!

     kogmo_rtdb_objsize_t getMinSize () const
      {
        return (*objsize_min_p);
      }

     void setMinSize (kogmo_rtdb_objsize_t size = 0)
      {
        if ( size == 0 )
          {
            (*objsize_min_p) = sizeof (kogmo_rtdb_subobj_base_t);
            return;
          }
        if ( size < 0 ) // negativ = angabe der "verkuerzung", relativ zu maximalgroesse
          {
            size = (*objsize_p) - (-size);
          }
        if ( size < (kogmo_rtdb_objsize_t)sizeof (kogmo_rtdb_subobj_base_t) )
          {
            throw DBError(-KOGMO_RTDB_ERR_INVALID);
          }
        (*objsize_min_p) = size;
      }


    // Methods that operate on the Object Datablock

    void RTDBWrite (int32_t size = 0, Timestamp ts = 0)
      {
        if ( ! objinfo_p -> oid )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
        if (size)
          objbase_p -> size = size;
        if (ts)
          objbase_p -> data_ts = ts;
	int err;
        do
	 {
          err = kogmo_rtdb_obj_writedata (db_h, objinfo_p -> oid, objbase_p);
          if ( err == -KOGMO_RTDB_ERR_TOOFAST )
            std::cout << "too fast!\n";
         }
        while ( err == -KOGMO_RTDB_ERR_TOOFAST );
        if ( err < 0 )
          throw DBError(err);
      }

    void RTDBRead ( Timestamp ts = 0 ) // TODO: read at different commit/data timestamps
      {
        kogmo_rtdb_objsize_t osize;
        osize = kogmo_rtdb_obj_readdata (db_h, objinfo_p -> oid, ts,
                                         objbase_p, (*objsize_p));
        if ( osize < 0 )
          {
            objbase_p -> size = 0;
            throw DBError(osize);
          }
        if ( osize < (*objsize_min_p) )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
      };

    void RTDBReadDataTime ( Timestamp ts = 0 )
      {
        kogmo_rtdb_objsize_t osize;
        osize = kogmo_rtdb_obj_readdata_datatime (db_h, objinfo_p -> oid, ts,
                                         objbase_p, (*objsize_p));
        if ( osize < 0 )
          {
            objbase_p -> size = 0;
            throw DBError(osize);
          }
        if ( osize < (*objsize_min_p) )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
      };

    void RTDBReadWaitNext ( Timestamp old_ts = 0, float timeout = 0 )
      // hier: relativer Timeout, z.B. timeout = 0.01 -> blockiere max 10ms (ist einfacher)
      // die *_until() Funktionen nehmen einen *absoluten* Timeout, wichtig fuer Realzeit
      {
        Timestamp wakeup_ts = 0;
        if ( timeout )
          {
            wakeup_ts.now(); // Achtung: diese Timeout-Geschichte funktioniert noch nicht mit
                      // den Zeitstempeln von kogmo_rtdb_timestamp_now()
                      // da die in der Simulation auch in der Vergangenheit
                      // liegen koennen (TODO-mg)
            wakeup_ts+=timeout;
          }
        kogmo_rtdb_objsize_t osize;
        if ( ! old_ts )
          old_ts = objbase_p -> committed_ts;
        osize = kogmo_rtdb_obj_readdata_waitnext_until (db_h, objinfo_p -> oid,
                                                        old_ts, objbase_p, (*objsize_p), wakeup_ts);
        if ( osize < 0 )
          throw DBError(osize);
        if ( osize < (*objsize_min_p) )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
      };

    void RTDBReadDataYounger ( Timestamp old_ts = 0 )
      {
        kogmo_rtdb_objsize_t osize;
        if ( ! old_ts )
          old_ts = objbase_p -> data_ts;
        osize = kogmo_rtdb_obj_readdata_datayounger (db_h, objinfo_p -> oid, old_ts,
                                         objbase_p, (*objsize_p));
        if ( osize < 0 )
          {
            objbase_p -> size = 0;
            throw DBError(osize);
          }
        if ( osize < (*objsize_min_p) )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
      };

    void RTDBReadYounger ( Timestamp old_ts = 0 )
      {
        kogmo_rtdb_objsize_t osize;
        if ( ! old_ts )
          old_ts = objbase_p -> data_ts;
        osize = kogmo_rtdb_obj_readdata_younger (db_h, objinfo_p -> oid, old_ts,
                                         objbase_p, (*objsize_p));
        if ( osize < 0 )
          {
            objbase_p -> size = 0;
            throw DBError(osize);
          }
        if ( osize < (*objsize_min_p) )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
      };

    void RTDBReadDataOlder ( Timestamp old_ts = 0 )
      {
        kogmo_rtdb_objsize_t osize;
        if ( ! old_ts )
          old_ts = objbase_p -> data_ts;
        osize = kogmo_rtdb_obj_readdata_dataolder (db_h, objinfo_p -> oid, old_ts,
                                         objbase_p, (*objsize_p));
        if ( osize < 0 )
          {
            objbase_p -> size = 0;
            throw DBError(osize);
          }
        if ( osize < (*objsize_min_p) )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
      };

    void RTDBReadOlder ( Timestamp old_ts = 0 )
      {
        kogmo_rtdb_objsize_t osize;
        if ( ! old_ts )
          old_ts = objbase_p -> data_ts;
        osize = kogmo_rtdb_obj_readdata_older (db_h, objinfo_p -> oid, old_ts,
                                         objbase_p, (*objsize_p));
        if ( osize < 0 )
          {
            objbase_p -> size = 0;
            throw DBError(osize);
          }
        if ( osize < (*objsize_min_p) )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
      };

    // RTDBReadWaitSuccessor() liefert genau den naechsten Puffer
    // und nicht nur den aktuellsten wie RTDBReadWaitNext()
    // (dies sollte nur in der Simulation verwendet werden,
    // denn wenn man dauerhaft zu langsam ist, verliert man dennoch
    // Puffer.)
    void RTDBReadWaitSuccessor ( Timestamp old_ts = 0 )
      {
        if ( ! old_ts )
          old_ts = objbase_p -> committed_ts;
        // warten, bis neuere daten verfuegbar sind
        kogmo_rtdb_obj_base_t tmp_obj; // wir wollen nur den header der neueren daten
        kogmo_rtdb_objsize_t osize;
        osize = kogmo_rtdb_obj_readdata_waitnext (db_h, objinfo_p -> oid,
                                                  old_ts, &tmp_obj, sizeof(tmp_obj));
        if ( osize < 0 )
          throw DBError(osize);
        // jetzt die daten lesen, die genau einen schritt juenger sind, als die alten
        RTDBReadYounger ( old_ts );
        // jetzt noch schauen, ob es die vorhergehenden noch gibt (sonst
        // koennen mehr schritte dazwischen sein!)
        if ( ! isValid ( old_ts ) )
          throw DBError("History wrap-around, you might be too slow");
          //throw DBError(-KOGMO_RTDB_ERR_HISTWRAP); // sonst History wrap-around signalisieren
      };

    void RTDBReadPredecessor ( Timestamp old_ts = 0 )
      {
        kogmo_rtdb_objsize_t osize;
        if ( ! old_ts )
          old_ts = objbase_p -> committed_ts;
        osize = kogmo_rtdb_obj_readdata_older (db_h, objinfo_p -> oid, old_ts,
                                               objbase_p, (*objsize_p));
        if ( osize < 0 )
          {
            objbase_p -> size = 0;
            if ( osize == -KOGMO_RTDB_ERR_NOTFOUND )
              throw DBError("History wrap-around, you might be too slow");
              // throw DBError("SimClient: Requested Cycle aged out of History, you might be too slow");
            throw DBError(osize);
          }
        if ( osize < (*objsize_min_p) )
          throw DBError(-KOGMO_RTDB_ERR_INVALID);
      };

    bool isValid ( Timestamp ts = 0 )
      {
        if ( ! ts )
          ts = objbase_p -> committed_ts;
        // warten, bis neuere daten verfuegbar sind
        kogmo_rtdb_obj_base_t tmp_obj; // wir wollen nur den header der daten
        kogmo_rtdb_objsize_t osize;
        osize = kogmo_rtdb_obj_readdata (db_h, objinfo_p -> oid,
                                         ts, &tmp_obj, sizeof(tmp_obj));
        if ( osize < 0 )
          return false;
        return true;
      }

    Timestamp getTimestamp () const // better use getDataTimestamp
      {
        return getDataTimestamp();
      }

    void setTimestamp (Timestamp ts = 0) // better use setDataTimestamp
      {
        setDataTimestamp (ts);
      }

    Timestamp getDataTimestamp () const
      {
        return objbase_p -> data_ts;
      }

    void setDataTimestamp (Timestamp ts = 0)
      {
        if (!ts)
          ts = kogmo_rtdb_timestamp_now(db_h);
        objbase_p -> data_ts = ts;
      }

    Timestamp getCommittedTimestamp () const
      {
        return objbase_p -> committed_ts;
      }

     kogmo_rtdb_objid_t getCommittedProcOID () const
      {
        return objbase_p -> committed_proc;
      }

     std::string getCommittedProcInfo () const
      {
        kogmo_rtdb_obj_c3_process_info_t pi;
        kogmo_rtdb_obj_c3_process_getprocessinfo (db_h,
                                                  objbase_p->committed_proc,
                                                  objbase_p->committed_ts, pi);
        std::string info = std::string (pi);
        return info;
      }

     kogmo_rtdb_objsize_t getSize () const
      {
        return objbase_p -> size;
      }

     void setSize (int size = 0)
      {
        if ( size < 0 ) // negativ = angabe der "verkuerzung", relativ zu maximalgroesse
          {
            size = (*objsize_p) - (-size);
          }
        if ( size < (kogmo_rtdb_objsize_t)sizeof (kogmo_rtdb_subobj_base_t) )
          {
            throw DBError(-KOGMO_RTDB_ERR_INVALID);
          }
        objbase_p -> size = size;
      }







    // Methods that operate on the Metadata *AND* the Data

    void RTDBSearch (kogmo_rtdb_objid_t oid, Timestamp ts = 0)
      {
        kogmo_rtdb_objtype_t saved_otype = objinfo_p->otype;
        int err = kogmo_rtdb_obj_readinfo ( db_h, oid, ts, objinfo_p );
        if ( err < 0 )
          throw DBError(err);
        if ( saved_otype && objinfo_p->otype != saved_otype )
          { // verschiedene typen?
            newObject();
            objinfo_p->otype = saved_otype;
            throw DBError(-KOGMO_RTDB_ERR_INVALID);
          }
        // Init Object Data (clear data block!)
        if ( objinfo_p->size_max > (*objsize_p) ) objinfo_p->size_max = (*objsize_p);
        err = kogmo_rtdb_obj_initdata (db_h, objinfo_p, objbase_p);
        if ( err < 0 )
          throw DBError(err);
      };

    void RTDBSearch (const char* name = "", kogmo_rtdb_objid_t parent_oid = 0,
                     kogmo_rtdb_objid_t proc_oid = 0, Timestamp ts = 0,
                     int nth = 1, kogmo_rtdb_objtype_t type = -1)
      {
        kogmo_rtdb_objid_t oid;
        if (type<0) type = objinfo_p -> otype;
        oid = kogmo_rtdb_obj_searchinfo ( db_h, name, type,
                    parent_oid, proc_oid, ts, NULL, nth);
        if ( oid < 0 )
          throw DBError(oid);
        RTDBSearch (oid, ts);
      };

    void RTDBSearchWait (const char* name = "", kogmo_rtdb_objid_t parent_oid = 0,
                         kogmo_rtdb_objid_t proc_oid = 0, float timeout = 0 )
      // siehe die Anmerkungen bei RTDBReadWaitNext()!
      {
        Timestamp wakeup_ts = 0;
        if ( timeout )
          {
            wakeup_ts.now();
            wakeup_ts+=timeout;
          }
        kogmo_rtdb_objid_t oid;
        oid = kogmo_rtdb_obj_searchinfo_wait_until ( db_h, name, objinfo_p -> otype,
                    parent_oid, proc_oid, wakeup_ts);
        if ( oid < 0 )
          throw DBError(oid);
        RTDBSearch (oid);
      };

    virtual std::string dump (void) const
      {
        char *p;
        p = kogmo_rtdb_obj_dumpinfo_str (db_h, objinfo_p);
        std::string info = std::string(p);
        free(p);
        p = kogmo_rtdb_obj_dumpbase_str (db_h, objbase_p);
        std::string data = std::string(p);
        free(p);
        return info+data;
      }

// This is a hook for your extentions, if you do not want to extend it by inheritance
#ifdef   KOGMO_RTDB_OBJ_BASE_CLASS_EXTRA_METHODS_FILE
#include KOGMO_RTDB_OBJ_BASE_CLASS_EXTRA_METHODS_FILE
#endif

};


inline std::ostream& operator<< (std::ostream& out, const RTDBObj& O)
{
  return out << std::endl << O.dump() << std::endl;
};


/*
  Empfohlene Benutzung des Templates:

  typedef RTDBObj_T < kogmo_rtdb_subobj_c3_camstate_t, KOGMO_RTDB_OBJTYPE_C3_CAMSTATE_V1, RTDBObj > C3_CamState_T;
  class C3_CamState : public C3_CamState_T
  {
    public:
      C3_CamState(class RTDBConn& DBC, const char* name = "") : C3_CamState_T (DBC, name)
      {
      }
  };
*/

template < typename KOGMO_SUBSTRUCT, int KOGMO_TYPE_ID,
           class KOGMO_CLASS = RTDBObj, int size_add = 0 > class RTDBObj_T
         : public KOGMO_CLASS
{
  public:
    KOGMO_SUBSTRUCT *subobj_p;
  public:
    RTDBObj_T (class RTDBConn& DBC,
               const char* name = "",
               const int& otype = KOGMO_TYPE_ID,
               const int32_t& child_size = size_add)
        : KOGMO_CLASS (DBC, name, otype, sizeof(KOGMO_SUBSTRUCT) +
                       child_size, (char**)&subobj_p)
      {
      };
};


/*@}*/
}; /* namespace KogniMobil */

#endif /* KOGMO_RTDB_OBJ_BASE_CLASSES_HXX */
