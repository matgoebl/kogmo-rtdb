/*! \file kogmo_time.hxx
 * \brief Definition of Time Handling Interface (C++ Classes)
 *
 * Copyright (c) 2005,2006 Matthias Goebl <mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

/*! \defgroup kogmo_time_cc Time Handling Class
 * \brief Class for Time Handling in Cognitive Automobiles.
 */
/*@{*/

#ifndef KOGMO_TIME_HXX
#define KOGMO_TIME_HXX

#include <inttypes.h>
#include <iostream>
#include <string>

#include "kogmo_time.h"

namespace KogniMobil
{


class Timestamp
{
  protected:
    kogmo_timestamp_t ts;
    // TODO: precission and probability measures

  public:
    Timestamp (const kogmo_timestamp_t& t=0)
      {
        ts = t;
      }

    Timestamp (std::string str)
      {
        ts = kogmo_timestamp_from_string (str.c_str());
      }

    Timestamp& now (void)
      {
        ts = kogmo_timestamp_now();
        return *this;
      }

    void settime(const kogmo_timestamp_t& t)
      {
        ts = t;
      }

    static Timestamp getNow (void)
      // please use DBC.getTimestamp(), because it takes the time from the RTDB
      // and works with simulation mode!
      {
        return Timestamp( kogmo_timestamp_now() );
      }

    void settime(const char *str)
      {
        ts = kogmo_timestamp_from_string (str);
      }

    void settime_secs(const unsigned long int& secs, const unsigned long int& nsecs = 0 )
      {
        ts = (kogmo_timestamp_t) secs * KOGMO_TIMESTAMP_TICKSPERSECOND
             + nsecs / KOGMO_TIMESTAMP_NANOSECONDSPERTICK;
      }

    kogmo_timestamp_t timestamp (void) const
      {
        return ts;
      }

    int64_t diffns (const Timestamp& End) const
      {
        return (End.ts-ts) * KOGMO_TIMESTAMP_NANOSECONDSPERTICK;
      }

    double diffs (const Timestamp& End) const
      {
        return (double)(End.ts-ts) / KOGMO_TIMESTAMP_TICKSPERSECOND;
      }

    void addns (const int64_t& ns)
      {
        ts += ns / KOGMO_TIMESTAMP_NANOSECONDSPERTICK;
      }

    void adds (const double& secs)
      {
        ts += (kogmo_timestamp_t) (secs * KOGMO_TIMESTAMP_TICKSPERSECOND);
      }

    Timestamp& operator+= (const double& secs)
      {
        ts += (kogmo_timestamp_t) (secs * KOGMO_TIMESTAMP_TICKSPERSECOND);
        return *this;
      }

    Timestamp& operator-= (const double& secs)
      {
        ts -= (kogmo_timestamp_t) (secs * KOGMO_TIMESTAMP_TICKSPERSECOND);
        return *this;
      }

    // this cast could have saved us all < > <= >= == != operators :-))
    // but not on old gcc versions :-()
    operator long long int ()
      {
        return ts;
      }

    std::string string (void) const
      {
        kogmo_timestamp_string_t tstring;
        kogmo_timestamp_to_string (ts, tstring);
        return std::string(tstring);
      }

};

inline Timestamp
operator+ (const Timestamp& Begin, const double& secs)
{
  return Timestamp ( Begin.timestamp()
               + (kogmo_timestamp_t) (secs * KOGMO_TIMESTAMP_TICKSPERSECOND) );
}

inline Timestamp
operator- (const Timestamp& Begin, const double& secs)
{
  return Timestamp ( Begin.timestamp()
               - (kogmo_timestamp_t) (secs * KOGMO_TIMESTAMP_TICKSPERSECOND) );
}

inline double
operator- (const Timestamp& End, const Timestamp& Begin) // because we write "T2-T1"
{
  return (double) ( End.timestamp() - Begin.timestamp() )
         / KOGMO_TIMESTAMP_TICKSPERSECOND;
}


inline bool     operator<  (const Timestamp& T1, const Timestamp& T2)
{ return T1.timestamp() <  T2.timestamp(); }
inline bool     operator<= (const Timestamp& T1, const Timestamp& T2)
{ return T1.timestamp() <= T2.timestamp(); }
inline bool     operator>  (const Timestamp& T1, const Timestamp& T2)
{ return T1.timestamp() > T2.timestamp(); }
inline bool     operator>= (const Timestamp& T1, const Timestamp& T2)
{ return T1.timestamp() >= T2.timestamp(); }
inline bool     operator== (const Timestamp& T1, const Timestamp& T2)
{ return T1.timestamp() == T2.timestamp(); }
inline bool     operator!= (const Timestamp& T1, const Timestamp& T2)
{ return T1.timestamp() != T2.timestamp(); }


inline std::ostream& operator<< (std::ostream& out, const Timestamp& T)
{
  kogmo_timestamp_string_t tstring;
  kogmo_timestamp_to_string (T.timestamp(), tstring);
  return out << tstring;
}

} /* namespace KogniMobil */
#endif /* KOGMO_TIME_HXX */
/*@}*/
