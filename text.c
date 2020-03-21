#include <stdlib.h>

#include "text.h"

struct Text *text_new_line(struct Text *prev, struct Text *next) {
    struct Text *line = malloc(sizeof(struct Text));
    line->prev = prev;
    line->next = next;

    if (prev) {
        prev->next = line;
    }

    if (next) {
        next->prev = line;
    }

    line->data = calloc(2, sizeof(char));
    line->len = 0;
    line->capacity = 1;
    return line;
}

void text_insert_char(struct Text *line, char c) {
    if (line->len >= line->capacity) {
        line->capacity *= 2;
        line->data = realloc(line->data, line->capacity + 1);
        line->data[line->capacity] = '\0';
    }

    line->data[line->len++] = c;
}

void text_write(struct Text *line, FILE *fp) {
    if (!fp) {
        return;
    }
    fseek(fp, 0, SEEK_SET);
    for (; line; line = line->next) {
        fprintf(fp, "%s\n", line->data);
    }
    fflush(fp);
}

void text_backspace(struct Text *line, size_t index) {
    size_t i;
    for (i = index; line->data[i] != '\0'; i++) {
        line->data[i] = line->data[i + 1];
    }
}
