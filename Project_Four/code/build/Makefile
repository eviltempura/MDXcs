all: logappend logread

CFLAGS=-g -Wall -std=c99
LFLAGS=-lssl -lcrypto
UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
CFLAGS += -DLINUX -I/usr/local/ssl/include -L/usr/local/ssl/lib
endif

logappend: logappend_skel.o
	$(CC) -o $@ $(CFLAGS) $^ $(LFLAGS)

logappend_skel.o: logappend_skel.c 
	$(CC) -c -o $@ $(CFLAGS) $^

logread: logread.o 
	$(CC) -o $@ $(CFLAGS) $^ $(LFLAGS)

logread.o: logread.c
	$(CC) -c -o $@ $(CFLAGS) $^

clean:
	rm -f *.o logappend logread
