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
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <limits.h>
#include <ctype.h>

#include "vin.h"
#include "text.h"
#include "command.h"
#include "term.h"

#define UNUSED(A) (void)(A)

#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define MAX(A, B) ((A) > (B) ? (A) : (B))

#define FLASH_MSG(MSG) \
    do { \
        term_move(win->maxlines - 1, 0); \
        term_puts(blank); \
        term_move(win->maxlines - 1, 0); \
        term_puts((MSG)); \
        term_move(cur->y, cur->x); \
    } while (0)

static const char *blank = "                                      ";

char *strdup(const char *s);

static void sigint_handler(int sig) {
#ifdef DEBUG
    UNUSED(sig);
    term_exit();
    exit(1);
#else
    signal(sig, SIG_IGN);
#endif
}

static void set_clipboard(struct Cursor *cur) {
    free(cur->before);
    cur->before = strdup(cur->line->data);
    cur->before_line = cur->line;
}

static enum Todo handle_input(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode,
    int c,
    struct Command *cmd,
    char *filename
);

static void handle_search_mode(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode
);

static void cursor_advance(struct Cursor *cur) {
    cur->x++;
}

/* move the cursor down one row, scrolling if it is at the bottom */
static void cursor_row_down(struct Window *win, struct Cursor *cur) {
    if (cur->y < win->maxlines - 2) {
        cur->y++;
    } else {
        if (cur->top_of_screen && cur->top_of_screen->next) {
            cur->top_of_screen = cur->top_of_screen->next;
        }
    }
}

/* remove the current line from the text and put it on the clipboard */
static void delete_line(struct Cursor *cur) {
    struct Text *tmp = cur->line;

    if (cur->clipboard) {
        free(cur->clipboard->data);
        free(cur->clipboard);
    }
    cur->clipboard = text_copy_line(tmp);
    cur->x = 0;

    if (!tmp->prev && !tmp->next) {
        /* the only line in the text, so empty it instead of removing it */
        free(tmp->data);
        tmp->data = strdup("\n");
        tmp->len = 1;
        tmp->capacity = 1;
        return;
    }

    if (tmp->prev) {
        tmp->prev->next = tmp->next;
    }
    if (tmp->next) {
        tmp->next->prev = tmp->prev;
        cur->line = tmp->next;
    } else {
        cur->line = tmp->prev;
        cur->line_no--;
        if (cur->y > 0) {
            cur->y--;
        }
    }

    if (cur->top_of_text == tmp) {
        cur->top_of_text = tmp->next;
    }
    if (cur->top_of_screen == tmp) {
        cur->top_of_screen = cur->line;
    }
    if (cur->before_line == tmp) {
        free(cur->before);
        cur->before = NULL;
        cur->before_line = NULL;
    }

    free(tmp->data);
    free(tmp);
}

static void wputchar(struct Cursor *cur, int c) {
    term_putc(c);
    cursor_advance(cur);
}

static void redraw_screen(
    struct Window *win,
    struct Cursor *cur,
    enum Mode mode
) {
    struct Text *line;
    size_t i;
    size_t screen_pos;
    size_t line_len = 0;
    char msg[80] = {0};
    char *str = NULL;
    memset(msg, ' ', 79);
    term_home();
    for (i = 0, line = cur->top_of_screen; line; line = line->next, i++) {
        if (!line || !line->data) {
            break;
        }

        term_move(i, 0);
        str = line->data;
        while (*str++) {
            if (*str == '\r') {
                *str = '\n';
                str++;
                *str = '\0';
            }
        }
        line->len = strlen(line->data);
        term_puts(line->data);
        term_clrtoeol();

        if (i >= (win->maxlines - 2)) {
            break;
        }
    }

    /* draw '~' when no lines exist at end of file */
    if (!line) {
        for (; i < (win->maxlines - 1); i++) {
            term_puts("~\n");
        }
    }

    term_move(win->maxlines - 1, 0);
    term_clrtoeol();

    switch (mode) {
        case INSERT:
            term_move(win->maxlines - 1, 0);
            term_puts(msg);
            term_move(win->maxlines - 1, 0);
            term_puts("-- INSERT --");
            break;

        case EX:
        case QUIT:
        case NORMAL:
        case SEARCH:
        default:
            break;
    }

    term_move(win->maxlines - 1, 55);
    sprintf(msg, "%lu - %lu", cur->x + 1, cur->line_no);
    term_puts(msg);
    /* count tabs to the left of the cursor, and add 8 spaces per tab */
    /* in the other modes the cursor is on the status line, not in the text */
    if ((mode == NORMAL || mode == INSERT) && cur->line && cur->line->data) {
        line_len = strlen(cur->line->data);
    }
    screen_pos = 0;
    for (i = 0; i <= cur->x; i++) {
        if (i < line_len && cur->line->data[i] == '\t') {
            screen_pos += 8;
        } else {
            screen_pos++;
        }
    }

    term_move(win->maxlines - 1, 0);
    term_puts(cur->buf);
    term_move(win->maxlines - 1, 0);

    term_move(cur->y, screen_pos - 1);
    term_refresh();
}

static enum Todo handle_normal_mode(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode,
    int c,
    struct Command *cmd
);

static enum Todo handle_ex_mode(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode,
    char *filename,
    int c
) {
    char buf[80] = {0};
    size_t buf_index = 0;
    size_t new_l = 0;
    size_t i;
    size_t difference;
    char *p;
    int do_write = 0;

    do {
        if (cur->buf_idx < 79) {
            cur->buf[cur->buf_idx++] = c;
        }
        redraw_screen(win, cur, *mode);
        wputchar(cur, c);
        switch (c) {
            case 27: /* escape key */
                *mode = NORMAL;
                do_write = 0;
                term_move(win->maxlines - 1, 0);
                term_puts(blank);
                cur->x = cur->old_x;
                cur->y = cur->old_y;
                cur->buf[cur->buf_idx] = '0';
                cur->buf_idx = 0;
                memset(cur->buf, 0, 80);
                goto leave_ex;

            case '\n':
                if (*mode == QUIT) {
                    goto leave_ex;
                }
                /* put the cursor back in the text before jumping, so that
                 * 'j' and 'k' move and scroll from the real cursor row */
                cur->x = cur->old_x;
                cur->y = cur->old_y;
                new_l = strtol(buf, &p, 10);
                if (buf_index > 0 && *p == 0) {
                    if (new_l > cur->line_no) {
                        difference = new_l - cur->line_no;
                        for (i = 0; i < difference && cur->line->next; i++) {
                            handle_normal_mode(win, cur, mode, 'j', NULL);
                        }
                        handle_normal_mode(win, cur, mode, '0', NULL);

                    } else if (new_l < cur->line_no) {
                        difference = cur->line_no - new_l;
                        for (i = 0; i < difference && cur->line->prev; i++) {
                            handle_normal_mode(win, cur, mode, 'k', NULL);
                        }
                        handle_normal_mode(win, cur, mode, '0', NULL);
                    }

                }
                *mode = NORMAL;
                term_move(win->maxlines - 1, 0);
                term_puts(blank);

                cur->buf[cur->buf_idx] = '0';
                cur->buf_idx = 0;
                memset(cur->buf, 0, 80);
                goto leave_ex;

            case 'q':
                *mode = QUIT;
                break;

            case 'w':
                /* write out */
                do_write = 1;
                break;

            default:
                if (buf_index < sizeof(buf) - 1) {
                    buf[buf_index++] = c;
                }
                break;
        }
    } while ((c = term_getch()));
leave_ex:
    if (do_write) {
        char msg[1024];
        int wrote = 0;
        if (filename != NULL) {
            if (text_write(cur->top_of_text, filename) == 0) {
                wrote = 1;
                sprintf(msg, "wrote file: '%.900s'", filename);
            } else {
                sprintf(msg, "failed to write file: '%.900s'", filename);
            }
        } else {
            sprintf(msg, "no file open");
        }
        if (!wrote && *mode == QUIT) {
            /* don't quit and throw away text that could not be saved */
            *mode = NORMAL;
            cur->x = cur->old_x;
            cur->y = cur->old_y;
            cur->buf_idx = 0;
            memset(cur->buf, 0, 80);
        }
        FLASH_MSG(msg);
        term_getch();
    }
    if (*mode == QUIT) {
        return TERMINATE;
    }

    return GET_CHAR;
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
            if (cur->x > 0) {
                cur->x--;
            }
            break;

        case 127: /* backspace key */
            if (cur->x > 0) {
                cur->x--;
                if (cur->line && cur->line->len > 0) {
                    cur->line->len--;
                }
                text_backspace(cur->line, cur->x);
            }
            break;

        case '\n':
            cur->line = text_split_line(cur->line, cur->x);
            cur->line_no++;
            cursor_row_down(win, cur);
            cur->x = 0;
            break;

        case '\t':
            text_insert_char(cur->line, cur->x, '\t');
            cur->x++;
            break;

        default:
            text_insert_char(cur->line, cur->x, c);
            cursor_advance(cur);
            term_move(cur->y, 0);
            term_puts(cur->line->data);
            term_move(cur->y, cur->x);
    }
}

static enum Todo handle_normal_mode(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode,
    int c,
    struct Command *cmd
) {
    size_t pos;
    char msg_buf[80];
    enum Todo todo = GET_CHAR;
    cur->line->len = strlen(cur->line->data);
    switch (c) {
        case 27: /* escape key */
            memset(cmd, 0, 80);
            break;

        case 'k':
            if (cur->line && cur->line->prev) {
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
                    if (cur->top_of_screen && cur->top_of_screen->prev) {
                        cur->top_of_screen = cur->top_of_screen->prev;
                    }
                }
            }
            break;

        case 'u': {
            /* the snapshot only applies to the line it was taken from */
            if (!cur->before || cur->before_line != cur->line) {
                break;
            } else {
                char *tmp = cur->line->data;
                cur->line->data = cur->before;
                cur->before = tmp;
                cur->line->len = strlen(cur->line->data);
                cur->line->capacity = cur->line->len;
                cur->x = 0;
            }
            break;
        }

        case '\n':
        case 'j':
            if (cur->line && cur->line->next) {
                cur->line_no++;
                cur->line = cur->line->next;
                if (cur->line->len > 2) {
                    pos = cur->line->len - 2;
                } else {
                    pos = 0;
                }

                cur->old_x = MAX(cur->x, cur->old_x);
                cur->x = MIN(cur->old_x, pos);
                cursor_row_down(win, cur);
            }
            break;

        case ' ':
        case 'l':
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
            set_clipboard(cur);
            if ((cur->x < cur->line->len)
                    && (cur->line->data[cur->x] != '\n')) {
                text_shift_left(cur->line, cur->x);
            }
            break;

        case '/':
            *mode = SEARCH;
            memset(cur->buf, 0, 80);
            cur->buf[0] = '/';
            cur->buf_idx = 1;
            cur->old_x = cur->x;
            cur->old_y = cur->y;
            cur->x = 0;
            cur->y = win->maxlines - 1;
            cur->x++;
            term_move(win->maxlines - 1, 0);
            term_puts(blank);
            term_move(win->maxlines - 1, 0);
            FLASH_MSG(cur->buf);
            while ((c = term_getch())) {
                if ((c == '\n') || (c == 27)) {
                    break;
                }
                cur->buf[cur->buf_idx++] = c;
                FLASH_MSG(cur->buf);
                /* leave room for the terminating '\0' */
                if (cur->buf_idx >= 79) {
                    break;
                }
                cur->x++;
                term_move(cur->y, cur->x);
            }
            cur->x = cur->old_x;
            cur->y = cur->old_y;
            if (c != 27) {
                todo = DONT_GET_CHAR;
            } else {
                cur->buf_idx = 0;
                memset(cur->buf, 0, 80);
            }

            break;

        case 'n':
            handle_search_mode(win, cur, mode);
            break;

        case 'r':
            if ((cur->x < cur->line->len)
                    && (cur->line->data[cur->x] != '\n')) {
                cur->line->data[cur->x] = term_getch();
            }
            break;

        case '~':
            if (cur->x < cur->line->len) {
                char *under_cursor = &cur->line->data[cur->x];
                if (isalpha((unsigned char)*under_cursor)) {
                    *under_cursor ^= 0x20;
                }
            }
            /* don't advance past the last character of the line */
            if (cur->line->len > 2 && cur->x < cur->line->len - 2) {
                cursor_advance(cur);
            }
            break;

        case 'y': {
            char next_cmd = term_getch();
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

        case 'w':
            c = cur->line->data[cur->x];
            if ((cur->line->data[cur->x] == '\n') ||
                (cur->line->data[cur->x + 1] == '\n')
            ) {
                if (!cur->line->next) {
                    break;
                }
                /* go to the first word of the next line */
                handle_normal_mode(win, cur, mode, '0', cmd);
                handle_normal_mode(win, cur, mode, 'j', cmd);
                c = cur->line->data[cur->x];
            } else {
                while (c != ' ' && c != '\n' && c != '\0') {
                    c = cur->line->data[++cur->x];
                }
            }
            while (c == ' ' && c != '\n' && c != '\0') {
                c = cur->line->data[++cur->x];
            }
            if ((cur->x > 0) && ((c == '\n') || (c == '\0'))) {
                cur->x--;
            }
            break;

        case 'p': {
            if (cur->clipboard) {
                struct Text *line = text_copy_line(cur->clipboard);
                text_insert_line(cur->line, line, cur->line->next);
            }
            break;
        }

        case 'd': {
            char next_c = term_getch();
            switch (next_c) {
                case 'd':
del_line:
                    delete_line(cur);
                    cmd->len = 0;
                    memset(cmd, 0, 80);
                    break;

                case 'w': {
                    char was_on_space = 0;
                    char *data = cur->line->data;
                    set_clipboard(cur);
                    if (data[cur->x] == '\n') {
                        goto del_line;
                    }
                    text_shift_left(cur->line, cur->x);
                    while (data[cur->x] == ' ' ||  data[cur->x] == '\t') {
                        text_shift_left(cur->line, cur->x);
                        was_on_space = 1;
                    }
                    if (!was_on_space) {
                        for (;
                            isalnum(data[cur->x]) ||
                            data[cur->x] == '_';
                        ) {
                            text_shift_left(cur->line, cur->x);
                        }
                    }
                    break;
                }
            }
            break;
        }

        case 'D': {
            if (cur->x < cur->line->len) {
                cur->line->data[cur->x] = '\n';
                cur->line->data[cur->x + 1] = '\0';
                cur->line->len = cur->x + 1;
            }
            if (cur->x > 0) {
                cur->x--;
            }
            break;
        }

        case '$':
        case 'E':
            if (cur->line->len > 2) {
                pos = cur->line->len - 2;
            } else {
                pos = 0;
            }
            cur->old_x = SIZE_MAX;
            cur->x = pos;
            break;

        case 'O': {
            struct Text *new_line = text_make_line();
            *mode = INSERT;
            set_clipboard(cur);
            text_insert_line(cur->line->prev, new_line, cur->line);
            if (cur->top_of_text == cur->line) {
                cur->top_of_text = new_line;
            }
            if (cur->top_of_screen == cur->line) {
                cur->top_of_screen = new_line;
            }
            cur->x = 0;
            cur->line = new_line;
            break;
        }

        case 'o': {
            struct Text *new_line = text_make_line();
            *mode = INSERT;
            set_clipboard(cur);

            cursor_row_down(win, cur);
            cur->line_no++;
            cur->x = 0;

            text_insert_line(cur->line, new_line, cur->line->next);

            cur->line = new_line;
            text_push_char(cur->line, '\n');

            break;
        }

        case 'i':
            memset(cur->buf, 0, 80);
            cur->buf_idx = 0;
            *mode = INSERT;
            set_clipboard(cur);
            term_move(cur->y, cur->x);
            break;

        case 'g': {
            char next_c = term_getch();
            switch (next_c) {
                case 'g':
                    cur->x = 0;
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
            cur->y = 0;
            cur->line_no = 1;
            cur->line = cur->top_of_text;
            cur->top_of_screen = cur->top_of_text;
            cur->x = 0;
            for (; cur->line && cur->line->next; cur->line = cur->line->next) {
                cur->line_no++;
            }
            cur->top_of_screen = cur->line;
            break;

        case 'a':
            set_clipboard(cur);
            *mode = INSERT;
            /* never step past the newline at the end of the line */
            if ((cur->line->data[cur->x] != '\n')
                    && (cur->line->data[cur->x] != '\0')) {
                cursor_advance(cur);
            }
            term_move(cur->y, cur->x);
            break;

        case 'A':
            set_clipboard(cur);
            *mode = INSERT;
            cur->x = cur->line->len;
            if ((cur->x > 0) && (cur->line->data[cur->x - 1] == '\n')) {
                cur->x--;
            }
            break;

        case '0':
            cur->old_x = 0;
            cur->x = 0;
            break;

        case ':':
            *mode = EX;
            memset(cur->buf, 0, 80);
            cur->buf[0] = ':';
            cur->buf_idx = 1;
            cur->old_x = cur->x;
            cur->old_y = cur->y;
            cur->x = 0;
            cur->y = win->maxlines - 1;
            term_move(win->maxlines - 1, 0);
            term_puts(blank);
            term_move(win->maxlines - 1, 0);
            wputchar(cur, c);
            break;

        case '\f':
            redraw_screen(win, cur, *mode);
            break;

        default:
            term_move(win->maxlines - 1, 0);
            switch (c) {
                case 27:
                    break;

                default:
                    sprintf(msg_buf, "not an editor command: %c", c);
                    term_puts(msg_buf);
                    term_move(cur->y, cur->x);
            }
            break;
    }
    return todo;
}

static long get_index_in_str(const char *line, const char *search_term) {
    char *str;
    if ((str = strstr(line, search_term))) {
        return str - line;
    } else {
        return -1;
    }
}

static void handle_search_mode(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode
) {
    struct Text *line = cur->line->next;
    long index = 0;
    size_t line_no = cur->line_no;

    *mode = NORMAL;

    FLASH_MSG(cur->buf);
    while (line) {
        index = get_index_in_str(line->data, cur->buf + 1);
        line_no++;
        if (index >= 0) {
            cur->x = index;
            cur->line = line;
            cur->line_no = line_no;
            cur->top_of_screen = cur->line;
            cur->y = 0;
            break;
        } else {
            line = line->next;
            continue;
        }
    }

    if (!line) {
        char buf[128];
        sprintf(buf, "'%.80s': not found", cur->buf + 1);
        FLASH_MSG(buf);
        term_getch();
    }
}

static enum Todo handle_input(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode,
    int c,
    struct Command *cmd,
    char *filename
) {
    enum Todo todo = GET_CHAR;
    switch (*mode) {
        case NORMAL:
            todo = handle_normal_mode(win, cur, mode, c, cmd);
            term_size(&win->maxlines, &win->maxcols);
            term_move(cur->y, cur->x);
            term_refresh();
            break;

        case INSERT:
            cur->line->len = strlen(cur->line->data);
            handle_insert_mode(win, cur, mode, c);
            break;

        case EX:
            todo = handle_ex_mode(win, cur, mode, filename, c);
            break;

        case SEARCH:
            handle_search_mode(win, cur, mode);
            break;

        case QUIT:
            return TERMINATE;
    }

    term_size(&win->maxlines, &win->maxcols);
    term_move(cur->y, cur->x);
    term_refresh();
    return todo;
}

static int event_loop(
    struct Window *win,
    struct Cursor *cur,
    char *filename
) {
    int c = '\n';
    enum Todo todo = GET_CHAR;
    enum Mode mode = NORMAL;
    struct Command cmd;
    cur->x = 0;
    cur->y = 0;
    cmd.len = 0;
    memset(&cmd.data, 0, 80);
    redraw_screen(win, cur, mode);
    while (1) {
        if (todo == GET_CHAR) {
            c = term_getch();
            if (c < 0) {
                goto quit;
            }
        }
        todo = handle_input(win, cur, &mode, c, &cmd, filename);
        switch (todo) {
            case DONT_GET_CHAR:
            case GET_CHAR:
                break;
            case TERMINATE:
                goto quit;
        }
        redraw_screen(win, cur, mode);
    }
quit:
    return 1;
}

int main(int argc, char **argv) {
    struct Window win;
    FILE *fp = NULL;
    char *filename = NULL;
    struct Cursor cur;
    struct Text *line;
    struct Text *next;

    signal(SIGINT, sigint_handler);

    cur.x = 0;
    cur.old_x = 0;
    cur.y = 0;
    cur.old_y = 0;
    cur.line = text_make_line();
    cur.top_of_text = cur.line;
    cur.clipboard = NULL;
    cur.line_no = 1;
    cur.buf = calloc(1, 80);
    cur.buf_idx = 0;
    cur.before = NULL;
    cur.before_line = NULL;

    /* setup the terminal */
    term_init();
    term_size(&win.maxlines, &win.maxcols);

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
    event_loop(&win, &cur, filename);

    line = cur.top_of_text;
    while (line) {
        next = line->next;
        free(line->data);
        free(line); 
        line = next;
    }

    if (cur.clipboard) {
        free(cur.clipboard->data);
        free(cur.clipboard);
    }
    free(cur.buf);
    free(cur.before);

    /* restore the terminal */
    term_exit();

    return 0;
}
