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

#include "text.h"

struct Cursor {
    size_t x;
    size_t y;
    size_t old_x;
    size_t old_y;
    struct Text *line;
    struct Text *top_of_text;
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
    FILE *fp,
    int c
) {
    wputchar(win, cur, c);
    switch (c) {
        case 27: /* escape key */
            *mode = NORMAL;
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "            ");
            cur->x = cur->old_x;
            cur->y = cur->old_y;
            break;

        case '\n':
            *mode = NORMAL;
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "            ");
            cur->x = cur->old_x;
            cur->y = cur->old_y;
            break;

        case 'q':
            *mode = QUIT;
            break;

        case 'w':
            /* write out */
            text_write(cur->top_of_text, fp);
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
            text_backspace(cur->line, cur->x);
            break;

        case '\n':
            wputchar(win, cur, c);
            cur->y++;
            cur->x = 0;
            cur->line = text_new_line(cur->line, cur->line->next);
            break;

        default:
            wputchar(win, cur, c);
            text_insert_char(cur->line, c);
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
            cur->old_x = cur->x;
            cur->old_y = cur->y;
            cur->x = 0;
            cur->y = win->maxlines - 1;
            wmove(win->curses_win, win->maxlines - 1, 0);
            wputchar(win, cur, c);
            break;

        default:
            break;
    }
}

static int event_loop(struct Window *win, struct Cursor *cur, FILE *fp) {
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
                handle_ex_mode(win, cur, &mode, fp, c);
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
    cur.line = text_new_line(NULL, NULL);
    cur.top_of_text = cur.line;

    /* setup curses */
    initscr();
    cbreak();
    noecho();

    clear();

    win.maxlines = LINES;
    win.maxcols = COLS;

    puts(argv[1]);
    if (argc == 2) {
        fp = fopen(argv[1], "w");
    }

    win.curses_win = newwin(win.maxlines, win.maxcols, cur.x, cur.y);

    event_loop(&win, &cur, fp);

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
