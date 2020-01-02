#
#   Makefile
#
#   Copyright (c) 2006, 2018 Bastiaan van Kesteren <bas@edeation.nl>
#   This program comes with ABSOLUTELY NO WARRANTY; for details see the file LICENSE.
#   This program is free software; you can redistribute it and/or modify it under the terms
#   of the GNU General Public License as published by the Free Software Foundation; either
#   version 2 of the License, or (at your option) any later version.
#

# The PIC type
PIC = 18f24j50
# Name of the project
NAME = PicoSnake
# All C code files, seperated with spaces
SRC = main.c delay.c serial1.c console.c snake.c

# Stuff to include
INCLUDE = -I./ -I./include -I./fgui

# Stuff to define
DEFINES =

# Compiler/toolchain to use; sdcc or xc8
CC = xc8

include fgui/makefile.inc

ifeq ($(CC),sdcc)
ASM = gpasm

OBJ = $(patsubst %.c,%.o,$(SRC))

CFLAGS = -mpic16 -p$(PIC) --use-non-free --use-crt=crt0iz.o $(DEFINES) $(INCLUDE)

LDFLAGS = $(CFLAGS) -l libc18f.lib -l libsdcc.lib -l libm18f.lib -Wl'-s $(PIC).lkr -S2 -m -p1 -C -O2' -o $(NAME).hex
endif

ifeq ($(CC),xc8)
OBJ = $(patsubst %.c,%.p1,$(SRC))

CFLAGS = -Q --chip=$(PIC) --OPT=asm --ASMLIST $(DEFINES) $(INCLUDE)

LDFLAGS = $(CFLAGS) -O$(NAME).hex
endif

all: svnrev $(NAME).hex

svnrev:
# Some trickery qith quotes on different platforms... :/
ifeq ($(OS),Windows_NT)
	@echo #define SVN_REV "$(shell svnversion -n .)" > ./svnrev.h
else
	@echo '#define SVN_REV "$(shell svnversion -n .)"' > ./svnrev.h
endif

$(NAME).hex : $(OBJ)
	@echo Linking...
	$(CC) $(OBJ) $(LDFLAGS)

%.p1: %.c Makefile
	@echo Compiling... $<
	@$(CC) $(CFLAGS) --pass1 --OUTDIR=$(patsubst %/,%,$(dir $<)) $<

%.o: %.c Makefile
	@echo Compiling... $<
	@$(CC) $(CFLAGS) -S -c $< -o $(patsubst %.c,%.asm,$<)
	@$(ASM) -c $*.asm

clean:
	@rm -f $(OBJ) $(NAME).hex svnrev.h
	@rm -f $(patsubst %.c,%.asm,$(SRC)) $(patsubst %.c,%.lst,$(SRC)) $(patsubst %.c,%.o,$(SRC))
	@rm -f $(patsubst %.c,%.d,$(SRC)) $(patsubst %.c,%.pre,$(SRC)) $(NAME).cod $(NAME).lst $(NAME).map
