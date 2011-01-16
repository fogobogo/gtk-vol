CC=gcc
CFLAGS=`pkg-config --cflags gtk+-2.0`
LDFLAGS=`pkg-config --libs gtk+-2.0`

SOURCES=gtk-vol.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=gtk-vol

all : obj bin

bin :
	$(CC) $(OBJECTS) -o $(EXECUTABLE) $(CFLAGS) $(LDFLAGS)

obj:
	$(CC) -c $(CFLAGS) $(SOURCES) -o $(OBJECTS)

clean:
	rm $(EXECUTABLE) $(OBJECTS)
