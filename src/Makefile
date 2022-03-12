CC         = gcc
CBASEFLAGS = -ggdb3 -O0 -rdynamic -DEBUG #-DEBUG2
CFLAGS 	   = -I. $(CBASEFLAGS) -Wall -pedantic -finstrument-functions
IMGCFLAGS  = -I/usr/local/include
IMGOBJS    = parcours.o positions.o scr.o deb.o array.o copy.o
CFILES     = parcours.c positions.c scr.c deb.c array.c copy.c
MAINOBJECT = main.o
print-%    : ; @echo $* = $($*)

all: fm

main.o: parcours.h positions.h scr.h deb.h array.h copy.h

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

.PHONY: clean

clean:
	rm -f $(IMGOBJS) $(MAINOBJECT) fm
