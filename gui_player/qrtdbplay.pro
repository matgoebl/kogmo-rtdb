TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release

LIBS	+= -lrt -lkogmo_rtdb -L$(KOGMO_RTDB_HOME)/lib/ -L../lib/

INCLUDEPATH	+= $(KOGMO_RTDB_HOME)/include/ ../include/

SOURCES	+= qrtdbplay.cpp

FORMS	= qrtdbplaymain.ui

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}


