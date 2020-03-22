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

#ifndef TEXT_H
#define TEXT_H

#include <stdio.h>

struct Text {
    char *data;
    size_t len;
    size_t capacity;
    struct Text *prev;
    struct Text *next;
};

/**
 * create a new empty line
 */
struct Text *text_make_line();

/**
 * Inserts a single character to the end of the current line
 */
void text_push_char(struct Text *line, char c);

/**
 * writes text out to file
 */
void text_write(struct Text *line, char *filename);

/**
 * backspaces text from the index
 */
void text_backspace(struct Text *line, size_t index);

/**
 * inserts a character to the position 'index' and pushes the rest back
 */
void text_insert_char(struct Text *line, size_t index, char c);

/**
 * shifts text to the left starting from index
 */
void text_shift_left(struct Text *line, size_t index);

/**
 * Load a file into a text struct
 */
void text_read_from_file(struct Text *line, FILE *fp);

/**
 * split a line of text into 2 lines starting from index
 */
struct Text *text_split_line(struct Text *line, size_t index);

/**
 * make a copy of a line of text
 */
struct Text *text_copy_line(struct Text *line);

/**
 * insert the line current between 2 lines
 */
void text_insert_line(
    struct Text *prev,
    struct Text *current,
    struct Text *next
);

size_t text_total_lines(struct Text *top_line);

#endif /* TEXT_H */
