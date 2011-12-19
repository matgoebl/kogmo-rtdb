bin_PROGRAMS += kogmo_rtdb_typessizecheck kogmo_rtdb_test kogmo_rtdb_histtest kogmo_rtdb_ratetest

export LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):../lib/
export DYLD_LIBRARY_PATH:=$(DYLD_LIBRARY_PATH):../lib/

all:
	make $(bin_PROGRAMS)
	./kogmo_rtdb_typessizecheck
	install -m 0775 $(bin_PROGRAMS) ../bin/

.PHONY: clean all install
install: all
	install -m 0775 -s $(bin_PROGRAMS) $(DEST)/bin/

clean:
	$(RM) *.o */*.o */*/*.o
	$(RM) $(bin_PROGRAMS)

export CPPFLAGS LDFLAGS_ALL
CPPFLAGS+=    -I../include/
LDFLAGS_ALL+= -L../lib/ -lkogmo_rtdb

## NON-Real-time Version
CFLAGS+= -O2 -Wall -g
CXXFLAGS+= $(CFLAGS)
LDFLAGS= -lpthread -lrt $(LDFLAGS_ALL)
CPPFLAGS+= -D_GNU_SOURCE -D_REENTRANT
