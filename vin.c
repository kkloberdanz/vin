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

#include <stdio.h>
#include <curses.h>

struct Cursor {
    size_t x;
    size_t y;
};

struct Window {
    WINDOW *curses_win;
    size_t maxlines;
    size_t maxcols;
};

enum Mode {
    NORMAL,
    INSERT,
    EX,
    QUIT
};

static void cursor_advance(struct Cursor *cur) {
    cur->x++;
}

static void wputchar(struct Window *win, struct Cursor *cur, int c) {
    waddch(win->curses_win, c);
    cursor_advance(cur);
}

static void handle_ex_mode(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode,
    int c
) {
    wputchar(win, cur, c);
    switch (c) {
        case 27: /* escape key */
            *mode = NORMAL;
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "            ");
            break;

        case '\n':
            *mode = NORMAL;
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "            ");
            break;

        case 'q':
            *mode = QUIT;
            break;

        default:
            break;
    }
}


static void handle_insert_mode(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode,
    int c
) {
    switch (c) {
        case 27: /* escape key */
            *mode = NORMAL;
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "            ");
            break;

        case 127: /* backspace key */
            cur->x--;
            wmove(win->curses_win, cur->y, cur->x);
            wrefresh(win->curses_win);
            waddch(win->curses_win, ' ');
            break;

        case '\n':
            wputchar(win, cur, c);
            cur->y++;
            cur->x = 0;
            break;

        default:
            wputchar(win, cur, c);
    }
}

static void handle_normal_mode(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode,
    int c
) {
    switch (c) {
        case 'k':
            if (cur->y > 0) {
                cur->y--;
            }
            break;

        case '\n':
        case 'j':
            if (cur->y < win->maxlines - 1) {
                cur->y++;
            }
            break;

        case ' ':
        case 'l':
            if (cur->x < win->maxcols - 1) {
                cur->x++;
            }
            break;

        case 'h':
            if (cur->x > 0) {
                cur->x--;
            }
            break;

        case 'x':
            wputchar(win, cur, ' ');
            break;

        case 'o':
            cur->y++;
            cur->x = 0;
            /* fallthrough */

        case 'i':
            *mode = INSERT;
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "-- INSERT --");
            break;

        case '0':
            cur->x = 0;
            break;

        case ':':
            *mode = EX;
            cur->x = 0;
            cur->y = win->maxlines - 1;
            wmove(win->curses_win, win->maxlines - 1, 0);
            wputchar(win, cur, c);
            break;

        default:
            break;
    }
}

static int event_loop(struct Window *win, struct Cursor *cur) {
    int c;
    enum Mode mode = NORMAL;
    while ((c = wgetch(win->curses_win))) {
        switch (mode) {
            case NORMAL:
                handle_normal_mode(win, cur, &mode, c);
                break;

            case INSERT:
                handle_insert_mode(win, cur, &mode, c);
                break;

            case EX:
                handle_ex_mode(win, cur, &mode, c);
                break;

            case QUIT:
                return 0;
        }
        getmaxyx(win->curses_win, win->maxlines, win->maxcols);
        wmove(win->curses_win, cur->y, cur->x);
        wrefresh(win->curses_win);
    }
    return 1;
}

int main(int argc, char **argv) {
    struct Window win;
    FILE *fp = NULL;
    struct Cursor cur;

    cur.x = 0;
    cur.y = 0;

    /* setup curses */
    initscr();
    cbreak();
    noecho();

    clear();

    win.maxlines = LINES;
    win.maxcols = COLS;

    if (argc == 2) {
        fp = fopen(argv[1], "rw");
    }

    win.curses_win = newwin(win.maxlines, win.maxcols, cur.x, cur.y);

    event_loop(&win, &cur);

    clrtoeol();
    refresh();
    endwin();

    if (fp) {
        fclose(fp);
    }

    /* exit curses */
    refresh();
    endwin();

    return 0;
}
