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
    INSERT
};

void cursor_advance(struct Cursor *cur) {
    cur->x++;
}

void wputchar(struct Window *win, struct Cursor *cur, int c) {
    waddch(win->curses_win, c);
    cursor_advance(cur);
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

        default:
            fprintf(stderr, "not an editor command: '%c'\n", c);
            break;
    }
}

int main(int argc, char **argv) {
    struct Window win;
    FILE *fp = NULL;
    int c;
    struct Cursor cur;
    enum Mode mode = NORMAL;

    cur.x = 0;
    cur.y = 0;

    win.maxlines = LINES;
    win.maxcols = COLS;

    /* setup curses */
    initscr();
    cbreak();
    noecho();

    clear();

    /*mvaddch(y[0], x[0], '0');*/

    if (argc == 2) {
        fp = fopen(argv[1], "rw");
    }

    win.curses_win = newwin(win.maxlines, win.maxcols, cur.x, cur.y);
    while ((c = wgetch(win.curses_win))) {
        switch (mode) {
            case NORMAL:
                handle_normal_mode(&win, &cur, &mode, c);
                break;

            case INSERT:
                handle_insert_mode(&win, &cur, &mode, c);
                break;
        }
        wmove(win.curses_win, cur.y, cur.x);
        wrefresh(win.curses_win);
    }
    clrtoeol();
    refresh();
    endwin();
    return 0;


    if (fp) {
        fclose(fp);
    }

    /* exit curses */
    refresh();
    endwin();

    return 0;
}
