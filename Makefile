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
OPTIM=-Os
LDFLAGS=-lcurses
CFLAGS= $(WARN_FLAGS) $(OPTIM) $(STD)

SRC = $(wildcard *.c) $(wildcard extern/*.c)
HEADERS = $(wildcard *.h)
OBJS = $(patsubst %.c,%.o,$(SRC))

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

debug: OPTIM := -ggdb3 -O0 -Werror
debug: all

sanitize: OPTIM := -ggdb3 -O0 -Werror \
	-fsanitize=address \
	-fsanitize=leak \
	-fsanitize=undefined
sanitize: all

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
