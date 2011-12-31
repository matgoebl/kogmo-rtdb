/*! \file exampleobj_reader.C
 * \brief Example for reading the ExampleObject with the C++ Interface
 *
 * (c) 2005-2009 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <iostream>
using namespace std;

#define KOGMO_RTDB_DONT_INCLUDE_ALL_OBJECTS
#include "kogmo_rtdb.hxx"
#include "exampleobj_rtdbobj.h"
using namespace KogniMobil;

int
main (int argc, char **argv)
{
 try {

  RTDBConn DBC("exampleobj_reader", 0.033, "");

  ExampleObj_T ExampleObj(DBC);
  ExampleObj.RTDBSearchWait("example_object");

  while (1)
    {
      ExampleObj.RTDBReadWaitNext();
      cout << ExampleObj;
      cout << "Data: " << endl;
      cout << "x = " << ExampleObj.subobj_p->x << endl;
      cout << "y = " << ExampleObj.subobj_p->y << endl;
      cout << "i = " << ExampleObj.subobj_p->i << endl;
      cout << "f = " << ExampleObj.subobj_p->f << endl << endl;
    }
 }
 catch(DBError err)  
 {   
  cout << "Died on Error: " << err.what() << endl;   
  return 1;
 }   
};
