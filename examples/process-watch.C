/*! \file process-watch.C
 * \brief Watch a Process
 *
 * (c) 2007 Matthias Goebl <mg@tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <iostream>
#include <cmath>
#include <unistd.h>
using namespace std;

#include "kogmo_rtdb.hxx"
using namespace KogniMobil;

int
main (int argc, char **argv)
{
try {

  RTDBConn DBC("demo_watch_process", 1.5, "");
  C3_Process Proc(DBC,"demo_waitother_process");
  Timestamp TS;

  for (;;)
    {
      TS = Proc.getDataTimestamp();
      cout << Proc.getDataTimestamp() << " "
           << Proc.getName() << " (" << Proc.getOID() << "): "
           << Proc.getStatusInfo()
           << " [";

      // Die Zeit, die der Prozess im vorherigen Zustand verbracht hat,
      // erhaelt man erst, wenn der naechste Zustand eintritt, daher:
      Proc.RTDBReadWaitSuccessor();

      cout << ((Proc.getDataTimestamp()-TS)*1000) << " ms]" << endl;
    }

  return 0;
}
catch(DBError err)  
{   
  cout << "Died on Error: " << err.what() << endl;   
  return 1;
}   
};
