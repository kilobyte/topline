ALL=bload

CC=gcc
CFLAGS=-Wall -O2

all: $(ALL)

.c.o:
	$(CC) $(CFLAGS) -c $<

bload: bload.o disk.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(ALL) *.o
