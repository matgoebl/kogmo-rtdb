/*! \file textreader.C
 * \brief Example for reading variable length ASCII Texts from the RTDB with the C++ Interface
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

  RTDBConn DBC("textreader", 0.1, "");

  C3_Text Text(DBC);
  Text.RTDBSearchWait("testtext");

  for (;;)
    {
      Text.RTDBReadWaitNext();
      cout << "Got Text: " << Text.getText() << endl << "---" << endl;
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
