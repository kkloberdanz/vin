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

void cursor_advance(struct Cursor *cur) {
    cur->x++;
}

void wputchar(struct Window *win, struct Cursor *cur, int c) {
    waddch(win->curses_win, c);
    cursor_advance(cur);
}

void handle_ex_mode(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode,
    int c
) {
    wputchar(win, cur, c);
    switch (c) {
        case 27: /* escape key */
            *mode = NORMAL;
            break;

        case '\n':
            *mode = NORMAL;
            break;

        case 'q':
            *mode = QUIT;
            break;

        default:
            break;
    }
}


void handle_insert_mode(
    struct Window *win,
    struct Cursor *cur,
    enum Mode *mode,
    int c
) {
    switch (c) {
        case 27: /* escape key */
            *mode = NORMAL;
            break;

        case 127:
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

void handle_normal_mode(
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

        case 'i':
            *mode = INSERT;
            break;

        case '0':
            cur->x = 0;
            break;

        case ':':
            *mode = EX;
            cur->y = win->maxlines - 1;
            cur->x = win->maxcols - 1;
            wputchar(win, cur, c);
            break;

        default:
            break;
    }
}

int event_loop(struct Window *win, struct Cursor *cur) {
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
