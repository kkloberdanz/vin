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
 * writes text out to file, fp
 */
void text_write(struct Text *top_of_text, FILE *fp);

/**
 * backspaces text from the index
 */
void text_backspace(struct Text *line, size_t index);

/**
 * inserts a character to the position 'index' and pushes the rest back
 */
void text_insert_char(struct Text *line, size_t index, char c);

#endif /* TEXT_H */
