#CC = ccmalloc gcc-2.95
CC = gcc
CFLAGS += -Wall -g -fPIC -DLINUX

SOURCES = alc_channel.c alc_hdr.c alc_rx.c alc_session.c alc_socket.c alc_tx.c \
	lct_hdr.c mad.c transport.c blocking_alg.c

OBJECTS = $(SOURCES:%.c=%.o)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

all: ../lib/libalc.a

../lib/libalc.a: $(OBJECTS)
	@echo "creating library $@..."
	ar rc $@ $(OBJECTS)
	ranlib $@

clean:
	rm -f ../lib/libalc.a *.o *~
