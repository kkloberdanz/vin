#include <stdio.h>
#include <curses.h>

int main(int argc, char **argv) {
    WINDOW *window;
    FILE *fp = NULL;
    size_t maxlines;
    size_t maxcols;
    size_t x = 0;
    size_t y = 0;
    int c;

    /* setup curses */
    initscr();
    cbreak();
    noecho();

    clear();

    maxlines = LINES;
    maxcols = COLS;

    /*mvaddch(y[0], x[0], '0');*/

    if (argc == 2) {
        fp = fopen(argv[1], "rw");
    }

    window = newwin(maxlines, maxcols, x, y);
    while ((c = wgetch(window))) {
        switch(c) {
            case 'k':
                if (y > 0) {
                    y--;
                }
                break;

            case 'j':
                if (y < maxlines - 1) {
                    y++;
                }
                break;

            case 'l':
                if (x < maxcols - 1) {
                    x++;
                }
                break;

            case 'h':
                if (x > 0) {
                    x--;
                }
                break;

            case 10:
                break;

            default:
                break;
        }
        /*
        fprintf(stderr, "key: %d, %lu, %lu\n", c, x, y);
        */
        wmove(window, y, x);
        wrefresh(window);
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
