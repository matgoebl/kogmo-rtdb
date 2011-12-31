CFLAGS+= -O2 -fno-strict-aliasing -Wstrict-prototypes -O2 -g -Wall
LDFLAGS= -lpthread $(LDFLAGS_ALL)
CPPFLAGS+= -D_GNU_SOURCE -D_REENTRANT
# No project specific objects definition within the base kogmo-rtdb:
CPPFLAGS+= -DKOGMO_RTDB_OBJ_TYPEIDS_PROJECT1_FILE=\"kogmo_rtdb.h\" -DKOGMO_RTDB_OBJ_DEFS_PROJECT1_FILE=\"kogmo_rtdb.h\" -DKOGMO_RTDB_OBJ_CLASSES_PROJECT1_FILE=\"kogmo_rtdb.h\"

CPPFLAGS+= -I../rtdb/ -I../include/ -I.
LDFLAGS_ALL += -L../lib/

bin_PROGRAMS= udpsimpleserver udpsimpleclient

all:
	make $(bin_PROGRAMS) libkogmo_rtdb_udpmon.so

.PHONY: clean all install
install:
	install -m 0775 -s $(bin_PROGRAMS) $(KOGMOINSTALLS)/bin/

clean:
	$(RM) *.o */*.o */*/*.o
	$(RM) $(bin_PROGRAMS) libkogmo_rtdb_udpmon.so.1

USEJPEG:=y

ifeq ($(USEJPEG),y)
JPEGOBJS:=jmem_dest.o jmemio.o jmem_src.o
LDFLAGS_ALL+=-ljpeg
CPPFLAGS+=-DUSEJPEG
bin_PROGRAMS+= image_jpeg_compress
else
JPEGOBJS:=
endif

CPPFLAGS+=-DUDPSIMPLEBZIP2
LDFLAGS_ALL+=-lbz2

udpsimpleserver: udpsimpleserver.c udpsimple.h $(JPEGOBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(JPEGOBJS) $(LDFLAGS)

udpsimpleclient: udpsimpleclient.c udpsimple.h $(JPEGOBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(JPEGOBJS) $(LDFLAGS)

image_jpeg_compress: image_jpeg_compress.c $(JPEGOBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(JPEGOBJS) $(LDFLAGS)

libkogmo_rtdb_udpmon.so.1: libkogmo_rtdb_udpmon.c $(JPEGOBJS:.o=.c)
	set -e; for i in $^; do \
	 $(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -c -Wall $$i -o `dirname $$i`/`basename $$i .c`.o; \
	 done
	$(CC) $(SHAREDLIB) -o $@ $(^:.c=.o) -lc -lrt $(LDFLAGS)
libkogmo_rtdb_udpmon.so: libkogmo_rtdb_udpmon.so.1
	ln -sf $< $@
SHAREDLIB=-shared

CPPFLAGS+= -I$(KOGMO_RTDB_HOME)/include/ -I$(KOGMO_OBJECTS)/ -I$(KOGMOHOME)/kogmo_rtdb/include/ -I$(KOGMOHOME)/kogmo_objects/ -I.
LDFLAGS_ALL+= -L$(KOGMO_RTDB_HOME)/lib/ -L$(KOGMOHOME)/kogmo_rtdb/lib -L. -lkogmo_rtdb       

## NON-Realtime Version
CFLAGS+= -O2 -Wall -g
CXXFLAGS+= $(CFLAGS)
LDFLAGS= -lpthread -lrt -lstdc++ $(LDFLAGS_ALL)
#CC:= not gcc4
CPPFLAGS+= -D_GNU_SOURCE -D_REENTRANT
