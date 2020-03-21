CC=cc
STD=-std=c89
WARN_FLAGS=-Wall -Wextra -Wpedantic
OPTIM=-Os
LDFLAGS=-lcurses
CFLAGS= $(WARN_FLAGS) $(OPTIM) $(STD)

all: vin

debug: OPTIM := -ggdb3 -O0
debug: all

vin: vin.c
	$(CC) -o vin vin.c $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f vin
