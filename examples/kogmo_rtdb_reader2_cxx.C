/*! \file kogmo_rtdb_reader_cxx.cxx
 * \brief Example for reading 2 Objects from the RTDB with the C++ Interface
 *
 * (c) 2005,2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <iostream>
using namespace std;

#include "kogmo_rtdb.hxx"
using namespace KogniMobil;

int
main (int argc, char **argv)
{
try {

  RTDBConn DBC("kogmo_rtdb_reader2", 1.000, "");

  // Auf Objecte initial warten
  C3_Ints DemoObj1(DBC);
  C3_Ints DemoObj2(DBC);
  DemoObj1.RTDBSearchWait("demo_object1");
  DemoObj2.RTDBSearchWait("demo_object2");
  DemoObj1.RTDBReadWaitNext();
  DemoObj2.RTDBReadWaitNext();

  if ( DemoObj1.getAvgCycleTime() != DemoObj2.getAvgCycleTime() )
    {
      printf("Grundsätzlicher Fehler: Objekte muessen die gleiche Zykluszeit haben!\n");
      exit(1);
    }

  const float AllowedCycleDelayPercent  = 0.25; // 25% Zuklusverspaetung erlaubt (zu frueh wird hier nicht geprueft)
  const float AllowedFollowDelayPercent = 0.25; // Updatezeit der Objekte darf nicht mehr als 25% eines Zyklus unterschiedlich sein

  Timestamp lastTS1 = 0, lastTS2 = 0;
  while (1)
    {
      DemoObj1.RTDBReadWaitNext(0, DemoObj1.getAvgCycleTime() * (1.0 + AllowedCycleDelayPercent) );
      DemoObj2.RTDBReadDataTime( DemoObj1.getDataTimestamp() + DemoObj1.getAvgCycleTime() * (1.0 + AllowedFollowDelayPercent ) );
      if ( DemoObj2.getDataTimestamp().diffs(DemoObj1.getDataTimestamp()) > AllowedFollowDelayPercent * DemoObj1.getAvgCycleTime() )
        {
          DemoObj2.RTDBReadWaitNext(0, DemoObj2.getAvgCycleTime() * AllowedFollowDelayPercent );
        }

      cout << endl;
      cout << "Object 1: DataTS=" << DemoObj1.getDataTimestamp() << endl;
      cout << "Object 2: DataTS=" << DemoObj2.getDataTimestamp() << endl;
      cout << "CommitTS Difference: " << (DemoObj1.getCommittedTimestamp().diffs(DemoObj2.getCommittedTimestamp())) << endl;

      if ( fabs (DemoObj1.getDataTimestamp().diffs(DemoObj2.getDataTimestamp())) > AllowedFollowDelayPercent * DemoObj1.getAvgCycleTime() )
        {
          printf("Datenzeiten zu unterschiedlich!\n");
          // error
        }
      else
        {
          // do something
        }
    }
}
catch(DBError err)  
{   
  cout << "Died on Error: " << err.what() << endl;   
  return 1;
}   
};
