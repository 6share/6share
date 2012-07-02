#CC = ccmalloc gcc-2.95
CC = gcc
CFLAGS += -Wall -g -fPIC -DLINUX -D__VXWORKS_OS__

SOURCES = port_list.c port_misc.c sdp_accessor.c sdp_rfc2327.c sdplib.c

OBJECTS = $(SOURCES:%.c=%.o)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

all: ../lib/libsdp.a

../lib/libsdp.a: $(OBJECTS)
	@echo "creating library $@..."
	ar rc $@ $(OBJECTS)
	ranlib $@

clean:
	rm -f ../lib/libsdp.a *.o *~
