ALL=topline

CC=gcc
CFLAGS=-Wall -Og -g

all: $(ALL)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<
*.o:	topline.h

topline: topline.o cpu.o disk.o signals.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(ALL) *.o
