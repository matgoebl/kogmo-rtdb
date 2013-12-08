SUBDIRS= rtdb/ record/ tools/ tests/ examples/ gui_recorder/ gui_player/ gui_rtdb/ udprmon/
DEST=$(PWD)/installed/kogmo_rtdb/
export DEST

#export KOGMO_RTDB_HOME=$(PWD)/
#export KOGMO_RTDB_OBJECTS=$(KOGMO_RTDB_HOME)/objects/
export CPPFLAGS=

default: all
.PHONY: default veryclean doc

%:
	for i in $(SUBDIRS); do \
	  make -C $$i $@ || exit $?; \
	 done

veryclean:
	-kogmo_rtdb_kill -a
	-make clean
	rm -rf lib bin installed objects/kogmo_rtdb_obj_autogen.*  # rtdb/kogmo_rtdb_version.h
	$(RM) -rf doc/generated

doc:
	mkdir -p doc/generated
	doxygen doc/Doxyfile
	-cd doc/;./mkpdftex all
	@echo "file://$(shell pwd)/doc/generated/html/index.html"    
