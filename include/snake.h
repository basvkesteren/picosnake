/*
    "Snake game"

    snake.h

    Copyright (c) 2019 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
#ifndef SNAKE_H
#define SNAKE_H

void snake_init(void);
void snake_tick(void);
void snake_buttons(char up, char down, char left, char right);

#endif /* SNAKE_H */