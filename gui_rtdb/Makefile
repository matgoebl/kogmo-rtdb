# Eine nette Loesung, um mit den QMakefiles zurechtzukommen - (c) matthias.goebl*goebl.net
TARGET=qrtdbview

all: .Makefile.Qt
	make -f .Makefile.Qt
	install -m 0775 $(TARGET) ../bin/

.Makefile.Qt: $(TARGET).pro
	qmake-qt3 -o .Makefile.Qt

clean:
	-make .Makefile.Qt
	-make -f .Makefile.Qt clean
	-rm -rf .Makefile.Qt .obj .moc .ui $(TARGET)

install: all
	install -m 0775 -s $(TARGET) $(DEST)/bin/

run:
	-killall $(TARGET)
	make all
	./$(TARGET) &

.PHONY: all clean install run
