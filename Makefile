 #
 #     Copyright (C) 2020 Kyle Kloberdanz
 #
 #  This program is free software: you can redistribute it and/or modify
 #  it under the terms of the GNU Affero General Public License as
 #  published by the Free Software Foundation, either version 3 of the
 #  License, or (at your option) any later version.
 #
 #  This program is distributed in the hope that it will be useful,
 #  but WITHOUT ANY WARRANTY; without even the implied warranty of
 #  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 #  GNU Affero General Public License for more details.
 #
 #  You should have received a copy of the GNU Affero General Public License
 #  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 #

CC=cc
STD=-std=c89
WARN_FLAGS=-Wall -Wextra -Wpedantic
OPT=-Os
LDFLAGS=-lcurses
WARNING=-Wall -Wextra -Wpedantic -Wfloat-equal -Wundef -Wshadow \
		-Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes \
		-Wstrict-overflow=5 -Wwrite-strings -Waggregate-return -Wcast-qual \
		-Wswitch-enum -Wunreachable-code -Wformat -Wformat -Wformat-security

FLAGS=-fstack-protector-all -fPIC -D_FORTIFY_SOURCE=2
CFLAGS=$(WARNING) $(STD) $(OPT) $(FLAGS)

SRC = $(wildcard *.c) $(wildcard extern/*.c)
HEADERS = $(wildcard *.h)
OBJS = $(patsubst %.c,%.o,$(SRC))

.PHONY: all
all: small

.PHONY: small
small: OPTIM := -Os
small: vin
	strip \
		-S \
		--strip-unneeded \
		--remove-section=.note.gnu.gold-version \
		--remove-section=.comment \
		--remove-section=.note \
		--remove-section=.note.gnu.build-id \
		--remove-section=.note.ABI-tag \
		vin

.PHONY: debug
debug: OPTIM := -ggdb3 -O0 -Werror -DDEBUG -fsanitize=address
debug: vin

.PHONY: static
static: CC := cc -static
static: LDFLAGS := -lcurses -ltinfo
static: vin
	strip \
		-S \
		--strip-unneeded \
		--remove-section=.note.gnu.gold-version \
		--remove-section=.comment \
		--remove-section=.note \
		--remove-section=.note.gnu.build-id \
		--remove-section=.note.ABI-tag \
		vin

sanitize: OPTIM := -ggdb3 -O0 -Werror -DDEBUG \
	-fsanitize=address \
	-fsanitize=leak \
	-fsanitize=undefined
sanitize: vin

vin: $(OBJS)
	$(CC) -o vin $(OBJS) $(CFLAGS) $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) -c $< -o $@ $(CFLAGS)

.PHONY: clean
clean:
	rm -f vin
	rm -f *.o
	rm -f core
	rm -f a.out
