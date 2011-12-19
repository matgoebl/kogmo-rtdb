/*! \file process-cyclic.C
 * \brief Example for a Cyclic Process
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

  RTDBConn DBC("demo_cyclic_process", 1.5, "");

  for (;;)
    {
      DBC.WaitNextCycle();
      cout << DBC.getCycleTimestamp() << " Cyclic Process: Next Cycle" << endl;
      sleep(1);
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
