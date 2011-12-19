/*! \file qrtdbrecmain.ui.h
 * \brief GUI for comfortable recording
 *
 * (c) 2007 Matthias Goebl <mg@tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

#define _FILE_OFFSET_BITS 64
#define __USE_FILE_OFFSET64
#include <qobject.h>
#include <qprocess.h>
#include <qtimer.h>
#include <qthread.h>
#include <iomanip>
#include <fstream>
#include <time.h>
#include <sys/stat.h>

#include "kogmo_rtdb.hxx"
using namespace KogniMobil;
using namespace std;

RTDBConn *dbc=NULL;
QTimer* updateTimer;
QTime* runtime=NULL;
C3_RecorderStatConvenient *RecStat = NULL;


void
QRTDBGUI::init ()
{
  dbc=new RTDBConn("c3_gui_recorder", 1.0);
  updateTimer = new QTimer(this);
  connect( updateTimer, SIGNAL(timeout()), this, SLOT(update()) );
  cmdargs->setText("kogmo_rtdb_record -a -B -q -0 left_image -1 right_image -2 tele_image -3 camera_flir -4 camera_ffmv_m5 -5 camera_ffmv_m6 -6 internal_bayer_image -7 left_visualization_image -N internal_image -o ");
  if ( getenv("RECORDER_ARGS") )
    cmdargs->setText( getenv("RECORDER_ARGS") );
  newfile();
  RecStat = new C3_RecorderStatConvenient (*dbc);
  update();
}

void
QRTDBGUI::update ()
{
  updateTimer->stop ();

  try {
    RecStat->Update();
    recorderstat->setText( RecStat->currInfo() + "\n" + RecStat->totalInfo() );
  } catch (DBError err) {
    recorderstat->setText( "(not running)" );
  }

  struct stat filestat;
  if ( stat( filename->text().ascii(), &filestat) < 0) {
   status->setText("(neue Datei)");
   pushButtonStart->setEnabled(true);
  } else {
   long long int size = filestat.st_size;
   char str[100];
   snprintf(str,sizeof(str),"Laenge: %.3f GB (%lli Bytes)  %d:%02d.%03d",(double)size/1024/1024/1024, size,
     runtime ? runtime->elapsed()/60000 : 0,
     runtime ? runtime->elapsed()%60000/1000 : 0,
     runtime ? runtime->elapsed()%1000 : 0);
   status->setText(str);
  }

  dbc->CycleDone();
  updateTimer->start ((int) (1000.0 / 2), false);
}

void
QRTDBGUI::commit ()
{
}

QProcess *recorder=NULL;
void QRTDBGUI::start()
{
  //stop();
  //doexec("kogmo_rtdb_record -a -0 camera_left -1 camera_right -2 camera_ffmv -3 camera_flir -o " + filename->text() );

  output->setText("");
  //QString args = "kogmo_rtdb_record -a -B -q -0 left_image -N right_image -N tele_image -N camera_flir -N camera_ffmv_m5 -N camera_ffmv_m6 -N internal_bayer_image -N internal_image -1 left_visualization_image -o " + filename->text();
/*  //QString args = "kogmo_rtdb_record -a -B -q -0 left_image -1 right_image -2 tele_image -3 camera_flir -4 camera_ffmv_m5 -5 camera_ffmv_m6 -6 internal_bayer_image -N internal_image -o " + filename->text();
  */
  QString args = cmdargs->text() + filename->text();
  if ( verbose->isChecked() )
    args += " -l";
  recorder = new QProcess();
  recorder->clearArguments();
  recorder->setCommunication(QProcess::Stdout|QProcess::Stderr|QProcess::DupStderr);
  recorder->setArguments( QStringList::split(" ", args));
  output->append(  "starting: " + recorder->arguments().join(" ") );
  connect( recorder, SIGNAL(readyReadStdout()), this, SLOT(stdout()) );
  connect( recorder, SIGNAL(processExited()), this, SLOT(exited()) );
  if ( !recorder->start() )
    {
      output->append("ERROR: Start failed!");
      delete recorder;
    }
  else
    {
      runtime = new QTime();
      runtime->start();
      pushButtonStart->setEnabled(false);
      verbose->setEnabled(false);
      filename->setEnabled(false);
      pushButtonDelete->setEnabled(false);
      pushButtonNew->setEnabled(false);
      pushButtonStop->setEnabled(true);
    }
  output->append("Started.\n");
}


void QRTDBGUI::stop()
{
  if(recorder)
    {
      updateTimer->stop ();
      commentsave();
      output->append("Sending STOP signal..\n");
      recorder->tryTerminate();
      delete runtime;
      runtime = NULL;
      update();
    }
  //doexec("killall -INT kogmo_rtdb_record");
}

void QRTDBGUI::stdout()
{
  if(recorder)
    output->append( recorder->readStdout() );
}


void QRTDBGUI::exited()
{
  output->append( recorder->readStdout() );
  output->append( "STOPPED." );
  disconnect( recorder, 0,0,0);
  delete recorder;
  recorder=NULL;
  filename->setEnabled(true);
  pushButtonDelete->setEnabled(true);
  pushButtonNew->setEnabled(true);
  pushButtonStop->setEnabled(false);  
  verbose->setEnabled(true);
}


void QRTDBGUI::deletefile()
{
 stop();
 doexec("rm -f '" + filename->text() + "' '" + filename->text() + ".txt'");
}


void QRTDBGUI::commentsave()
{
  ofstream out;
  out.open(filename->text() + ".txt");
  out << comment->text();
  out.close();
}


void QRTDBGUI::newfile()
{
 stop();
 comment->setText("");
 output->setText("");
 // Construct unique date-string
 struct tm timetm;
 time_t secs = time(NULL);
 char date[30];
 localtime_r(&secs, &timetm);
 strftime( date, sizeof (date) -1, "%Y-%m-%d_%H-%M-%S", &timetm );
 //
 QString fn;
 fn = getenv("VIDEOHOME") ? getenv("VIDEOHOME") : "/data";
 fn += "/";
 fn += date;
 fn += ".avi";
 filename->setText(fn);
}

void QRTDBGUI::checkfile()
{
  doexec("xterm -fn 10x20 -geometry 100x40+0+0 -e sh -c 'kogmo_rtdb_play -l -D -S 999999 -i \"" + filename->text() + "\" ;echo \"[END OF FILE - PRESS RETURN!]\";read -t 3 key'");
}

void QRTDBGUI::doexec( QString cmd )
{
  QProcess proc;
  proc.addArgument( "sh");
  proc.addArgument( "-c" ); 
  proc.addArgument( cmd + " &" );
  proc.launch("");
}
