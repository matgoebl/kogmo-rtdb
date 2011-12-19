/*! \file rtdbplayctrl.C
 * \brief Commandline Tool to control the KogMo-RTDB-Player
 *
 * (c) 2009 Matthias Goebl <mg@tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <iostream>
#include <unistd.h>
#include <getopt.h>
using namespace std;

#include "kogmo_rtdb.hxx"
using namespace KogniMobil;

void
usage (void)
{
  fprintf(stderr,
"Usage: rtdbplayctrl\n"
" -j DATETIME   jump to position (without playing)\n"
" -g DATETIME   go to position ('scanning'=with playing at maximum speed)\n"
" -F NAME       set object name for by-single-frame-control\n"
" -f +N/-N      play next N updates of object given by -F / rewind N updates\n"
"               (rewind uses a history that gets reset every time -F is changed)\n"
" -S ST -E ET   loop between start time ST and end time ET (both must be given)\n"
" -P            continue playing after jump/go (default: pause)\n"
" -s SPEED      set relative speed SPEED (1.0 = normal, 0.5 = half speed)\n"
" -h            Print this help message\n\n");
  exit(1);
}


int
main (int argc, char **argv)
{
try {

  Timestamp go_to=0, jump_to=0, loop_start=0, loop_end=0;
  char *frame_name=NULL;
  int frame_go=0, pause=1, set_loop=0;
  float speed=-1;
  int opt;

  // Verbindung zur Datenbank aufbauen
  RTDBConn DBC("rtdbplayctrl", 1.0);

  C3_PlayerCtrlConvenient PCtrl(DBC);
  try {
   PCtrl.Update();
  } catch (DBError err) {
   cout << "Cannot Read Player Control Object - Is the KogMo-RTDB-Player running? Error: " << err.what() << endl;
   return 1;
  }

  while( ( opt = getopt (argc, argv, "g:j:F:f:S:E:Ps:h") ) != -1 )
    switch(opt)
      {
        case 'g': go_to.settime(optarg); break;
        case 'j': jump_to.settime(optarg); break;
        case 'F': frame_name  = optarg; break;
        case 'f': frame_go = atoi(optarg); break;
        case 'P': pause = 0; break;  
        case 'S': loop_start.settime(optarg); set_loop=1; break;
        case 'E': loop_end.settime(optarg); break;
        case 's': speed = atof(optarg); break;
        case 'h':
        default: usage(); break;
      }

  if ( set_loop )
    PCtrl.Loop ( loop_start, loop_end);
  if (speed >= 0)
    PCtrl.Speed (speed);
  if (go_to)
    PCtrl.ScanTo (go_to, pause);
  if (jump_to)
    PCtrl.JumpTo (jump_to, pause);
  if ( frame_go || frame_name )
    PCtrl.FrameGo (frame_go, frame_name, pause);
  if ( !go_to && !jump_to && !frame_go )
    PCtrl.Pause ( pause );

  Timestamp ts = PCtrl.currPlayTime();
  cout << ts << endl;

  return 0;
}
catch(DBError err)  
{   
  cout << "Died on Error: " << err.what() << endl;   
  return 1;
}   
};
