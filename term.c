/*
 *     Copyright (C) 2020 Kyle Kloberdanz
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _DARWIN_C_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "term.h"

#define ESC "\033"

#define ALT_SCREEN_ON  ESC "[?1049h"
#define ALT_SCREEN_OFF ESC "[?1049l"

#define WRAP_OFF ESC "[?7l"
#define WRAP_ON  ESC "[?7h"

#define CLEAR_SCREEN ESC "[2J"
#define CLEAR_TO_EOL ESC "[K"
#define HOME         ESC "[H"

#define NEXT_ROW ESC "[B\r"

static struct termios saved;
static int raw_mode = 0;

static void out(const char *s) {
    fputs(s, stdout);
}

void term_exit(void) {
    if (!raw_mode) {
        return;
    }
    raw_mode = 0;
    out(WRAP_ON);
    out(ALT_SCREEN_OFF);
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved);
}

void term_init(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &saved) == -1) {
        perror("not a terminal");
        exit(1);
    }
    raw = saved;

    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= (tcflag_t)~(IXON | ISTRIP | BRKINT);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(1);
    }
    raw_mode = 1;
    atexit(term_exit);

    setvbuf(stdout, NULL, _IOFBF, 1 << 16);

    out(ALT_SCREEN_ON);
    out(WRAP_OFF);
    out(HOME);
    out(CLEAR_SCREEN);
    fflush(stdout);
}

void term_size(size_t *rows, size_t *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1
            || ws.ws_row == 0 || ws.ws_col == 0) {
        *rows = 24;
        *cols = 80;
    } else {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
    }
}

void term_move(size_t y, size_t x) {
    printf(ESC "[%lu;%luH", (unsigned long)y + 1, (unsigned long)x + 1);
}

void term_home(void) {
    out(HOME);
}

void term_clrtoeol(void) {
    out(CLEAR_TO_EOL);
}

void term_putc(int c) {
    unsigned char uc = (unsigned char)c;
    if (uc == '\n') {
        out(CLEAR_TO_EOL NEXT_ROW);
    } else if (uc == 127) {
        out("^?");
    } else if (uc == '\t' || uc >= 32) {
        putchar(uc);
    } else {
        putchar('^');
        putchar(uc ^ 0x40);
    }
}

void term_puts(const char *s) {
    for (; *s; s++) {
        term_putc(*s);
    }
}

void term_refresh(void) {
    fflush(stdout);
}

int term_getch(void) {
    unsigned char c;
    ssize_t n;
    term_refresh();
    do {
        n = read(STDIN_FILENO, &c, 1);
    } while (n == -1 && errno == EINTR);
    if (n != 1) {
        return -1;
    }
    return c;
}
