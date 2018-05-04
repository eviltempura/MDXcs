all: logappend logread

CFLAGS=-g -Wall
LFLAGS=-lcrypto

UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
CFLAGS += -DLINUX -I/usr/local/ssl/include -L/usr/local/ssl/lib
endif

logappend: logappend_skel.o
	$(CC) logappend_skel.o $(CFLAGS) -lcrypto -o logappend

logread: logread_skel.o data_skel.o
	$(CC) logread_skel.o data_skel.o $(CFLAGS) -lcrypto -o logread

logappend_skel.o: logappend_skel.c
	$(CC) logappend_skel.c $(CFLAGS) -lcrypto -c

logread_skel.o: logread_skel.c
	$(CC) logread_skel.c $(CFLAGS) -lcrypto -c

test: test.o
	$(CC) -o $@ $(CFLAGS) $^

test.o: test.c
	$(CC) -c -o $@ $(CFLAGS) $^

clean:
	rm -f *.o logappend logread test.* test log1.txt
