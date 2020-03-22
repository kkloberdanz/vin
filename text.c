#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "text.h"

ssize_t getline(char **lineptr, size_t *n, FILE *stream);

char *strdup(const char *s);

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

void text_push_char(struct Text *line, char c) {
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
    fp = freopen(NULL, "w", fp);
    for (; line; line = line->next) {
        fprintf(fp, "%s", line->data);
    }
    fflush(fp);
}

void text_backspace(struct Text *line, size_t index) {
    size_t i;
    for (i = index; line->data[i] != '\0'; i++) {
        line->data[i] = line->data[i + 1];
    }
}

void text_insert_char(struct Text *line, size_t index, char c) {
    size_t i;
    line->len++;

    if (line->len >= line->capacity) {
        line->capacity *= 2;
        line->data = realloc(line->data, line->capacity + 1);
        line->data[line->capacity] = '\0';
    }
    for (i = line->len - 1; i > index; i--) {
        line->data[i] = line->data[i - 1];
    }
    line->data[index] = c;
}

void text_shift_left(struct Text *line, size_t index) {
    size_t i;
    line->data[line->len] = '\0';
    line->len--;

    for (i = index; line->data[i] != '\0'; i++) {
        line->data[i] = line->data[i + 1];
    }
}

void text_read_from_file(struct Text *line, FILE *fp) {
    char *line_of_input = NULL;
    size_t n = 0;
    long tell = ftell(fp);
    while ((getline(&line_of_input, &n, fp) != -1)) {
        text_new_line(line, NULL);
        line->data = strdup(line_of_input);
        line->len = strlen(line->data);
        line->capacity = line->len;
        line = line->next;
    }
    free(line_of_input);
    fseek(fp, tell, SEEK_SET);
}
