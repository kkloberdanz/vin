#ifndef COMMAND_H
#define COMMAND_H

#include <stdio.h>

struct Command {
    size_t len;
    char data[80];
};

void command_add_char(struct Command *cmd, char c);

#endif /* COMMAND_H */
