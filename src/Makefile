CC         = gcc
CBASEFLAGS = -I. -w -rdynamic -DWITH_STBI
DEBUGFLAGS = -ggdb3 -O0
TERMFLAGS1 = -DPRINT_OTHERTTY -DPRINT_OTHERTTY_2 #-ggdb3 -Og #-DEBUG #-DSHOW_ATOMS  -DV_DEBUG  -DV_DEBUG_POSITION #-DEBUG2
TERMFLAGS2 = -DBOXDBG
CFLAGS 	   = $(CBASEFLAGS) -Wall -pedantic $(DEBUGFLAGS) #-finstrument-functions
IMGCFLAGS  = -I/usr/local/include -lm -pthread
DRAWCFLAGS = -I/usr/local/include -lX11 -lm -pthread
IMGOBJS    = parcours.o positions.o scr.o deb.o array.o copy.o
CFILES     = parcours.c positions.c scr.c deb.c array.c copy.c
MAINOBJECT = main.o
IMGFOLDER  = ./image
PROPSOBJS  = $(IMGFOLDER)/props.o
IMGCFILES  = $(IMGFOLDER)/props.c
DRAWOBJECT = $(IMGFOLDER)/draw.o
PREFIXDIR  = /usr/local
print-%    : ; @echo $* = $($*)

all: fm drawprog

main.o: parcours.h positions.h scr.h deb.h array.h copy.h props.h

main.o: main.c $(CFILES)
	$(CC) $(CFLAGS) $(IMGCFLAGS) -c $<

fm: $(MAINOBJECT) $(IMGOBJS)
	$(CC) -I. $(CFLAGS) -o $@ $(MAINOBJECT) $(IMGOBJS) $(IMGCFLAGS)

parcours.o: parcours.c
	$(CC) $(CFLAGS) $(IMGCFLAGS) -o $@ -c $<

positions.o: positions.c
	$(CC) $(CFLAGS) $(IMGCFLAGS) -o $@ -c $<

scr.o: scr.c
	$(CC) $(CFLAGS) $(IMGCFLAGS) -o $@ -c $<

deb.o: deb.c
	$(CC) $(CFLAGS) $(IMGCFLAGS) -o $@ -c $<

array.o: array.c
	$(CC) $(CFLAGS) $(IMGCFLAGS) -o $@ -c $<

copy.o: copy.c
	$(CC) $(CFLAGS) $(IMGCFLAGS) -o $@ -c $<

drawprog: $(IMGFOLDER)/draw
	mv $(IMGFOLDER)/draw .

$(IMGFOLDER)/draw.o: $(IMGFOLDER)/props.h

$(IMGFOLDER)/draw.o: $(IMGFOLDER)/draw.c $(IMGCFILES)
	$(CC) $(CBASEFLAGS) $(DRAWCFLAGS) -c $<

$(IMGFOLDER)/draw: $(DRAWOBJECT) $(PROPSOBJS)
	$(CC) -I. $(CBASEFLAGS) -o $@ $(DRAWOBJECT) $(PROPSOBJS) $(DRAWCFLAGS)

$(IMGFOLDER)/props.o: $(IMGFOLDER)/props.c
	$(CC) $(CBASEFLAGS) $(DRAWCFLAGS) -o $@ -c $<

$(IMGFOLDER)/draw.o: $(IMGFOLDER)/draw.c $(IMGFOLDER)/stb_image.h $(IMGFOLDER)/stb_image_resize.h
	$(CC) $(CBASEFLAGS) $(DRAWCFLAGS) -o $@ -c $<

.PHONY: all clean install uninstall

install:
	@echo "Installing fm"
	mkdir -p $(PREFIXDIR)/bin
	cp fm $(PREFIXDIR)/bin/
	chmod 755 $(PREFIXDIR)/bin/fm
	cp draw $(PREFIXDIR)/bin/
	chmod 755 $(PREFIXDIR)/bin/draw

uninstall:
	@echo "Uninstalling fm"
	rm -f $(PREFIXDIR)/bin/fm
	rm -f $(PREFIXDIR)/bin/draw

clean:
	rm -f $(IMGOBJS) $(MAINOBJECT) $(PROPSOBJS) $(DRAWOBJECT) fm draw
