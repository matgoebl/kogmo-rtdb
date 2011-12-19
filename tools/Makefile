bin_PROGRAMS += rtdbstat rtdbrecstat rtdbplayctrl rtdbdumptxt rtdbcyclecheck
inst_SCRIPTS += image_extract.pl

all: $(bin_PROGRAMS)
	install -m 0775 $(bin_PROGRAMS) $(inst_SCRIPTS) ../bin/

.PHONY: clean all install
install: all
	install -m 0775 -s $(bin_PROGRAMS) $(DEST)/bin/
	install -m 0775    $(inst_SCRIPTS) $(DEST)/bin/

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
