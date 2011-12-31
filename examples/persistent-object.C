/*! \file persistent-object.C
 * \brief Example for using persistent Objects
 *
 * (c) 2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <iostream>
#include <unistd.h>
using namespace std;

#include "kogmo_rtdb.hxx"
using namespace KogniMobil;

int
main (int argc, char **argv)
{try{

  RTDBConn DBC("persistent-object-demo", 1.0);

  C3_Text Text(DBC);
  ObjID ParentID;

  char what = '?';
  if ( argc >= 2 )
   what = argv[1][0];

  switch (what)
    {
      case 'a':
        Text.setName("persistenttext");
        Text.setText("parent persistent object");
        Text.setPersistent(true);
        Text.setAllowPublicWrite(true);
        Text.RTDBInsert();
        Text.RTDBWrite();
        break;
      case 'd':
        Text.RTDBSearch("persistenttext");
        Text.RTDBDelete();
        break;
      case 'n':
        Text.setName("nonpersistenttext");
        Text.setText("normal non-persistent object");
        Text.setPersistent(false);
        Text.RTDBInsert();
        Text.RTDBWrite();
        sleep(2);
        break;
      case 'A':
        Text.RTDBSearch("persistenttext");
        Text.RTDBRead();
        ParentID=Text.getOID();
        Text.newObject();
        Text.setParent(ParentID);
        Text.setName("childpersistenttext");
        Text.setText("auto-deleted child persistent object");
        Text.setPersistent(true);
        Text.setParentDelete(true);
        Text.setAllowPublicWrite(true);          
        Text.RTDBInsert();
        Text.RTDBWrite();
        break;
      case 'D':
        Text.RTDBSearch("childpersistenttext");
        Text.RTDBDelete();
        break;
      default:
        cout << "unknown operation, see source!\n";
        break;
    }
  return 0;
}catch(DBError err){cout<<"Error: "<<err.what()<<endl;return 1;}};
