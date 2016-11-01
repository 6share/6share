#CC = ccmalloc gcc-2.95
CC = gcc
CFLAGS += -Wall -g -I/usr/local/ssl/include/ -I/usr/X11R6/include -I/usr/local/include/ -DLINUX -D__VXWORKS_OS__
LFLAGS += -DLINUX -D__VXWORKS_OS__ -L../lib -L/usr/local/ssl/lib/ \
	-L/usr/local/lib/ -lz -lalc -lsdp -lnullfec -lxorfec -lrsfec -lrlc -lexpat -lpthread -lm -lssl -lcrypto #-lmpatrol -lelf
SOURCES = main.c flute.c sender.c receiver.c fdt.c fdt_gen.c mad_md5.c uri.c getdnsname.c mad_zlib.c

GTK_CFLAGS = `pkg-config --cflags gtk+-2.0 glib-2.0`
GTK_LFLAGS = `pkg-config --libs gtk+-2.0 glib-2.0 gthread-2.0`
GTK_SOURCES = gtk.c flute.c sender.c receiver.c fdt.c fdt_gen.c mad_md5.c uri.c getdnsname.c mad_zlib.c

OBJECTS = $(SOURCES:%.c=%.o)
GTK_OBJECTS = $(GTK_SOURCES:%.c=%.o)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

all: 6share

gtk.o: gtk.c
	$(CC) -c $(CFLAGS) $(GTK_CFLAGS) -o gtk.o gtk.c

flute: $(OBJECTS)
	$(CC) -o ../bin/flute $(OBJECTS) $(LFLAGS)

6share: $(GTK_OBJECTS)
	$(CC) -o ../bin/6share $(GTK_OBJECTS) $(LFLAGS) $(GTK_LFLAGS)

clean:
	rm -f ../bin/6share *.o *~

main.c: ../alclib/inc.h flute.h sender.h receiver.h fdt.h fdt_gen.h mad_md5.h uri.h getdnsname.h
