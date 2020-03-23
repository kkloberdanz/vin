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

#ifndef VIN_H
#define VIN_H

#include <stdio.h>
#include <curses.h>

#ifndef SIZE_MAX
#define SIZE_MAX sizeof(size_t)
#endif

struct Cursor {
    size_t x;
    size_t y;
    size_t old_x;
    size_t old_y;
    size_t line_no;
    struct Text *line;
    struct Text *top_of_text;
    struct Text *top_of_screen;
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
    QUIT,
    SEARCH
};

#endif /* VIN_H */
