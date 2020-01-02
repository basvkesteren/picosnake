/*
    main.c

    Copyright (c) 2018 Bastiaan van Kesteren <bas@edeation.nl>
    This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
    This program is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
/*!
\file
Main code
*/
#include "cpusettings.h"
#include "config.h"
#include "io.h"
#include "delay.h"
#include "serial.h"
#include "console.h"
#include <string.h>
#include "snake.h"

#ifdef __SDCC
#include <pic16/signal.h>
#include <pic16/string.h>
#endif

#include <stdlib.h>

#include "fgui.h"
#include "font.h"

unsigned char framebuffer[FGUI_FBSIZE];

unsigned char buttons;

/* Uptime in minutes, increased in the RTC minute interrupt handler */
unsigned long system_uptime;

/* Random number generation */
unsigned int randomkey;

#ifdef DOCONSOLE
/* Incoming, unparsed, UART data (console) */
#define UART_RAWDATASIZE   50
unsigned char uart_rawdata[UART_RAWDATASIZE];
unsigned char uart_pointer, uart_pointer_handled;
#endif

static inline void init(void)
/*!
  Setup oscillator and I/O
*/
{
    /* Select 8MHz for the internal clock */
    OSCCONbits.IRCF0 = 1;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF2 = 1;
    /* Use primary clock source */
    OSCCONbits.SCS0 = 0;
    OSCCONbits.SCS1 = 0;

    #if defined(__SDCC_PIC18F27J53) || defined (__18F27J53)
    /* Wait until INTOSC is stable */
    while (!OSCCONbits.FLTS);
    #endif

    /* Setup PLL */
    OSCTUNEbits.PLLEN = 1;
    delay_ms(2);                // Wait for Trc, PLL start-up/lock time

    #if WATCHDOG != 0
    #warning Watchdog enabled
    WDTCONbits.SWDTEN = 1;
    #endif

    ANCON0 = 0xFF;              // All AD inputs..
    ANCON1 = 0x1F;              // ..as digital I/O

    UCONbits.USBEN = 0;	        // Disable USB module
    UCFGbits.UTRDIS = 1;        // Disable USB transceiver

    /* PORT A I/O usage:
        0       o       Status LED
        1       o       LED_DOUT
        2       i
        3       i
        4       i       - (not available)
        5       i
        6       i
        7       i        */
    LATA = 0x00;
    TRISA = 0xFC;               // Set = input, clear = output

    /* PORT B I/O usage:
        0       i       Button 1
        1       i       Button 2
        2       i       Button 3
        3       i       Button 4
        4       i
        5       i
        6       i
        7       i        */
    LATB = 0x00;
    TRISB = 0xFF;               // Set = input, clear = output
    INTCON2bits.RBPU = 0;       // Enable pullups

    /* PORT C I/O usage:
        0       i
        1       o       RTS
        2       i       CTS
        3       i       - (not available)
        4       i
        5       i
        6       o       TX1
        7       i       RX1 */
    LATC = 0x00;
    TRISC = 0xBD;               // Set = input, clear = output

    /* Console */
    serial1_init();

    #ifdef DOCONSOLE
    serial1_int_setup(TRUE);
    rts_on();
    uart_pointer = 0;
    uart_pointer_handled = 0;
    #endif

    #ifdef __SDCC
    stdout = STREAM_USER;
    #endif

    /* RTC, setup alarm for a tick every minute */
    ALRMCFGbits.AMASK3 = 0;
    ALRMCFGbits.AMASK2 = 0;
    ALRMCFGbits.AMASK1 = 1;
    ALRMCFGbits.AMASK0 = 1;
    ALRMCFGbits.CHIME = 1;      // Interrupt roll-over enabled
    ALRMCFGbits.ALRMEN = 1;     // Alarm set
    EECON2 = 0x55;              // Clear write lock
    EECON2 = 0xAA;
    RTCCFGbits.RTCWREN = 1;     // Make RTCCFG register writeable, so that we can..
    RTCCFGbits.RTCEN = 1;       // ..enable the RTC
    RTCCFGbits.RTCWREN = 0;
    IPR3bits.RTCCIP = 0;        // Low priority interrupt..
    PIE3bits.RTCCIE = 1;        // ..enabled
    system_uptime = 0;

    /* Timer1, used as the overall system tick
       Using Fosc/4 as clock with an 1:2 prescaler means a timer-increment each 0.167uS,
       60000 increments to get a 10ms interrupt-interval */
    T1CONbits.RD16 = 1;         // Read/write timer as one 16 bits value
    T1CONbits.T1CKPS1 = 0;      // Use a 1:2 prescaler
    T1CONbits.T1CKPS0 = 1;
    T1CONbits.T1OSCEN = 0;      // Disable timer1 oscillator
    T1CONbits.TMR1CS0 = 0;      // Use instruction clock..
    T1CONbits.TMR1CS1 = 0;      // ..as source
    TMR1H = 5536>>8;
    TMR1L = 5536&0xFF;          // Set counter; timer counts up, this value gives us 60000 increments before an interrupt
    IPR1bits.TMR1IP = 0;        // Low priority interrupt..
    PIE1bits.TMR1IE = 1;        // ..enabled
    T1CONbits.TMR1ON = 1;       // Start timer

    /* Timer0, used as a free running timer for random number generation */
    T0CONbits.T08BIT = 0;       // 16 bit timer
    T0CONbits.T0CS = 0;         // Use instruction clock as source
    T0CONbits.PSA = 1;          // Not using the prescaler
    T0CONbits.TMR0ON = 1;       // Start timer
}

static void systick(void)
/*!
  Periodic system tick, 10ms interval
*/
{
    static unsigned short button1_shift = 0xFFFF;
    static unsigned short button2_shift = 0xFFFF;
    static unsigned short button3_shift = 0xFFFF;
    static unsigned short button4_shift = 0xFFFF;
    static unsigned char snake_prescaler = 0;

    button1_shift = (button1_shift<<1) | button1_state();
    if((button1_shift & 0xFC00) == 0) {
        if((buttons & (1<<0)) == 0) {
            buttons |= (1<<0);
        }
    }
    else {
        if((buttons & 1<<0)) {
            buttons &= ~(1<<0);
        }
    }
    
    button2_shift = (button2_shift<<1) | button2_state();
    if((button2_shift & 0xFC00) == 0) {
        if((buttons & (1<<1)) == 0) {
            buttons |= (1<<1);
        }
    }
    else {
        if((buttons & 1<<1)) {
            buttons &= ~(1<<1);
        }
    }
    
    button3_shift = (button3_shift<<1) | button3_state();
    if((button3_shift & 0xFC00) == 0) {
        if((buttons & (1<<2)) == 0) {
            buttons |= (1<<2);
        }
    }
    else {
        if((buttons & 1<<2)) {
            buttons &= ~(1<<2);
        }
    }
    
    button4_shift = (button4_shift<<1) | button4_state();
    if((button4_shift & 0xFC00) == 0) {
        if((buttons & (1<<3)) == 0) {
            buttons |= (1<<3);
        }
    }
    else {
        if((buttons & 1<<3)) {
            buttons &= ~(1<<3);
        }
    }

    snake_prescaler++;
    if(snake_prescaler == 10) {
        snake_prescaler = 0;
        snake_tick();
    }
}

unsigned int ticks(void)
/*!
  Get periodic system tick
*/
{
    unsigned int i;

    i = TMR1L;
    i |= TMR1H<<8;

    return i;
}

void random_setkey(int key)
{
    randomkey = key;
}

unsigned int random()
/*!
  Return a (sort off) random number
  Based on a paper from Deva Seetharam and Sokwoo Rhee, "An Efficient Random Number Generator for Low-Power Sensor Networks"
*/
{
    unsigned int rv = 0;
    unsigned int tv = 0;

    tv = TMR0L;
    tv |= TMR0H<<8;

    rv = tv ^ randomkey;
    randomkey = ~tv;
    tv = ~rv;

    TMR1H = tv>>8;
    TMR1L = tv&0xFF;

    return rv;
}

void main(void)
/*!
  Startpoint of C-code
*/
{
    char did_random_setkey;
    unsigned char buttons_seen;

    /* Low level init; oscillator, watchdog, I/O */
    init();

    buttons = 0;
    buttons_seen = 0;
    did_random_setkey = 0;

    /* Hello world */
    /*
    dprint("PicoLed rev. %s\n\r", SVN_REV);
    #ifdef __SDCC
    dprint("Build %s, %s, SDCC %d\n\r", __DATE__, __TIME__, __SDCC_REVISION);
    #endif
    #ifdef __XC8
    dprint("Build %s, %s, XC8 %d\n\r", __DATE__,__TIME__, __XC8_VERSION);
    #endif
    */

    /* Enable global interrupts */
    RCONbits.IPEN = 1;          /* enable priority interrupts */
    INTCONbits.GIE = 1;         /* enable global interrupts */
    INTCONbits.PEIE = 1;        /* enable peripheral interrupts */

    fgui_init(framebuffer);     /* Initialise screen.. */
    fgui_clear();               /* ..clear framebuffer..*/
    fgui_setfont(font);         /* .. and set font */

    snake_init();

    while(1) {    
        #ifdef DOCONSOLE
        /* Handle incoming data from console */
        while(uart_pointer_handled != uart_pointer) {
            console_incoming(uart_rawdata[uart_pointer_handled]);
            rts_on();
            uart_pointer_handled++;
            if(uart_pointer_handled >= UART_RAWDATASIZE) {
                uart_pointer_handled = 0;
            }

            CLEAR_WATCHDOG();
        }
        #endif

        if(buttons != buttons_seen) {
            buttons_seen = buttons;
            snake_buttons(buttons_seen & 1<<0, buttons_seen & 1<<1, buttons_seen & 1<<2, buttons_seen & 1<<3);

            if(did_random_setkey == 0) {
                random_setkey(ticks());
                did_random_setkey = 1;
            }
        }

        CLEAR_WATCHDOG();
    }
}

#ifdef __SDCC
void putchar (char c) __wparam
#endif
#ifdef __XC8
void putch(char c)
#endif
/*!
  This function is called by printf to print a character
*/
{
    serial1_putchar(c);
}

#ifdef DOCONSOLE
#ifdef __SDCC
void isr_high(void) __interrupt 1
#endif
#ifdef __XC8
void high_priority interrupt isr_high(void)
#endif
/*!
  High priority interrupts
*/
{
    /* UART1 */
    if(RCSTA1bits.OERR) {
        /* Overrun error. Clear by toggeling RCSTA.CREN */
        RCSTA1bits.CREN = 0;
        RCSTA1bits.CREN = 1;
    }
    else if(RCSTA2bits.FERR) {
        /* Framing error. Clear by reading RCREG */
        uart_rawdata[uart_pointer] = RCREG1;
    }
    else {
        uart_rawdata[uart_pointer] = RCREG1;
        uart_pointer++;
        if(uart_pointer >= UART_RAWDATASIZE) {
            uart_pointer = 0;
        }

        if(uart_pointer == uart_pointer_handled) {
            rts_off();
        }
    }
}
#endif

#ifdef __SDCC
void isr_low(void) __interrupt 2
#endif
#ifdef __XC8
void low_priority interrupt isr_low(void)
#endif
/*!
  Low priority interrupts
*/
{
    if(PIR1bits.TMR1IF) {
        /* Timer1, setup for an interrupt each 10 ms */
        PIR1bits.TMR1IF = 0;
        TMR1H = 5536>>8;
        TMR1L = 5536&0xFF;
        systick();
    }
    else if(PIR3bits.RTCCIF) {
        /* RTC, setup for one interrupt per minute */
        system_uptime++;
        PIR3bits.RTCCIF = 0;
    }
    else if(PIR2bits.LVDIF) {
        /* Low voltage detect */
        RESET();
    }
    else if(PIR2bits.BCL1IF) {
        /* Bus-collision */
        RESET();
    }
    else {
        /* dprint("Unhandled low priority interrupt\n\r"); */
        RESET();
    }
}
