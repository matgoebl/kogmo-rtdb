/*! \file rtdbcyclecheck.C
 * \brief Check the Cycle Time time of any Object
 *
 * (c) 2007 Matthias Goebl <mg@tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <iostream>
#include <cmath>
#include <unistd.h>
#include <stdlib.h> /* exit */
using namespace std;

#include "kogmo_rtdb.hxx"
using namespace KogniMobil;

#define DIE(msg...) do { \
 fprintf(stderr,"%d DIED in %s line %i: ",getpid(),__FILE__,__LINE__); fprintf(stderr,msg); fprintf(stderr,"\n"); exit(1); } while (0)

int
main (int argc, char **argv)
{
try {

  if ( argc < 2 )
    DIE("usage: rtdbcyclecheck OBJECTNAME [c|d]   c=commit-timestamps,d=data-timestamps");

  char tstype='c';
  if ( argc > 2 )
    tstype=argv[2][0];

  RTDBConn DBC("show_cycle", 0.1, "");
  RTDBObj OBJ(DBC);
  OBJ.RTDBSearchWait(argv[1]);
  OBJ.RTDBReadWaitNext();

  Timestamp TS0=0, TS=0;

  float min=0,max=0,sum=0;
  int nr=0;

  for (;;)
    {
      OBJ.RTDBReadWaitSuccessor();

      if ( tstype == 'd' )
        TS=OBJ.getDataTimestamp();
      else
        TS=OBJ.getCommittedTimestamp();

      if ( (long long int)TS0 != 0 )
      {
        float dT = TS - TS0;
        nr++;sum+=dT;
        if ( dT < min || !min ) min = dT;
        if ( dT > max || !max ) max = dT;
        printf("last cycle time: %f  average:%f min:%f max:%f type:%c\n",dT,sum/nr,min,max,tstype);
        if ( TS - TS0 == 1 )
          printf("(committed-time difference is exactly one unit - this might be forged to avoid the same time twice)\n");
      }
      TS0 = TS;
    }

  return 0;

}
catch(DBError err)  
{ cout << "Died on Error: " << err.what() << endl; return 1; }
};
