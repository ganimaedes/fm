CC         = gcc
CBASEFLAGS = -ggdb3 -Og -rdynamic -DWITH_STBI
CFLAGS 	   = -I. $(CBASEFLAGS) -Wall -Wextra -pedantic
IMGCFLAGS  = -I/usr/local/include -lX11 -lm -pthread
PROPSOBJS  = props.o
IMGCFILES  = props.c
DRAWOBJECT = draw.o
IMGLIB     = stb_image.h stb_image_resize.h
print-%    : ; @echo $* = $($*)

drawprog: draw

draw.o: props.h

draw.o: draw.c $(IMGCFILES)
	$(CC) $(CFLAGS) $(IMGCFLAGS) -c $<

draw: $(DRAWOBJECT) $(PROPSOBJS)
	$(CC) -I. $(CFLAGS) -o $@ $(DRAWOBJECT) $(PROPSOBJS) $(IMGCFLAGS)

props.o: props.c
	$(CC) $(CFLAGS) $(IMGCFLAGS) -o $@ -c $<

draw.o: draw.c $(IMGLIB)
	$(CC) $(CFLAGS) $(IMGCFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	rm -f $(PROPSOBJS) $(DRAWOBJECT) draw
