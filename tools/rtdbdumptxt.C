/*! \file rtdbdumptxt
 * \brief Dump object data as text
 *
 * (c) 2008 Matthias Goebl <mg@tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <iostream>
using namespace std;

#define _NEED_NEW_ANYDUMP
#include "kogmo_rtdb.hxx"
using namespace KogniMobil;

int
main (int argc, char **argv)
{
try {

  RTDBConn DBC("rtdbdumptxt");

  if ( argc < 2 )
    {
      printf ("Usage: rtdbdumptxt [OBJNAME [REPEATDELAY]]\n\n");
      int i;
      for(i=1;i<1000;i++)
        {
          RTDBObj Obj(DBC, "", 0, 0);
          try {
           Obj.RTDBSearch (NULL, 0, 0, 0, i, 0);
          } catch(DBError err) {
           break;
          }
          if ( Obj.getType() == KOGMO_RTDB_OBJTYPE_C3_PROCESS )
            continue;
          printf(" %s  (#%lli 0x%llX)\n", Obj.getName().c_str(), (long long int)Obj.getOID(), (long long int)Obj.getType());
        }
      exit(0);
    }

  do
    {
      int i;
      for(i=1;i<1000;i++)
        {
          RTDBObj Obj(DBC, "", 0, 2000000);
          Obj.setMinSize();
          try {
           Obj.RTDBSearch (argv[1], 0, 0, 0, i, 0);
          } catch(DBError err) {
           if (i<=1)
             cout << "Error: " << err.what() << endl;
           break;
          }
          Obj.RTDBRead ();
          cout << anydump (Obj);
        }
    if ( argc >= 3 )
      {
        usleep((int)(atof(argv[2])*1000000));
        cout << "\x1b\x5b\x48\x1b\x5b\x32\x4a";
      }
    }
  while ( argc >= 3 );
}
catch(DBError err)  
{   
  cout << "Died on Error: " << err.what() << endl;   
  return 1;
}   
  return 0;
};
