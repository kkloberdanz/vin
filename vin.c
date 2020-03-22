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
#include <ctype.h>
#include <signal.h>
#include <limits.h>

#include "vin.h"
#include "text.h"
#include "command.h"

#define UNUSED(A) (void)(A)

#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define MAX(A, B) ((A) > (B) ? (A) : (B))

#define FLASH_MSG(MSG) \
    do { \
        waddstr(win->curses_win, "                                      "); \
        wmove(win->curses_win, win->maxlines - 1, 0); \
        waddstr(win->curses_win, (MSG)); \
        wmove(win->curses_win, cur->y, cur->x); \
    } while (0)

void sigint_handler(int sig) {
    signal(sig, SIG_IGN);
}

static void cursor_advance(struct Cursor *cur) {
    cur->x++;
}

static void wputchar(struct Window *win, struct Cursor *cur, int c) {
    waddch(win->curses_win, c);
    cursor_advance(cur);
}

static void redraw_screen(
    struct Window *win,
    struct Cursor *cur,
    enum Mode mode
) {
    struct Text *line;
    size_t i;
    char msg[80] = {0};
    memset(msg, ' ', 79);
    wclear(win->curses_win);
    for (i = 0, line = cur->top_of_screen; line; line = line->next, i++) {
        if (i >= win->maxlines - 1) {
            break;
        }
        line->len = strlen(line->data);
        wmove(win->curses_win, i, 0);
        waddstr(win->curses_win, line->data);
    }

    switch (mode) {
        case INSERT:
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, msg);
            wmove(win->curses_win, win->maxlines - 1, 0);
            waddstr(win->curses_win, "-- INSERT --");
            break;

        default:
            break;
    }

    wmove(win->curses_win, win->maxlines - 1, 55);
    sprintf(msg, "%lu - %lu", cur->x + 1, cur->line_no);
    waddstr(win->curses_win, msg);
    wmove(win->curses_win, cur->y, cur->x);
    wrefresh(win->curses_win);
}

static void handle_ex_mode(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode,
    char *filename,
    int c
) {
    wputchar(win, cur, c);
    /* TODO: put this in a while loop to read in commands */
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
            text_write(cur->top_of_text, filename);
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
            cur->line = text_split_line(cur->line, cur->x);
            cur->x = 0;
            redraw_screen(win, cur, *mode);
            break;

        case '\t':
            for (i = 0; i < 4; i++) {
                text_insert_char(cur->line, cur->x, ' ');
                cursor_advance(cur);
            }
            redraw_screen(win, cur, *mode);
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
                cur->line_no--;
                cur->line = cur->line->prev;
                if (cur->line->len > 2) {
                    pos = cur->line->len - 2;
                } else {
                    pos = 0;
                }

                cur->old_x = MAX(cur->x, cur->old_x);
                cur->x = MIN(cur->old_x, pos);
                if (cur->y > 0) {
                    cur->y--;
                } else {
                    if (cur->top_of_screen) {
                        cur->top_of_screen = cur->top_of_screen->prev;
                    }
                }
            }
            break;

        case '\n':
        case 'j':
            if (cur->line->next) {
                cur->line_no++;
                cur->line = cur->line->next;
                if (cur->line->len > 2) {
                    pos = cur->line->len - 2;
                } else {
                    pos = 0;
                }

                cur->old_x = MAX(cur->x, cur->old_x);
                cur->x = MIN(cur->old_x, pos);
                if (cur->y < win->maxlines - 2) {
                    cur->y++;
                } else {
                    if (cur->top_of_screen) {
                        cur->top_of_screen = cur->top_of_screen->next;
                    }
                }
            }
            break;

        case ' ':
        case 'l':
            cur->line->len = strlen(cur->line->data);
            if (cur->line->len > 2) {
                pos = cur->line->len - 2;
            } else {
                pos = 0;
            }
            if (cur->x < pos) {
                if (cur->x < win->maxcols - 1) {
                    cur->x++;
                }
            }
            cur->old_x = cur->x;
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
                redraw_screen(win, cur, *mode);
            }
            break;

        case 'r':
            if ((cur->x < cur->line->len)
                    && (cur->line->data[cur->x] != '\n')) {
                cur->line->data[cur->x] = wgetch(win->curses_win);
                redraw_screen(win, cur, *mode);
            }
            break;

        case '~': {
            char *under_cursor = &cur->line->data[cur->x];
            if (isalpha(*under_cursor)) {
                *under_cursor ^= 0x20;
            }
            cursor_advance(cur);
            break;
        }

        case 'y': {
            char next_cmd = wgetch(win->curses_win);
            switch (next_cmd) {
                case 'y':
                    if (cur->clipboard) {
                        free(cur->clipboard->data);
                        free(cur->clipboard);
                    }
                    cur->clipboard = text_copy_line(cur->line);
                    break;

                default:
                    break;
            }
            break;
        }

        case 'p': {
            if (cur->clipboard) {
                struct Text *line = text_copy_line(cur->clipboard);
                text_insert_line(cur->line, line, cur->line->next);
            }
            break;
        }

        case 'd':
            if (*(cmd->data) == 'd') {
                if (cur->y > 0) {
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
                    redraw_screen(win, cur, *mode);
                    cmd->len = 0;
                    memset(cmd, 0, 80);
                } else {
                    handle_normal_mode(win, cur, mode, 'D', cmd);
                    cur->y = 0;
                    cur->x = 0;
                }
            } else {
                command_add_char(cmd, c);
            }
            break;

        case 'D': {
            char *start_ptr = cur->line->data + cur->x;
            memset(start_ptr, 0, strlen(start_ptr));
            cur->line->data[cur->x] = '\n';
            cur->x--;
            cur->line->len = strlen(cur->line->data);
            redraw_screen(win, cur, *mode);
            break;
        }

        case '$':
        case 'E': {
            size_t pos;
            cur->line->len = strlen(cur->line->data);
            if (cur->line->len > 2) {
                pos = cur->line->len - 2;
            } else {
                pos = 0;
            }
            cur->old_x = SIZE_MAX;
            cur->x = pos;
            break;
        }

        case 'O': {
            struct Text *new_line = text_make_line();
            *mode = INSERT;
            text_insert_line(cur->line->prev, new_line, cur->line);
            cur->x = 0;
            cur->line = new_line;
            cur->line->len = strlen(cur->line->data);
            break;
        }

        case 'o': {
            struct Text *new_line = text_make_line();
            cur->y++;
            cur->line_no++;
            cur->x = 0;

            text_insert_line(cur->line, new_line, cur->line->next);

            cur->line = cur->line->next;
            text_push_char(cur->line, '\n');

            wmove(win->curses_win, cur->y + 1, 0);
            waddstr(win->curses_win, cur->line->next->data);
            wmove(win->curses_win, cur->y, cur->x);

            *mode = INSERT;
            wmove(win->curses_win, cur->y, cur->x);
            redraw_screen(win, cur, *mode);
            break;
        }

        case 'i':
            *mode = INSERT;
            wmove(win->curses_win, cur->y, cur->x);
            redraw_screen(win, cur, *mode);
            break;

        case 'g': {
            char next_c = wgetch(win->curses_win);
            switch (next_c) {
                case 'g':
                    cur->x = 0;
                    cur->y = 0;
                    cur->line_no = 1;
                    cur->line = cur->top_of_text;
                    cur->top_of_screen = cur->top_of_text;
                    break;

                default:
                    break;
            }
            break;
        }

        case 'G':
            cur->x = 0;
            cur->y = 0;
            cur->line_no = 1;
            cur->line = cur->top_of_text;
            cur->top_of_screen = cur->top_of_text;
            for (; cur->line->next; cur->line = cur->line->next) {
                cur->line_no++;
            }
            cur->top_of_screen = cur->line;
            break;

        case 'a':
            *mode = INSERT;
            cursor_advance(cur);
            wmove(win->curses_win, cur->y, cur->x);
            break;

        case 'A':
            *mode = INSERT;
            cur->x = cur->line->len - 1;
            break;

        case '0':
            cur->old_x = 0;
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
            redraw_screen(win, cur, *mode);
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

static int event_loop(
    struct Window *win,
    struct Cursor *cur,
    char *filename
) {
    int c;
    enum Mode mode = NORMAL;
    struct Command cmd;
    redraw_screen(win, cur, mode);
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
                handle_ex_mode(win, cur, &mode, filename, c);
                break;

            case QUIT:
                return 0;
        }

        switch (mode) {
            case NORMAL:
            case INSERT:
                redraw_screen(win, cur, mode);

            default:
                break;
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
    char *filename = NULL;
    struct Cursor cur;
    struct Text *line;

    signal(SIGINT, sigint_handler);

    cur.x = 0;
    cur.old_x = 0;
    cur.y = 0;
    cur.old_y = 0;
    cur.line = text_make_line();
    cur.top_of_text = cur.line;
    cur.clipboard = NULL;
    cur.line_no = 1;

    /* setup curses */
    initscr();
    cbreak();
    noecho();

    clear();

    win.maxlines = LINES;
    win.maxcols = COLS;

    if (argc == 2) {
        filename = argv[1];
        fp = fopen(argv[1], "r");
        if (fp) {
            text_read_from_file(cur.line, fp);
            fclose(fp);
        }
    }

    cur.line = cur.top_of_text;
    cur.top_of_screen = cur.top_of_text;
    win.curses_win = newwin(win.maxlines, win.maxcols, cur.x, cur.y);
    event_loop(&win, &cur, filename);

    for (line = cur.top_of_text; line; line = line->next) {
        free(line->data);
        free(line);
    }

    free(cur.clipboard);

    /* exit curses */
    clrtoeol();
    refresh();
    endwin();

    return 0;
}
