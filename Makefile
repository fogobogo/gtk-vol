CC=gcc
CFLAGS=`pkg-config --cflags gtk+-2.0` -Wall -Os
LDFLAGS=`pkg-config --libs gtk+-2.0`

PREFIX=/usr/local
BINDIR=/bin

SOURCES=gtk-vol.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=gtk-vol

all : obj bin

bin :
	$(CC) $(OBJECTS) -o $(EXECUTABLE) $(CFLAGS) $(LDFLAGS)

obj:
	$(CC) -c $(CFLAGS) $(SOURCES) -o $(OBJECTS)

install: 
	install -d $(DESTDIR)$(PREFIX)$(BINDIR) # create directory if nonexistant 
	install $(EXECUTABLE) $(DESTDIR)$(PREFIX)$(BINDIR)

clean:
	rm $(EXECUTABLE) $(OBJECTS)
