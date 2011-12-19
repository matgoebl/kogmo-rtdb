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
 LDFLAGS_ALL+= -lm -L. -lkogmo_rtdb -lc
 LDFLAGS_LIB=-dynamiclib
 LIB_EXT:=dylib
else
 LDFLAGS_ALL+= -lm -L. -lkogmo_rtdb -lc -lrt
 LDFLAGS_LIB=-shared -lrt
 LIB_EXT:=so.1
 lib_LIBRARIES+=libkogmo_rtdb.so
endif


bin_PROGRAMS+=  kogmo_rtdb_man kogmo_rtdb_dump kogmo_rtdb_objtool
bin_PROGRAMS+=  kogmo_time_convert
lib_LIBRARIES+= libkogmo_rtdb.$(LIB_EXT) libkogmo_rtdb.a
inst_SCRIPTS+=  kogmo_rtdb_kill kogmo_rtdb_objects_config

KOGMO_RTDB_LIB_SOURCES= kogmo_rtdb_obj_local.c kogmo_rtdb_ipc_posix.c kogmo_rtdb_rtmalloc.c kogmo_time.c \
			kogmo_rtdb_objmeta.c kogmo_rtdb_housekeeping.c kogmo_rtdb_helpers.c kogmo_rtdb_utils.c \
			kogmo_rtdb_objdata.c kogmo_rtdb_objdata_slot.c \
			kogmo_rtdb_trace.c kogmo_rtdb_obj_base_funcs.c
CPPFLAGS+= -DRTMALLOC_suba -I../include/ -I.
KOGMO_RTDB_LIB_SOURCES += rtmalloc/suba.c

.PHONY: all install doc clean pkg rev

all: $(lib_LIBRARIES) $(bin_PROGRAMS)
	install -m 0775 -d ../bin/ ../lib/
	install -m 0775 $(bin_PROGRAMS) $(inst_SCRIPTS) ../bin/
	install -m 0664 $(lib_LIBRARIES) ../lib/
	[ -L libkogmo_rtdb.so ] && ln -sf libkogmo_rtdb.so.1 ../lib/libkogmo_rtdb.so || true
	ln -sf kogmo_rtdb_man ../bin/rtdbman
	ln -sf kogmo_rtdb_dump ../bin/rtdbdump
	ln -sf kogmo_rtdb_kill ../bin/rtdbkill

DEST=../installed/kogmo_rtdb/
install: all
	install -m 0775 -d $(DEST)/bin/ $(DEST)/lib/ $(DEST)/include/
	install -m 0775 -s $(bin_PROGRAMS) $(DEST)/bin/
	install -m 0775    $(inst_SCRIPTS) $(DEST)/bin/
	install -m 0664 -s $(lib_LIBRARIES) $(DEST)/lib/
	install -m 0664    ../include/* $(DEST)/include/
	[ -L libkogmo_rtdb.so ] && ln -sf libkogmo_rtdb.so.1 $(DEST)/lib/libkogmo_rtdb.so || true
	ln -sf kogmo_rtdb_man $(DEST)/bin/rtdbman
	ln -sf kogmo_rtdb_dump $(DEST)/bin/rtdbdump
	ln -sf kogmo_rtdb_kill $(DEST)/bin/rtdbkill
	@echo; echo "*** Installation of core kogmo-rtdb was successful."; echo

clean:
	$(RM) *.o */*.o */*/*.o
	$(RM) $(bin_PROGRAMS) $(lib_LIBRARIES) $(patsubst %.so, %.so.1, $(lib_LIBRARIES))

libkogmo_rtdb.$(LIB_EXT):	$(KOGMO_RTDB_LIB_SOURCES)
	@#libtool+automake..
	set -e; for i in $^; do \
	 $(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -c -Wall $$i -o `dirname $$i`/`basename $$i .c`.o; \
	 done
	$(CC) $(LDFLAGS_LIB) \
	 -o $@ $(^:.c=.o) -lc

libkogmo_rtdb.a: libkogmo_rtdb.$(LIB_EXT)
	ar rs libkogmo_rtdb.a $(patsubst %.c, %.o, $(KOGMO_RTDB_LIB_SOURCES))

libkogmo_rtdb.so: libkogmo_rtdb.so.1
	ln -sf $< $@

#libkogmo_rtdb.dylib: libkogmo_rtdb.so.1
#	cp -f $< $@

*.c:	rev
*.h:	rev
KOGMO_RTDB_REV=$(shell svnversion -n . | sed -e 's/^\([0-9]*\).*$$/\1/')
KOGMO_RTDB_DATE=$(shell svn info -R|perl -ne 's/^Last Changed Date: // && print'|sed -e 's/ [^-0-9 +:].*//'|sort -n|tail -n1)
rev:	Makefile
	@grep -q 'KOGMO_RTDB_REV_ORIGIN.*/' kogmo_rtdb_version.h || \
	 (echo "#define KOGMO_RTDB_REV    $(KOGMO_RTDB_REV)";    \
	  echo "#define KOGMO_RTDB_DATE \"$(KOGMO_RTDB_DATE)\""; \
	  echo "#define KOGMO_RTDB_REV_ORIGIN \"$(ORIGIN)\""     )> kogmo_rtdb_version.h
