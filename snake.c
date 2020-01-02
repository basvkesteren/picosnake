/*
    "Snake game"

    snake.c

    Copyright (c) 2019 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
#include "fgui.h"
#include "fgui/hw/ws2812.h"
#include "delay.h"
#include "serial.h"
#include "config.h"

#define X_MAX       23      // Screen width
#define Y_MAX       19      // Screen height
#define SNAKE_MAX   250     // Max snake length
#define SNAKE_SPEED 5       // Speed, 100ms steps

char snake_alive;

typedef struct {
    void *next, *previous;
    signed char x,y;
} snake_pixel_t;

snake_pixel_t snake_pixels[SNAKE_MAX];
snake_pixel_t *snake_begin, *snake_end;

unsigned int snake_length;
unsigned char snake_speed;
unsigned char snake_direction;
unsigned int snake_livetime;

#define STOP        0
#define UP          1
#define DOWN        2
#define LEFT        3
#define RIGHT       4

struct {
    signed char x,y;
} dot;

void snake_init()
/*!
  Initialisation, call before calling any other snake_ function
 */
{
    snake_alive = 0;
}

static void snake_movedot()
/*!
  Create a new dot
 */
{
    snake_pixel_t *snake_p;

    while(1) {
        /* Pick a random location */
        dot.x = random()%X_MAX;
        dot.y = random()%Y_MAX;

        /* Make sure the picked location doesn't collide with the snake */
        snake_p = snake_begin;
        do {
            if(snake_p->x == dot.x && snake_p->y == dot.y) {
                break;
            }
            snake_p = snake_p->previous;
        } while(snake_p);

        if(snake_p == NULL) {
            /* Found an empty spot! */
            break;
        }
    }
}

void snake_tick()
/*!
  Called periodically (every 100ms)
 */
{
    static int startup=(7*8);
    static char prescaler=0;
    static char prescaler_seconds=0;
    snake_pixel_t *snake_p;
    unsigned int i;
    char collision=0;

    if(startup) {
        /* Startup animation */
        fgui_text(startup-(7*8),5,"Snake;)");
        startup--;
        if(startup == 0) {
            /* Startup done, setup game */  
            for(i=0;i<SNAKE_MAX;i++) {
                snake_pixels[i].x = -1;
                snake_pixels[i].y = -1;
                snake_pixels[i].next = NULL;
                snake_pixels[i].previous = NULL;
            }

            /* Create a new snake with a length of 1 */
            snake_begin = &snake_pixels[0];
            snake_begin->x = X_MAX/2;
            snake_begin->y = Y_MAX/2;
            snake_end = snake_begin;
            snake_length = 1;

            /* Not moving */
            snake_direction = STOP;
            snake_speed = SNAKE_SPEED;

            /* There is no dot */
            dot.x = -1;
            dot.y = -1;

            snake_alive = 1;
            fgui_pixelon(snake_begin->x, snake_begin->y);
        }
        fgui_refresh();
    }

    if(snake_alive == 0) {
        return;
    }

    if(snake_direction != STOP && dot.x == -1) {
        /* Snake started moving, draw first dot */
        snake_movedot();
        fgui_pixelon(dot.x, dot.y);

        /* Start livetime counter */
        snake_livetime = 0;
        prescaler_seconds = 0;
    }

    prescaler_seconds++;
    if(prescaler_seconds == 10) {
        prescaler_seconds = 0;
        snake_livetime++;
    }

    prescaler++;
    if(prescaler >= snake_speed) {
        prescaler = 0;
        
        ws2812_setcolors(16, 0, 0, 0, 0, 1);

        if(snake_length == SNAKE_MAX) {
            /* Snake has maximum length; remove a dot from the end */
            fgui_pixeloff(snake_end->x, snake_end->y);
            snake_end->x = -1;
            snake_end->y = -1;
            snake_end = snake_end->next;
            snake_end->previous = NULL;
            snake_length--;
        }

        /* Increase snake-length by one; loop through 'snake_pixels' untill an empty entry is found,
           use that as the new beginning of the snake */
        for(i=0;i<SNAKE_MAX;i++) {
            if(snake_pixels[i].x == -1) {
                /* This 'snake_pixels' entry is empty. Add it to the beginning of the snake,
                   for now with x/y of the current beginning */
                snake_pixels[i].x = snake_begin->x;
                snake_pixels[i].y = snake_begin->y;
                snake_pixels[i].previous = snake_begin;
                snake_begin->next = &snake_pixels[i];
                snake_begin = &snake_pixels[i];
                snake_length++;

                /* Update x/y of new snake beginning according to direction of snake */
                switch(snake_direction) {
                    case UP:
                        if(snake_begin->y) {
                            snake_begin->y--;
                        }
                        else {
                            /* whoops, snake hit the wall */
                            collision=1;
                        }
                        break;
                    case DOWN:
                        if(snake_begin->y < Y_MAX) {
                            snake_begin->y++;
                        }
                        else {
                            /* whoops, snake hit the wall */
                            collision=1;
                        }
                        break;
                    case LEFT:
                        if(snake_begin->x) {
                            snake_begin->x--;
                        }
                        else {
                            /* whoops, snake hit the wall */
                            collision=1;
                        }
                        break;
                    case RIGHT:
                        if(snake_begin->x < X_MAX) {
                            snake_begin->x++;
                        }
                        else {
                            /* whoops, snake hit the wall */
                            collision=1;
                        }
                        break;
                }
                
                if(snake_begin->x != dot.x || snake_begin->y != dot.y) {
                    /* If snake did not hit the dot, remove a pixel from the end of the snake */
                    fgui_pixeloff(snake_end->x, snake_end->y);
                    snake_end->x = -1;
                    snake_end->y = -1;
                    snake_end = snake_end->next;
                    snake_end->previous = NULL;
                    snake_length--;
                }
                break;
            }
        }
        
        if(snake_begin->x == dot.x && snake_begin->y == dot.y) {
            /* Snake has hit the dot! */
            ws2812_setcolors(0, 16, 0, 0, 0, 1);

            /* Create a new dot */
            snake_movedot();
            fgui_pixelon(dot.x, dot.y);

            /* Reset speed */
            snake_speed = SNAKE_SPEED;
        }
        else {
            /* Make sure snake isn't colliding with it's own tail */
            snake_p = snake_begin->previous;
            while(snake_p && collision == 0) {
                if(snake_begin->x == snake_p->x && snake_begin->y == snake_p->y) {
                    collision = 1;
                }
                snake_p = snake_p->previous;
            }
        }

        /* Did a collision occur? */
        if(collision) {
            /* Dump length and livetime on the console */
            serial1_putchar('l');
            serial1_putchar(snake_length>>8);
            serial1_putchar(snake_length);
            serial1_putchar('t');
            serial1_putchar(snake_livetime>>8);
            serial1_putchar(snake_livetime);
            serial1_putchar('\n');
            serial1_putchar('\r');

            ws2812_setcolors(0, 0, 16, 0, 1, 0);
            for(i=0;i<(4*8);i++) {
                fgui_text(0-i,0, "GAME");
                fgui_text(0-i,10, "OVER");
                fgui_refresh();
                if(i==0) {
                    delay_s(1);
                }
                delay_ms(100);
            }

            /* Restart game */
            ws2812_setcolors(16, 0, 0, 0, 0, 1);
            snake_alive = 0;
            startup=(7*8);
        }
        else {
            /* Draw new beginning of snake */
            fgui_pixelon(snake_begin->x, snake_begin->y);

            fgui_refresh();
        }
    }
}

void snake_buttons(char up, char down, char left, char right)
/*!
  Called when a button is pressed or released
 */
{
    unsigned char direction = STOP;

    if(snake_alive == 0) {
        return;
    }

    if(up) {
        direction = UP;
    }
    else if(down) {
        direction = DOWN;
    }
    else if(left) {
        direction = LEFT;
    }
    else if(right) {
        direction = RIGHT;
    }
    else {
        return;
    }

    if(direction == snake_direction) {
        /* Already moving in requested direction, increase speed */
        if(snake_speed) {
            snake_speed--;
        }
    }
    snake_direction = direction;
}
