all: logappend logread

CFLAGS=-g -std=gnu99
LIBS=-lcrypto

UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
CFLAGS += -DLINUX -I/usr/local/ssl/include -L/usr/local/ssl/lib
endif

logappend: logappend.o loghelper.o
	$(CC) -o $@ $(CFLAGS) $^ $(LIBS)

logread: logread.o loghelper.o
	$(CC) -o $@ $(CFLAGS) $^ $(LIBS)

logappend.o: logappend.c
	$(CC) -c -o $@ $(CFLAGS) $^

logread.o: logread.c
	$(CC) -c -o $@ $(CFLAGS) $^

loghelper.o: loghelper.c
	$(CC) -c -o $@ $(CFLAGS) $^

clean:
	rm -f *.o logappend logread loghelper