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
OPT=-Os
# needs optimization and does not mix with the sanitizers, so the debug and
# sanitize builds turn it off
FORTIFY=-D_FORTIFY_SOURCE=2
# -fPIE only makes a position independent executable on Linux when the link
# step asks for it too. MacOS does this by default and warns about the flag.
ifeq ($(shell uname -s),Linux)
LDFLAGS=-pie
else
LDFLAGS=
endif
LDLIBS=-lcurses
WARNING=-Wall -Wextra -Wpedantic -Wfloat-equal -Wundef -Wshadow \
		-Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes \
		-Wstrict-overflow=5 -Wwrite-strings -Waggregate-return -Wcast-qual \
		-Wswitch-enum -Wunreachable-code -Wformat=2 \
		-Wno-error=deprecated-declarations

FLAGS=-fstack-protector-all -fPIE
CFLAGS=$(WARNING) $(STD) $(OPT) $(FORTIFY) $(FLAGS)

SRC = $(wildcard *.c) $(wildcard extern/*.c)
HEADERS = $(wildcard *.h)
OBJS = $(patsubst %.c,%.o,$(SRC))

# Objects from one kind of build must not be reused by another (e.g. 'make'
# followed by 'make debug'), so remember which kind was built last and start
# over when it changes. Dry runs (make -n) leave everything alone.
BUILD_TYPE := $(firstword $(filter debug static sanitize,$(MAKECMDGOALS)) small)
ifeq ($(findstring n,$(firstword -$(MAKEFLAGS))),)
ifneq ($(BUILD_TYPE),$(shell cat .build-type 2>/dev/null))
$(shell rm -f vin $(OBJS); echo $(BUILD_TYPE) > .build-type)
endif
endif

.PHONY: all
all: small

.PHONY: small
small: OPT := -Os
small: vin

.PHONY: debug
debug: OPT := -ggdb3 -O0 -Werror -DDEBUG -fsanitize=address
debug: FORTIFY :=
debug: vin

.PHONY: static
static: CC := cc -static
static: LDFLAGS :=
static: LDLIBS := -lcurses -ltinfo
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

.PHONY: sanitize
sanitize: OPT := -ggdb3 -O0 -Werror -DDEBUG \
	-fsanitize=address \
	-fsanitize=undefined
sanitize: FORTIFY :=
sanitize: vin

vin: $(OBJS)
	$(CC) -o vin $(OBJS) $(CFLAGS) $(LDFLAGS) $(LDLIBS)

%.o: %.c $(HEADERS)
	$(CC) -c $< -o $@ $(CFLAGS)

.PHONY: clean
clean:
	rm -f vin
	rm -f *.o
	rm -f .build-type
	rm -f core
	rm -f a.out
