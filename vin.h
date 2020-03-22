#ifndef VIN_H
#define VIN_H

#include <stdio.h>
#include <curses.h>

struct Cursor {
    size_t x;
    size_t y;
    size_t old_x;
    size_t old_y;
    size_t total_lines;
    struct Text *line;
    struct Text *top_of_text;
    struct Text *clipboard;
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

#endif /* VIN_H */
