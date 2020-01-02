/*
    Piconet RS232 ethernet interface

    config.h

    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Global configuration
*/
#ifndef _CONFIG_H_
#define _CONFIG_H_

/* Uncomment to enable code in console.c; allows display-control over the console, has nothing to do with the snake-game */
//#define DOCONSOLE

/* Clockspeed, as set in cpusettings.h (changing values here will _not_ change the clock speed!) */
#define FOSC    48000000            // CPU clock speed (do not get confused by the datasheet,
                                    // using this term for both the cpu clock and external oscillator frequency)
#define FCY     (FOSC/4)            // Instruction clock

/*! Baudrate of the UART's
     BAUDRATEx_BRG16 = 0, BAUDRATEx_BRGH = 0: BAUDRATEx = FOSC/[64 (n + 1)]
     BAUDRATEx_BRG16 = 0, BAUDRATEx_BRGH = 1: BAUDRATEx = FOSC/[16 (n + 1)]
     BAUDRATEx_BRG16 = 1, BAUDRATEx_BRGH = 0: BAUDRATEx = FOSC/[16 (n + 1)]
     BAUDRATEx_BRG16 = 1, BAUDRATEx_BRGH = 1: BAUDRATEx = FOSC/[4 (n + 1)] */
#define BAUDRATE1_BRG16 1
#define BAUDRATE1_BRGH  1
#define BAUDRATE1   103     // 115200 baud @ FOSC=48MHz
#define BAUDRATE2_BRG16 1
#define BAUDRATE2_BRGH  1
#define BAUDRATE2   1249   // 9600 baud @ FOSC=48MHz

/*! Watchdog settings. Use '0' to disable it. Values > 0 are used as the watchdog
    prescaler values. Possible values are 1, 2, 4, 8, 16, 32, 64, 128 and so on up to and including 32768 */
#define WATCHDOG    0

/*! Enable (1) or disable (0) debug output */
#define DEBUG       1

#ifdef __VISUAL_CODE
/* To make the Visual Studio Code parser happy.
   Defining these in the c_cpp_properties.json file doesn't work, as they'll be defined with a value (1); we need empty defines here! */
#define __sfr
#define __at(x)
#define __wparam
#define __interrupt //
#define low_priority
#define high_priority
#define interrupt
#undef _WIN32
#endif

#ifdef __SDCC
#include <pic18fregs.h>
#endif

#ifdef __XC8
#include <pic18.h>
#endif

#include "types.h"
#include "debug.h"
#include "../svnrev.h"  // Generated from the Makefile at compile time, defines SVN_REV

#if WATCHDOG != 0
#ifdef __SDCC
#define CLEAR_WATCHDOG()    do { __asm__ ("clrwdt"); } while(0)
#endif
#ifdef __XC8
#define CLEAR_WATCHDOG()    do { asm ("clrwdt"); } while(0)
#endif
#else
#define CLEAR_WATCHDOG()
#endif

#ifndef RESET
#ifdef __SDCC
#define RESET()    do { __asm__ ("reset"); } while(0)
#endif
#ifdef __XC8
#define RESET()    do { asm ("reset"); } while(0)
#endif
#endif

#ifndef NOP
#ifdef __SDCC
#define NOP()    do { __asm__ ("nop"); } while(0)
#endif
#ifdef __XC8
#define NOP()    do { asm ("nop"); } while(0)
#endif
#endif

unsigned int ticks(void);
void random_setkey(int key);
unsigned int random(void);

#endif /* _CONFIG_H_ */
