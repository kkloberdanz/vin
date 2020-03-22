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
#include <stdlib.h>
#include <curses.h>
#include <string.h>

#include "vin.h"
#include "text.h"
#include "command.h"

#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define FLASH_MSG(MSG) \
    do { \
        waddstr(win->curses_win, "                                      "); \
        wmove(win->curses_win, win->maxlines - 1, 0); \
        waddstr(win->curses_win, (MSG)); \
        wmove(win->curses_win, cur->y, cur->x); \
    } while (0) \

static void cursor_advance(struct Cursor *cur) {
    cur->x++;
}

static void wputchar(struct Window *win, struct Cursor *cur, int c) {
    waddch(win->curses_win, c);
    cursor_advance(cur);
}

static void redraw_screen(struct Window *win, struct Cursor *cur) {
    struct Text *line;
    size_t i;
    wclear(win->curses_win);
    for (i = 0, line = cur->top_of_text; line; line = line->next, i++) {
        wmove(win->curses_win, i, 0);
        waddstr(win->curses_win, line->data);
    }
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
            waddstr(win->curses_win, "                                      ");
            cur->x = cur->old_x;
            cur->y = cur->old_y;
            break;

        case '\n':
            *mode = NORMAL;
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "                                      ");
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
    int i;
    switch (c) {
        case 27: /* escape key */
            *mode = NORMAL;
            cur->x--;
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "            ");
            break;

        case 127: /* backspace key */
            cur->x--;
            if (cur->line && cur->line->len > 0) {
                cur->line->len--;
            }
            wmove(win->curses_win, cur->y, cur->x);
            wrefresh(win->curses_win);
            waddch(win->curses_win, ' ');
            text_backspace(cur->line, cur->x);
            break;

        case '\n':
            cur->y++;
            cur->x = 0;
            cur->line = text_new_line(cur->line, cur->line->next);
            redraw_screen(win, cur);
            break;

        case '\t':
            for (i = 0; i < 4; i++) {
                text_insert_char(cur->line, cur->x, ' ');
                cursor_advance(cur);
                wmove(win->curses_win, cur->y, 0);
                waddstr(win->curses_win, cur->line->data);
                wmove(win->curses_win, cur->y, cur->x);
            }
            break;

        default:
            text_insert_char(cur->line, cur->x, c);
            cursor_advance(cur);
            wmove(win->curses_win, cur->y, 0);
            waddstr(win->curses_win, cur->line->data);
            wmove(win->curses_win, cur->y, cur->x);
    }
}

static void handle_normal_mode(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode,
    int c,
    struct Command *cmd
) {
    size_t pos;
    char msg_buf[80];
    switch (c) {
        case 'k':
            if (cur->line->prev) {
                cur->line = cur->line->prev;
                pos = cur->line->len - 2;
                cur->x = (pos > cur->x) ? 0 : MIN(cur->x, pos);
                if (cur->y > 0) {
                    cur->y--;
                }
            }
            break;

        case '\n':
        case 'j':
            if (cur->line->next && *(cur->line->next->data)) {
                cur->line = cur->line->next;
                pos = cur->line->len - 2;
                cur->x = (pos > cur->x) ? 0 : MIN(cur->x, pos);
                sprintf(msg_buf, "%lu %lu\n", cur->x, cur->y);
                if (cur->y < win->maxlines - 1) {
                    cur->y++;
                }
            }
            break;

        case ' ':
        case 'l':
            pos = cur->line->len - 2;
            if (cur->x < pos) {
                if (cur->x < win->maxcols - 1) {
                    cur->x++;
                }
            }
            break;

        case 'h':
            if (cur->x > 0) {
                cur->x--;
            }
            break;

        case 'x':
            if ((cur->x < cur->line->len) 
                    && (cur->line->data[cur->x] != '\n')) {
                text_shift_left(cur->line, cur->x);
                redraw_screen(win, cur);
            }
            break;

        case 'd':
            if (*(cmd->data) == 'd') {
                struct Text *tmp;
                cur->line->prev->next = cur->line->next;
                if (cur->line->next) {
                    cur->line->next->prev = cur->line->prev;
                    tmp = cur->line;
                    cur->line = cur->line->next;
                } else {
                    tmp = cur->line;
                    cur->line = cur->line->prev;
                    cur->line->next = NULL;
                }
                free(tmp->data);
                free(tmp);
                redraw_screen(win, cur);
                cmd->len = 0;
                memset(cmd, 0, 80);
            } else {
                command_add_char(cmd, c);
            }
            break;

        case '$':
        case 'E':
            cur->x = cur->line->len - 2;
            break;

        case 'o':
            cur->y++;
            cur->x = 0;

            cur->line = text_new_line(cur->line, cur->line->next);
            text_push_char(cur->line, '\n');
            if (cur->line->next) {
                wmove(win->curses_win, cur->y + 1, 0);
                waddstr(win->curses_win, cur->line->next->data);
                wmove(win->curses_win, cur->y, cur->x);
            }
            redraw_screen(win, cur);
            /* fallthrough */

        case 'i':
            *mode = INSERT;
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "                                      ");
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "-- INSERT --");
            wmove(win->curses_win, cur->y, cur->x);
            break;

        case 'a':
            *mode = INSERT;
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "                                      ");
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "-- INSERT --");
            cursor_advance(cur);
            wmove(win->curses_win, cur->y, cur->x);
            break;

        case 'A':
            *mode = INSERT;
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "                                      ");
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "-- INSERT --");
            cur->x = cur->line->len;
            wmove(win->curses_win, cur->y, cur->x);
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
            waddstr(win->curses_win, "                                      ");
            wmove(win->curses_win, win->maxlines - 1, 0);
            wputchar(win, cur, c);
            break;

        case '\f':
            redraw_screen(win, cur);
            break;

        default:
            wmove(win->curses_win, win->maxlines - 1, 0);
            switch (c) {
                case 27:
                    break;

                default:
                    sprintf(msg_buf, "not an editor command: %c", c);
                    waddstr(win->curses_win, msg_buf);
                    wmove(win->curses_win, cur->y, cur->x);
            }
            break;
    }
}

static int event_loop(struct Window *win, struct Cursor *cur, FILE *fp) {
    int c;
    enum Mode mode = NORMAL;
    struct Command cmd;
    redraw_screen(win, cur);
    cur->x = 0;
    cur->y = 0;
    cmd.len = 0;
    while ((c = wgetch(win->curses_win))) {
        switch (mode) {
            case NORMAL:
                handle_normal_mode(win, cur, &mode, c, &cmd);
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
    struct Text *line;

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

    if (argc == 2) {
        fp = fopen(argv[1], "r+");
        if (!fp) {
            fp = fopen(argv[1], "w");
        } else {
            text_read_from_file(cur.line, fp);
        }
    }

    cur.line = cur.top_of_text;
    win.curses_win = newwin(win.maxlines, win.maxcols, cur.x, cur.y);
    event_loop(&win, &cur, fp);

    for (line = cur.top_of_text; line; line = line->next) {
        free(line->data);
        free(line);
    }
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
