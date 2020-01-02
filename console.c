/*
    console.c
           
    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Simple line console
*/
#include "console.h"
#include "fgui.h"
#include "fgui/hw/ws2812.h"
#include "io.h"
#include <string.h>

void console_incoming(unsigned char c)
{
    static unsigned char commandlength=0;
    static char command[50];
    static char seenReturn = FALSE;

    if(c == 0x0D && seenReturn == FALSE) {
        seenReturn = TRUE;
        return;
    }

    if(seenReturn && c == 0x0A) {
        /* End-of-command. Figure out what it is */
        if(commandlength >= 1) {
            switch(command[0]) {
                /* X = horizontal, y = vertical
                 */
                case 's':
                    /* s - get size of display */
                    dprint("%2d%2d!", FGUI_SCR_W, FGUI_SCR_H);
                    break;
                case 'e':
                    /* e - Empty display */
                    fgui_clear();
                    dprint("!");
                    break;
                case 'c':
                    /* cRGBRGB - set foreground and background color (both rgb values, 8 bit/color) */
                    if(commandlength == 7) {
                        ws2812_setcolors(command[1], command[2], command[3], command[4], command[5], command[6]);
                        dprint("!");
                    }
                    else {
                        dprint("?");
                    }
                    break;
                case 'p':
                    /* pXYn - set pixel at x/y to 0 or 1 */
                    if(commandlength == 4 && command[3] == 0) {
                        fgui_pixeloff(command[1], command[2]);
                        dprint("!");
                    }
                    else if(commandlength == 4 && command[3] == 1) {
                        fgui_pixelon(command[1], command[2]);
                        dprint("!");
                    }
                    else {
                        dprint("?");
                    }
                    break;
                case 'd':
                    /* * dn - set pixel-value to draw with from now on (0 or 1), existing pixels remain as-is */
                    if(commandlength == 2 && command[1] == '0') {
                        fgui_setcolor(FGUI_WHITE);
                        dprint("!");
                    }
                    else if(commandlength == 2 && command[1] == '1') {
                        fgui_setcolor(FGUI_BLACK);
                        dprint("!");
                    }
                    else {
                        dprint("?");
                    }
                    break;
                case 'h':
                    /* hXYL - draw horizontal line starting at x/y with length l */
                    if(commandlength == 4) {
                        fgui_lineh(command[1], command[2], command[3]);
                        dprint("!");
                    }
                    else {
                        dprint("?");
                    }
                    break;
                case 'v':
                    /* vXYL - draw vertical line starting at x/y with length l */
                    if(commandlength == 4) {
                        fgui_linev(command[1], command[2], command[3]);
                        dprint("!");
                    }
                    else {
                        dprint("?");
                    }
                    break;
                case 'l':
                    /* lXYXY - draw line from x/y to x/y */
                    if(commandlength == 5) {
                        fgui_line(command[1], command[2], command[3], command[4]);
                        dprint("!");
                    }
                    else {
                        dprint("?");
                    }
                    break;
                case 't':
                    /* TXYn - text at location x/y */
                    if(commandlength >= 4) {
                        fgui_text(command[1], command[2], &command[3]);
                        dprint("!");
                    }
                    else {
                        dprint("?");
                    }
                    break;
                case 'i':
                    /* iXYn - image at location x/y */
                    if(commandlength >= 4) {
                        
                    }
                    else {
                        dprint("?");
                    }
                    break;
                case 'u':
                    /* u - update display */
	                rts_off();
                    fgui_refresh();
                    rts_on();
                    dprint("!");
                    break;
                case 'r':
                    RESET();
                    break;
                default:
                    printf("?");
                    break;
            }
        }
        else {
            printf("?");
        }
        commandlength = 0;
    }
    else if(commandlength == sizeof(command)) {
        printf("?");
        commandlength = 0;
    }
    else {
        command[commandlength] = c;
        commandlength++;
    }
    
    seenReturn = FALSE;
}