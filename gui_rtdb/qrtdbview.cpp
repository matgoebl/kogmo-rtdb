#include <qapplication.h>

#include "qrtdbviewmain.h"

int main( int argc, char ** argv )
{
    QApplication a( argc, argv );
    QRTDBGUI w;
    a.setMainWidget(&w);
    w.show();
    a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
    return a.exec();
}
