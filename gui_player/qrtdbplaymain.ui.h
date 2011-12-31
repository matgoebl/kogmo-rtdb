/*! \file qrtdbplaymain.ui.h
 * \brief GUI to control the RTDB Player comfortably
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

#include <qobject.h>
#include <qprocess.h>
#include <qtimer.h>
#include <qthread.h>
#include <iomanip>
#include <qfiledialog.h> 
#include <qapplication.h>
#include <string.h>
#include <qinputdialog.h> 
#include <stdio.h>

#include "kogmo_rtdb.hxx"
using namespace KogniMobil;
using namespace std;

#define SCREEN_UPDATE_FREQ 20
QProcess *player=NULL;

RTDBConn *dbc;
C3_PlayerCtrl *Ctrl = NULL;
C3_PlayerStat *Stat = NULL;
QTimer* updateTimer;

void
QRTDBGUI::init ()
{
  dbc=new RTDBConn("c3_gui_playerctrl", 1.0/SCREEN_UPDATE_FREQ);
  Ctrl = new C3_PlayerCtrl(*dbc);
  Stat = new C3_PlayerStat(*dbc);
  cmdargs->setText( "-T 0xC30003" );
  if ( getenv("PLAYER_ARGS") )
    cmdargs->setText( getenv("PLAYER_ARGS") );

  updateTimer = new QTimer(this);
  connect( updateTimer, SIGNAL(timeout()), this, SLOT(update()) );
  update();

  if ( qApp->argc() >= 2 )
    {
      //system("killall -INT kogmo_rtdb_play 2>/dev/null");
      filename->setText( qApp->argv()[1] );
      pause->setOn(false);
      loop->setChecked(true);
      keepcreated->setChecked(true);
      start();
    }
  else
    {
      QString dir = getenv("VIDEOHOME") ? getenv("VIDEOHOME") : "/data";
      filename->setText( dir + "/" + "noname.avi" );
    }
  if ( getenv("VIDEOHOME") )
    frameobj->setText( getenv("KOGMO_RTDB_PLAYER_FRAMEOBJ") );
  commit();
}

int no_commit=0;
int posupd=1;
int running=0;
Timestamp CurrentTS=0, FirstTS=0, LastTS=0, LastCtrlDataTS=0;

void
QRTDBGUI::update ()
{
  updateTimer->stop ();

  int new_running = 0;
  try {
   Stat->RTDBSearch("playerstat");
   Stat->RTDBRead();
   CurrentTS.settime( Stat->subobj_p->current_ts );
   FirstTS.settime(   Stat->subobj_p->first_ts );
   LastTS.settime(     Stat->subobj_p->last_ts );
   currenttime->setText(CurrentTS.string());
   firsttime  ->setText(FirstTS.string());
   lasttime    ->setText(LastTS.string());
   new_running = 1;
   double pos = CurrentTS - FirstTS;
   double len = LastTS - FirstTS;
   char s[6];
   snprintf(s,6,"%02d:%02d",(int)(len)/60,(int)(len)%60);
   endmin->setText(s);
   snprintf(s,6,"%02d:%02d",(int)(pos)/60,(int)(pos)%60);
   currentmin->setText(s);
   if ( len != 0 && posupd && ! pause->isOn() )
     position->setValue( (int)( pos / len * position->maxValue() ));
  } catch (DBError err) {
  }

  if ( new_running != running )
    {
      running = new_running;        
      pushButtonStart->setEnabled(!running);
      pushButtonBrowse->setEnabled(!running);
      filename->setEnabled(!running);
      cmdargs->setEnabled(!running);
      pushButtonStop->setEnabled(running);
    }
  
// current_ts first_ts last_ts

  no_commit=1;
  try {
   Ctrl->RTDBSearch("playerctrl");
   Ctrl->RTDBRead();
   if ( LastCtrlDataTS != Ctrl->getDataTimestamp() )
     {
       log->setChecked(Ctrl->subobj_p->flags.log);
       pause->setOn(Ctrl->subobj_p->flags.pause);
       scan->setChecked(Ctrl->subobj_p->flags.scan);
       loop->setChecked(Ctrl->subobj_p->flags.loop);
       keepcreated->setChecked(Ctrl->subobj_p->flags.keepcreated);
       speed->changeItem(QString("%1").arg( (int)(Ctrl->subobj_p->speed*100.0) ),0);
       speed->setCurrentItem(0);
       LastCtrlDataTS = Ctrl->getDataTimestamp();
     }
  } catch (DBError err) {
    LastCtrlDataTS = 0;
  }
  no_commit=0;

  if(player)
    output->append( player->readStdout() );

  updateTimer->start ((int) (1000.0 / SCREEN_UPDATE_FREQ), false);

  dbc->CycleDone();
}

void
QRTDBGUI::commit ()
{
  if (no_commit)
    return;

  updateTimer->stop ();

  try {
   Ctrl->RTDBSearch("playerctrl");
   Ctrl->RTDBRead();
   Ctrl->subobj_p->flags.log = log->isChecked() ? 1:0;
   Ctrl->subobj_p->flags.pause = pause->isOn() ? 1:0;
   Ctrl->subobj_p->flags.scan = scan->isChecked() ? 1:0;
   Ctrl->subobj_p->flags.loop = loop->isChecked() ? 1:0;
   Ctrl->subobj_p->flags.keepcreated = keepcreated->isChecked() ? 1:0;
   Ctrl->subobj_p->speed = (float)speed->currentText().toInt()/100.0;
   Ctrl->subobj_p->goto_ts = 0;
   Ctrl->subobj_p->frame_go = 0;
   strncpy(Ctrl->subobj_p->frame_objname,frameobj->text().ascii(),sizeof(Ctrl->subobj_p->frame_objname));

   // In Simulation Mode the Time is stopped when in Pause,
   // so we have to alter the timestamp to notify the player:
   Timestamp LastTS = Ctrl->getDataTimestamp();
   Ctrl->setDataTimestamp();
   if ( LastTS == Ctrl->getDataTimestamp() )
     Ctrl->setDataTimestamp( 1 + LastTS );
   Ctrl->RTDBWrite();
   LastCtrlDataTS = Ctrl->getDataTimestamp();
  } catch (DBError err) {
  }

  update();
}


void QRTDBGUI::browse()
{
 QString file;
 file = QFileDialog::getOpenFileName( filename->text(),
          "RTDB-AVIs (*.avi *.rtdb);;All Files (*)", this,
          "open file dialog", "Choose a Video file to play" );
 if ( !file.isEmpty() )
   filename->setText(file);
}

void QRTDBGUI::renamefile()
{
  bool ok;
  QString oldfile = filename->text();
  QString newfile = QInputDialog::getText(
          "rename file", "Neuer Name:", QLineEdit::Normal,
          oldfile, &ok, this );
  if ( ok && !newfile.isEmpty() ) {
    filename->setText(newfile);
    rename(oldfile.ascii(),newfile.ascii());
    oldfile+=".txt";
    newfile+=".txt";
    rename(oldfile.ascii(),newfile.ascii());
  }
}

void QRTDBGUI::start()
{
  output->setText("");
  QString args;
  args = "kogmo_rtdb_play -i " + filename->text();
  if ( log->isChecked() ) args += " -l";
  if ( pause->isOn() ) args += " -p";
  if ( loop->isChecked() ) args += " -L";
  if ( keepcreated->isChecked() ) args += " -K";
  if ( speed->currentText().toInt() != 100 ) args += QString(" -s %1").arg( (double)speed->currentText().toInt() / 100.0, 0, 'f' );
  if ( cmdargs->text() != "" )  args += " " + cmdargs->text();

  player = new QProcess();
  player->clearArguments();
  player->setCommunication(QProcess::Stdout|QProcess::Stderr|QProcess::DupStderr);
  player->setArguments( QStringList::split(" ", args));
  output->append(  "starting: " + player->arguments().join(" ") );
  connect( player, SIGNAL(readyReadStdout()), this, SLOT(stdout()) );
  connect( player, SIGNAL(processExited()), this, SLOT(exited()) );
  if ( !player->start() )
    {
      output->append(player->readStdout());
      output->append("\nERROR: Start failed!\n");
      delete player;
      player = NULL;
    }
  else
    {
      output->append("Started.\n");
    }
  update();
}


void QRTDBGUI::stop()
{
  if(player)
    {
      output->append("Sending STOP signal..\n");
      player->tryTerminate();
      update();
    }
  //doexec("killall -INT kogmo_rtdb_play");
}

void QRTDBGUI::kill()
{
  QProcess proc;
  proc.addArgument( "sh");
  proc.addArgument( "-c" ); 
  proc.addArgument( "killall -INT kogmo_rtdb_play 2>/dev/null &" );
  proc.launch("");
}

void QRTDBGUI::notes()
{
  QProcess proc;
  QString xeditor = getenv("XEDITOR") ? getenv("XEDITOR") : "nedit";
  proc.addArgument( xeditor );
  proc.addArgument( filename->text() + ".txt" ); 
  proc.launch("");
}

void QRTDBGUI::stdout()
{
  if(player)
    output->append( player->readStdout() );
}


void QRTDBGUI::exited()
{
  output->append( player->readStdout() );
  output->append( "STOPPED." );
  disconnect( player, 0,0,0);
  delete player;
  player = NULL;
  update();
}

void QRTDBGUI::goset()
{
  try {
   Ctrl->RTDBSearch("playerctrl");
   Ctrl->RTDBRead();
   Ctrl->subobj_p->frame_go = 0;
   Ctrl->subobj_p->goto_ts = Timestamp(gototime->text().ascii());
   Ctrl->RTDBWrite();
  } catch (DBError err) {
  }
}


void QRTDBGUI::beginset()
{
  try {
   Ctrl->RTDBSearch("playerctrl");
   Ctrl->RTDBRead();
   Ctrl->subobj_p->goto_ts = 0;
   Ctrl->subobj_p->frame_go = 0;
   Ctrl->subobj_p->begin_ts = Timestamp(begintime->text().ascii());
   Ctrl->RTDBWrite();
  } catch (DBError err) {
  }
}


void QRTDBGUI::endset()
{
  try {
   Ctrl->RTDBSearch("playerctrl");
   Ctrl->RTDBRead();
   Ctrl->subobj_p->goto_ts = 0;
   Ctrl->subobj_p->frame_go = 0;
   Ctrl->subobj_p->end_ts = Timestamp(endtime->text().ascii());
   Ctrl->RTDBWrite();
  } catch (DBError err) {
  }
}


void QRTDBGUI::gocur()
{
  gototime->setText(currenttime->text());
}
void QRTDBGUI::begincur()
{
  begintime->setText(currenttime->text());
  beginset();
}
void QRTDBGUI::endcur()
{
  endtime->setText(currenttime->text());
  endset();
}

void QRTDBGUI::beginclr()
{
  begintime->setText("0");
  beginset();
}

void QRTDBGUI::endclr()
{
  endtime->setText("0");
  endset();
}

void QRTDBGUI::framenextx()
{
  framego( + framex->value() );
}

void QRTDBGUI::framenext()
{
  framego( + 1 );  
}

void QRTDBGUI::frameprevx()
{
  framego( - framex->value() );
}

void QRTDBGUI::frameprev()
{
  framego( - 1 );
}


void QRTDBGUI::quit()
{
  stop();
  close();
}


void QRTDBGUI::framego(int n)
{
  try {
   Ctrl->RTDBSearch("playerctrl");
   Ctrl->RTDBRead();
   Ctrl->subobj_p->goto_ts = 0;
   Ctrl->subobj_p->frame_go = n;
   strncpy(Ctrl->subobj_p->frame_objname,frameobj->text().ascii(),sizeof(Ctrl->subobj_p->frame_objname));
   Ctrl->RTDBWrite();
  } catch (DBError err) {
  }
}


void QRTDBGUI::gopos()
{
   gototime->setText(currenttime->text());
   double len = LastTS-FirstTS;
   Timestamp GoTS = FirstTS;
   GoTS += len * (double) position->value() / position->maxValue ();
   gototime->setText( GoTS.string() );
   goset();
   //framenext();
}


void QRTDBGUI::posupd_on()
{
  posupd=1;
}


void QRTDBGUI::posupd_off()
{
  posupd=0;
}


void QRTDBGUI::gofwd()
{
   Timestamp GoTS = CurrentTS;
   GoTS += 1;
   gototime->setText( GoTS.string() );
   goset();
}


void QRTDBGUI::gorev()
{
   Timestamp GoTS = CurrentTS;
   GoTS -= 1.5;
   gototime->setText( GoTS.string() );
   goset();
}


