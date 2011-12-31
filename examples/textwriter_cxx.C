/*! \file textwriter.C
 * \brief Example for writing variable length ASCII Texts to the RTDB with the C++ Interface
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

  RTDBConn DBC("textwriter", 0.1, "");

  C3_Text Text(DBC, "testtext");
  Text.RTDBInsert();

  for (;;)
    {
      std::string input;
      getline(cin,input);
      Text.setText(input);
      Text.RTDBWrite();
      DBC.CycleDone();
      cout << "---" <<endl;
      //sleep(1);
    }

  return 0;
}
catch(DBError err)  
{   
  cout << "Died on Error: " << err.what() << endl;   
  return 1;
}   
};
