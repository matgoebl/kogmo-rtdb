/*! \file kogmo_rtdb_writer_cxx.cxx
 * \brief Example for writing to the RTDB with the C++ Interface
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

  // Verbindung zur Datenbank aufbauen, unsere Zykluszeit is 33 ms
  RTDBConn DBC("kogmo_rtdb_writer_cxx", 0.033, "");

  // Object fuer Fahrspur erstellen
  C3_Ints DemoObj(DBC, "demo_object");
  DemoObj.RTDBInsert();

  // Datenobjekt initialisieren
  DemoObj.setInt(0,0);

  int i;
  for ( i=0;/*endlos*/; i++ )
    {
      // Auf naechsten Zyklus warten
      DBC.WaitNextCycle();

      // Von welchem Zeitpunkt sind die Daten?
      DemoObj.setTimestamp(DBC.getCycleTimestamp());
      // Nun die eigentlichen Daten (Achtung: sinnlos! Nur zur Demo!)
      DemoObj.setInt(0,i); // Beispieldaten, die sich aendern...

      // Daten schreiben
      DemoObj.RTDBWrite();

      // Zyklus fertig, als Lebenszeichen
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
