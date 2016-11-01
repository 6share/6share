#CC = ccmalloc gcc-2.95
CC = gcc
CFLAGS += -Wall -g -fPIC -DLINUX -D__VXWORKS_OS__

SOURCES = mad_rlc.c

OBJECTS = $(SOURCES:%.c=%.o)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

all: ../lib/librlc.a

../lib/librlc.a: $(OBJECTS)
	@echo "creating library $@..."
	ar rc $@ $(OBJECTS)
	ranlib $@

clean:
	rm -f ../lib/librlc.a *.o *~
