# bin_PROGRAMS= kogmo_rtdb_writer kogmo_rtdb_reader kogmo_rtdb_writer_cxx kogmo_rtdb_reader_cxx kogmo_rtdb_watcher kogmo_rtdb_lister
#-bin_PROGRAMS+=kogmo_rtdb_udpsender kogmo_rtdb_udpreceiver textwriter textreader simtextwriter process-cyclic process-waitother process-watch

bin_PROGRAMS += kogmo_rtdb_writer kogmo_rtdb_reader kogmo_rtdb_writer_cxx kogmo_rtdb_reader_cxx kogmo_rtdb_watcher kogmo_rtdb_lister kogmo_rtdb_slotreader
bin_PROGRAMS += kogmo_rtdb_udpsender kogmo_rtdb_udpreceiver process-cyclic process-waitother process-watch
bin_PROGRAMS += textreader textwriter textwriter_cxx textreader_cxx
bin_PROGRAMS += persistent-object textreader_multithread textreader_multithread_nonworking
bin_PROGRAMS += kogmo_rtdb_reader_timeoutblocking
bin_PROGRAMS += kogmo_rtdb_writer2_cxx kogmo_rtdb_reader2_cxx
bin_PROGRAMS += exampleobj_writer exampleobj_reader

OBJDEFSTOOL= KOGMO_RTDB_HOME=../ KOGMO_RTDB_OBJECTS=../objects/ ../bin/kogmo_rtdb_objects_config

all: objdefs $(bin_PROGRAMS)
	install -m 0775 $(bin_PROGRAMS) ../bin/

objdefs:
	$(OBJDEFSTOOL) -u

.PHONY: clean all install objdefs
install: all
	install -m 0775 -s $(bin_PROGRAMS) $(DEST)/bin/

clean:
	$(RM) *.o */*.o */*/*.o
	$(RM) $(bin_PROGRAMS)

CPPFLAGS+=    $(shell $(OBJDEFSTOOL) --cflags)
LDFLAGS_ALL+= $(shell $(OBJDEFSTOOL) --libs)

#CPPFLAGS+=    -I$(KOGMO_RTDB_HOME)/include/ -I$(KOGMO_RTDB_OBJECTS)/ -I. -I../include/
#LDFLAGS_ALL+= -L$(KOGMO_RTDB_HOME)/lib/ -L../lib/ -L. -lkogmo_rtdb -lm
#
## For Compatibility to existing KogniMobil-Project Code
#CPPFLAGS+=    -DKOGMO_RTDB_OBJ_TYPEIDS_PROJECT1_FILE=\"kogmo_rtdb.h\" -DKOGMO_RTDB_OBJ_DEFS_PROJECT1_FILE=\"kogmo_rtdb.h\" -DKOGMO_RTDB_OBJ_CLASSES_PROJECT1_FILE=\"kogmo_rtdb.h\"
#CPPFLAGS+=    -I$(KOGMO_OBJECTS)/ -I$(KOGMOHOME)/kogmo_rtdb/include/ -I$(KOGMOHOME)/kogmo_objects/ -I$(KOGMOINSTALLS)/include/
#LDFLAGS_ALL+= -L$(KOGMOHOME)/kogmo_rtdb/lib/ -L$(KOGMOINSTALLS)/lib/

## NON-Real-time Version
CFLAGS+= -O2 -fno-strict-aliasing -Wall -g
CXXFLAGS+= $(CFLAGS)
LDFLAGS= -lpthread -lrt -lstdc++ $(LDFLAGS_ALL)
#CC:= not gcc4
CPPFLAGS+= -D_GNU_SOURCE -D_REENTRANT
