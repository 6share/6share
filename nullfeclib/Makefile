#CC = ccmalloc gcc-2.95
CC = gcc
CFLAGS += -Wall -g -fPIC -DLINUX -D__VXWORKS_OS__

SOURCES = null_fec.c

OBJECTS = $(SOURCES:%.c=%.o)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

all: ../lib/libnullfec.a

../lib/libnullfec.a: $(OBJECTS)
	@echo "creating library $@..."
	ar rc $@ $(OBJECTS)
	ranlib $@

clean:
	rm -f ../lib/libnullfec.a *.o *~
