/*! \file kogmo_rtdb_writer2_cxx.cxx
 * \brief Example for writing 2 Objects to the RTDB with the C++ Interface (see kogmo_rtdb_reader2_cxx.C)
 *
 * (c) 2005,2006 Matthias Goebl <mg@tum.de>
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

  RTDBConn DBC("kogmo_rtdb_writer2", 1.000, "");

  // 2 Demo-Objekte (Daten egal)
  C3_Ints DemoObj1(DBC, "demo_object1");
  DemoObj1.RTDBInsert();

  C3_Ints DemoObj2(DBC, "demo_object2");
  DemoObj2.RTDBInsert();

  int i;
  Timestamp TS;
  for ( i=1;/*endlos*/; i++ )
    {
      // Auf naechsten Zyklus warten
      DBC.WaitNextCycle();

      // ...
      TS = DBC.getTimestamp ();
      DemoObj1.setDataTimestamp(TS);
      DemoObj2.setDataTimestamp(TS);

      // Daten schreiben
      DemoObj1.RTDBWrite();
      usleep(200000); // Stoerung
      DemoObj2.RTDBWrite();

      // Zyklus fertig, als Lebenszeichen
      DBC.CycleDone();


      // Und jetzt in anderer Reihenfolge
      DBC.WaitNextCycle();

      TS = DBC.getTimestamp ();
      DemoObj1.setDataTimestamp(TS);
      DemoObj2.setDataTimestamp(TS);

      DemoObj2.RTDBWrite();
      usleep(100000); // Stoerung
      DemoObj1.RTDBWrite();

      DBC.CycleDone();
    }

  return 0;
}
catch(DBError err)  
{   
  cout << "Died on Error: " << err.what() << endl;   
  return 1;
}   
};
