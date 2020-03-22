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
 * Insert a new line into the list of text
 */
struct Text *text_new_line(struct Text *prev, struct Text *next);

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
