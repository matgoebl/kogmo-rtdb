#CC=gcc-3.3

CFLAGS+= -O2 -fno-strict-aliasing -Wstrict-prototypes -O2 -g -Wall
LDFLAGS= -lpthread $(LDFLAGS_ALL)
CPPFLAGS+= -D_GNU_SOURCE -D_REENTRANT
# No project specific objects definition within the base kogmo-rtdb:
CPPFLAGS+= -DKOGMO_RTDB_OBJ_TYPEIDS_PROJECT1_FILE=\"kogmo_rtdb.h\" -DKOGMO_RTDB_OBJ_DEFS_PROJECT1_FILE=\"kogmo_rtdb.h\" -DKOGMO_RTDB_OBJ_CLASSES_PROJECT1_FILE=\"kogmo_rtdb.h\"

# Test auf Mac OS-X
ifeq ($(shell uname -s),Darwin)
 CPPFLAGS+= -DMACOSX
 CFLAGS+= -arch i386
 LDFLAGS_ALL+= -lm -L. -L.. -lkogmo_rtdb -lc
 LDFLAGS_LIB=-dynamiclib
 LIB_EXT:=dylib
else
 LDFLAGS_ALL+= -lm -L. -L.. -lkogmo_rtdb -lc -lrt
 LDFLAGS_LIB=-shared -lrt
 LIB_EXT:=so
endif



bin_PROGRAMS=  kogmo_rtdb_record kogmo_rtdb_play kogmo_rtdb_play_nodb
bin_PROGRAMS+= kogmo_rtdb_avi_idx aviriffchunkdump

CPPFLAGS+= -I../rtdb/ -I../include/ -I.
LDFLAGS_ALL += -L../lib/

.PHONY: all install doc clean pkg rev

all: $(bin_PROGRAMS)
	install -m 0775 $(bin_PROGRAMS) ../bin/
	ln -sf kogmo_rtdb_record ../bin/rtdbrec
	ln -sf kogmo_rtdb_play ../bin/rtdbplay

install: all
	install -m 0775 -s $(bin_PROGRAMS) $(DEST)/bin/
	ln -sf kogmo_rtdb_record $(DEST)/bin/rtdbrec
	ln -sf kogmo_rtdb_play $(DEST)/bin/rtdbplay

clean:
	$(RM) *.o
	$(RM) $(bin_PROGRAMS)

kogmo_rtdb_record: kogmo_rtdb_record.o kogmo_rtdb_avirawcodec.o

kogmo_rtdb_play: kogmo_rtdb_play.o kogmo_rtdb_avirawcodec.o kogmo_rtdb_timeidx.o


kogmo_rtdb_play_nodb.o: kogmo_rtdb_play.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -DNODB -c -o $@ $^

kogmo_rtdb_play_nodb: kogmo_rtdb_play_nodb.o kogmo_rtdb_avirawcodec.o kogmo_rtdb_timeidx.o ../lib/libkogmo_rtdb.a
	$(CC) $(TARGET_ARCH) -lc -lrt -L../lib/ -o $@ $^
