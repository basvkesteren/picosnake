/*
    Piconet RS232 ethernet interface

    io.h

    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
I/O function definitions and macro's
*/
#ifndef IO_H
#define IO_H

#include "config.h"

/*
 * Status LED
 */

#define led_on()                do { \
                                    LATAbits.LATA0 = 1; \
                                } while(0);

#define led_off()               do { \
                                    LATAbits.LATA0 = 0; \
                                } while(0);

/*
 * Buttons
 */

#define buttons_state()         (PORTB&0x0F)

#define button1_state()         PORTBbits.RB0

#define button2_state()         PORTBbits.RB1

#define button3_state()         PORTBbits.RB2

#define button4_state()         PORTBbits.RB3

/*
 * Flowcontrol
 */

#define rts_on()               do { \
                                    LATCbits.LATC1 = 0; \
                                } while(0);

#define rts_off()               do { \
                                    LATCbits.LATC1 = 1; \
                                } while(0);

#define cts_state()             PORTCbits.RC2

#endif /* IO_H */
