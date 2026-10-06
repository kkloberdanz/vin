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

#ifndef TERM_H
#define TERM_H

#include <stddef.h>

void term_init(void);
void term_exit(void);
void term_size(size_t *rows, size_t *cols);
void term_move(size_t y, size_t x);
void term_home(void);
void term_clrtoeol(void);
void term_puts(const char *s);
void term_putc(int c);
void term_refresh(void);
int term_getch(void);

#endif /* TERM_H */
