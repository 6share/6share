#CC = ccmalloc gcc-2.95
CC = gcc
CFLAGS += -Wall -g -fPIC -DLINUX -D__VXWORKS_OS__

SOURCES = rs_fec.c fec.c

OBJECTS = $(SOURCES:%.c=%.o)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

all: ../lib/librsfec.a

../lib/librsfec.a: $(OBJECTS)
	@echo "creating library $@..."
	ar rc $@ $(OBJECTS)
	ranlib $@

clean:
	rm -f ../lib/librsfec.a *.o *~
