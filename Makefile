CC = gcc
BIN = gtk-vol
LIBS = `pkg-config --libs gtk+-2.0`
CFLAGS = `pkg-config --cflags gtk+-2.0` -O2 -Wall

all:
	$(CC) gtk-vol.c -o $(BIN) $(CFLAGS) $(LIBS)

clean:
	rm $(BIN)
