#include "command.h"

void command_add_char(struct Command *cmd, char c) {
    if (cmd->len <= 79) {
        cmd->data[cmd->len++] = c;
    }
}
