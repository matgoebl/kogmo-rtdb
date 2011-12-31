/*! \file kogmo_rtdb_reader_cxx.cxx
 * \brief Example for reading from the RTDB with the C++ Interface
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

  // Verbindung zur Datenbank aufbauen, unsere Zykluszeit is 33 ms
  RTDBConn DBC("a2_roadtracker_example_cxx", 0.033, "");

  // Auf Object fuer Fahrspur warten
  C3_Ints DemoObj(DBC);
  DemoObj.RTDBSearchWait("demo_object");

  while (1)
    {
      DemoObj.RTDBReadWaitNext();
      cout << DemoObj;
      cout << "Data: " << DemoObj.getInt(0) << " ..." << endl;
    }
}
catch(DBError err)  
{   
  cout << "Died on Error: " << err.what() << endl;   
  return 1;
}   
};
