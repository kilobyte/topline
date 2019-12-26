ALL=topline

CC=gcc
CFLAGS=-Wall -Og -g

all: $(ALL)

.c.o:
	$(CC) $(CFLAGS) -c $<
*.o:	topline.h

topline: topline.o cpu.o disk.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(ALL) *.o
