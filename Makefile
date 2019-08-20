CC = gcc
CFLAGS = -Wall -pedantic

src = $(wildcard *.c)
obj = $(src:.c=.o)

main: $(obj)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f $(obj) main
