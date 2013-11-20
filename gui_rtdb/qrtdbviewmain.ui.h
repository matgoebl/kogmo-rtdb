/*! \file qrtdbviewmain.ui.h
 * \brief GUI to inspect the contents of the RTDB
 *
 * (c) 2007 Matthias Goebl <matthias.goebl*goebl.net>
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
#include <qwidget.h>
#include <qapplication.h>
#include <iomanip>
#include <stdio.h>


#define _NEED_NEW_ANYDUMP
#include "kogmo_rtdb.hxx"
using namespace KogniMobil;
using namespace std;

RTDBConn *dbc = NULL;
QTimer* updateTimer;


class RTDBObjTab : public QWidget
{
 private:
  QGridLayout *TabPageLayout;
  QTextBrowser *textBrowserContent;
  kogmo_rtdb_objid_t RTDBObjectID;
  QString RTDBObjectName;
  QTabWidget *myparent;
 public:
  RTDBObjTab ( QTabWidget * parent = 0, const char *name = "", kogmo_rtdb_objid_t oid = 0)
   : QWidget ( parent, "RTDBObjTabPage" )
  {
    TabPageLayout = new QGridLayout( this, 1, 1, 11, 6, "TabPageLayout"); 

    textBrowserContent = new QTextBrowser( this, "textBrowserContent" );
    textBrowserContent->setEnabled( TRUE );
    QFont textBrowserContent_font(  textBrowserContent->font() );
    textBrowserContent_font.setPointSize( 8 );
    textBrowserContent->setFont( textBrowserContent_font ); 

    TabPageLayout->addWidget( textBrowserContent, 0, 0 );
    parent->insertTab( this, QString::fromLatin1(name) + QString("(%1)").arg(oid) );

    RTDBObjectID = oid;
    RTDBObjectName = name;
    myparent=parent;
    update();
  };
  void update(void);
};

void
QRTDBGUI::init ()
{
  dbc=new RTDBConn("qrtdbview", 1.0/2);
  updateTimer = new QTimer(this);
  connect( updateTimer, SIGNAL(timeout()), this, SLOT(update()) );
  if ( qApp->argc() >= 2 )
    {
      int i;
      for(i=1; i < qApp->argc(); i++ )
        {
          QString name = qApp->argv()[i];
          new RTDBObjTab ( tabWidgetObjects, qApp->argv()[i], 0);
          tabWidgetObjects->setCurrentPage(i+1);
        }
    }
  update();
}


static bool do_video = false;
void
QRTDBGUI::toggle_video ()
{
  //do_video = !do_video;
  do_video = checkBoxVideo->isChecked();
  update();
}

static float seconds_past = 0;
void
QRTDBGUI::toggle_history ()
{
  seconds_past = (float)spinBoxPast->value ();
  update();
}

void
RTDBObjTab::update()
{
  Timestamp ts = dbc->getTimestamp();
  ts -= seconds_past;

  try
  {
    RTDBObj Obj(*dbc, "", 0, 2000000);
    Obj.setMinSize();
    Obj.RTDBSearch (RTDBObjectID, ts);
    try
    {
      Obj.RTDBRead (ts);
    }
    catch (DBError err)
    {
    }

    //if ( Obj.getType() == KOGMO_RTDB_OBJTYPE_A2_IMAGE && checkBoxVideo->isChecked())
    if ( Obj.getType() == KOGMO_RTDB_OBJTYPE_A2_IMAGE && do_video )
    {  
      A2_Image < 1024, 768, 3 > Img(*dbc);
      Img.setMinSize(-1024*768*3);
      Img.Copy(Obj);
      unsigned char pgmbuf [ Img.calcImgSize() + 20 ];
      sprintf ( (char*)pgmbuf, "P%i\n%05i %05i \n%03i\n", Img.getChannels()==1 ? 5 : 6,
                Img.getWidth(), Img.getHeight(), 255);

      memcpy ( pgmbuf+20, Img.getData(), Img.calcImgSize() );

      QImage img;
      img.loadFromData ( (const uchar*)pgmbuf, sizeof(pgmbuf));
      img.scale(textBrowserContent->width(), textBrowserContent->height(), QImage::ScaleMin);
      QPixmap px;
      px.convertFromImage( img.scale(textBrowserContent->width(), textBrowserContent->height(), QImage::ScaleFree) );
      textBrowserContent->setText("");
      textBrowserContent->setPaletteBackgroundPixmap(px);
    }
    else
    {
      textBrowserContent->unsetPalette();
      textBrowserContent->setText (anydump (Obj));
    }
  }
  catch (DBError err)
  {
    textBrowserContent->
      setText (QString ("ERROR READING OBJECT CONTENTS:\n") + err.what ());
      try
      {
        RTDBObj Obj(*dbc, "", 0, 2000000);
        Obj.setMinSize();
        Obj.RTDBSearch (RTDBObjectName);
        RTDBObjectID = Obj.getOID();
        myparent->setTabLabel(this, Obj.getName() + QString("(%1)").arg( Obj.getOID() ) );
      }
      catch (DBError err) {}
  }

}


void
QRTDBGUI::update ()
{
  updateTimer->stop ();

  if ( getenv("RTDBVIEW_AUTOCYCLE") )
    {
      static int autocycle = 0;
      autocycle++;
      if ( autocycle > atoi(getenv("RTDBVIEW_AUTOCYCLE")) )
        {
          int curpg = tabWidgetObjects->currentPageIndex();
          curpg++;
          if ( curpg >= tabWidgetObjects->count() )
            curpg=0;
          tabWidgetObjects->setCurrentPage(curpg);
          autocycle=0;
        }
    }

  if ( tabWidgetObjects->currentPageIndex() == 0 )
    update_objlist();
  if ( tabWidgetObjects->currentPageIndex() == 1 )
    update_rtdbinfo();
  if ( tabWidgetObjects->currentPageIndex() > 1 )
    ((RTDBObjTab*)(tabWidgetObjects->currentPage()))->update();

  if (spinBoxUpdateFreq->value () && !checkBoxUpdateStop->isChecked() )
    updateTimer->start ((int) (1000.0 / spinBoxUpdateFreq->value ()), false);

  dbc->CycleDone();
}

void
QRTDBGUI::update_toggle_stop ()
{
  updateTimer->stop ();

  if ( checkBoxUpdateStop->isChecked() )
    return;
  else
    update();
}


void
QRTDBGUI::update_rtdbinfo (void)
{
  try
  {
    C3_RTDB *rtdbinfo;
    rtdbinfo=new C3_RTDB(*dbc);

    if (rtdbinfo->getOID () == 0)
      rtdbinfo->RTDBSearch ("rtdb");

    if (rtdbinfo->getOID () != 0)
      {
	rtdbinfo->RTDBRead ();
/*
        std::ostringstream ostr;
        ostr << setprecision(2) << setw(8) << setiosflags(ios::fixed)
             << "Revision of Database: " << rtdbinfo->getVersionRevision() << " (" << rtdbinfo->getVersionDate() << ")" << std::endl
             << "Database Runtime: " << (float)((rtdbinfo->getCommittedTimestamp()-rtdbinfo->getStartedTimestamp())/60) << " minutes"<< std::endl
             << "Last Manager activity: " << (float)((Timestamp().now()-rtdbinfo->getCommittedTimestamp())) << " seconds ago"<< std::endl
             << "Memory free: " << rtdbinfo->getMemoryFree() << "/" << rtdbinfo->getMemoryMax() << std::endl
             << "Objects free: " << rtdbinfo->getObjectsFree() << "/" << rtdbinfo->getObjectsMax() << std::endl
             << "Processes free: " << rtdbinfo->getProcessesFree() << "/" << rtdbinfo->getProcessesMax() << std::endl;
	textBrowserInfo->setText (ostr.str());
*/
	textBrowserInfo->setText (rtdbinfo->dump());
      }
    else
      {
        textBrowserInfo->setText ("ERROR READING RTDB INFO OBJECT!");
      }
    delete rtdbinfo;
  }
  catch (DBError err)
  {
    textBrowserInfo->setText (QString ("ERROR READING RTDB INFO OBJECT:\n") + err.what ());
  }
}



#define OBJLIST_LEN 9999
kogmo_rtdb_objid_t update_objlist_ids[OBJLIST_LEN] = {0};
unsigned int update_objlist_i=0;
kogmo_rtdb_objid_t update_objlist_selected_oid=0;
bool do_update_content = false;

void
QRTDBGUI::update_objlist (void)
{
  Timestamp ts=dbc->getTimestamp();
  ts-=spinBoxPast->value();

  int last_selection=listBoxObjects->currentItem();

  update_objlist_i=0;
  update_objlist_recursive(ts, 1, 0);

  while ( listBoxObjects->count()>update_objlist_i )
    {
      listBoxObjects->removeItem(listBoxObjects->count()-1);
    }

  listBoxObjects->setCurrentItem(last_selection);
}



void
QRTDBGUI::update_objlist_recursive (long long int ts, int oid, int level)
// oh je! solle eigentlich (kogmo_rtdb_objid_t oid, Timestamp ts) sein - so ein hack!
// wie bringe ich qt meine typen bei?
{
  if (oid > 1)
    try
    {
      RTDBObj Obj (*dbc, "", 0);
      Obj.RTDBSearch (oid, ts);
      std::ostringstream ostr;
      try
      {
	Obj.RTDBRead (ts);
	float age = Timestamp (ts) - Obj.getCommittedTimestamp ();
	if (age > Obj.getMaxCycleTime() * 3)
	  ostr << "!!";
	else if (age > Obj.getAvgCycleTime())
	  ostr << "! ";
	else
	  ostr << "  ";
      }
      catch (DBError err)
      {
	ostr << ". ";
      }

      int i;
      for (i = 1; i < level-1; i++)
	ostr << " |  ";
      if ( level > 1 )
	ostr << " +- ";
      char str[100];
      snprintf (str, sizeof(str), "  0x%X", Obj.getType() );
      ostr << Obj.getName () << " (" << Obj.getOID () << ")" << str;
      //listBoxObjects->insertItem (ostr.str ());
      update_objlist_ids[update_objlist_i] = Obj.getOID ();
      if (listBoxObjects->count () < update_objlist_i+1)
	{
	  listBoxObjects->insertItem (ostr.str ());
	}
      else
	{
	  listBoxObjects->changeItem (ostr.str (), update_objlist_i);
	}
      update_objlist_i++;
      if (update_objlist_i >= OBJLIST_LEN)
        update_objlist_i = OBJLIST_LEN-1;
    }
  catch (DBError err)
  {
  }

  int i;
  try
  {
    RTDBObj Obj (*dbc, "", 0);
    for (i = 1; i < OBJLIST_LEN; i++)
      {
        Obj.newObject();
        Obj.setType(0);
	Obj.RTDBSearch (NULL, oid, 0, ts, i, 0);
	update_objlist_recursive (ts, Obj.getOID (), level + 1);
      }
  }
  catch (DBError err)
  {
    //listBoxObjects->insertItem ("");
  }
}

/*
static std::string getObjName(kogmo_rtdb_objid_t oid)
{
  std::string name = "?";
  RTDBObj Obj (*dbc, "", 0);
  try
    {
      Obj.RTDBSearch (oid);
      name = Obj.getName ();
    }
  catch (DBError err)
    {
    }
  return name;
}
*/

void
QRTDBGUI::add_object (QListBoxItem *it)
{
  if ( ! it->isCurrent() )
    return;
  kogmo_rtdb_objid_t oid = update_objlist_ids[listBoxObjects->currentItem ()];
  std::string name = "?";
  RTDBObj Obj (*dbc, "", 0);
  try
    {
      Obj.RTDBSearch (oid);
      name = Obj.getName ();
    }
  catch (DBError err)
    {
    }

  new RTDBObjTab ( tabWidgetObjects, name.c_str(), oid);
}

void
QRTDBGUI::remove_object ()
{
  if ( tabWidgetObjects->currentPageIndex() <= 1 )
    return;
  tabWidgetObjects->removePage( tabWidgetObjects->currentPage() );
}
