DESTDIR=libxenomai
XENOCONFIG=$(DESTDIR)/usr/xenomai/bin/xeno-config
CC=	$(shell DESTDIR=$(DESTDIR) $(XENOCONFIG) --cc)
CFLAGS= $(shell DESTDIR=$(DESTDIR) $(XENOCONFIG) --posix --alchemy --cflags)
LDFLAGS=$(shell DESTDIR=$(DESTDIR) $(XENOCONFIG) --posix --alchemy --ldflags)

all: snd_player

snd_player: snd_player.o io_utils.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o : %.c
	$(CC) -o $@ -c $< $(CFLAGS) -Wall -std=c99

clean:
	rm -f *.o snd_player

.PHONY: clean

