/*! \file process-waitother.C
 * \brief Example for a Process that waits for another Process
 *
 * (c) 2007 Matthias Goebl <matthias.goebl*goebl.net>
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

  RTDBConn DBC("demo_waitother_process", 1.5, "");
  C3_Process OtherProcess(DBC,"demo_cyclic_process");

  for (;;)
    {
      //OtherProcess.WaitSuccessiveCycleDone();
      OtherProcess.WaitCycleDone();
      cout << OtherProcess.getDataTimestamp() << " Waiting Process: Next Cycle" << endl;
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
