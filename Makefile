CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-pthread -g
PLIKI=serwer

all: $(PLIKI)

clean:
	rm -f $(PLIKI)

.PHONY: all clean
