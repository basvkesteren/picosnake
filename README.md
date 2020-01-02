This is snake (https://en.wikipedia.org/wiki/Snake_(video_game_genre)) on a WS2812 led string

The display is made by cutting up a 7 meter WS2812 LED string..
![Cutting the LED string](ledstring_cutting.jpg?raw=true "Cutting the LED string")
..and glueing this in a ~40x30 CM photo frame
![The LED string frame](ledstring_frame.jpg?raw=true "The LED string frame")

The string is driven by a PIC18F24J50  (https://www.microchip.com/wwwproducts/en/en534039), an 8 bit microcontroller. See [schematic.sch](./schematic.sch) for a full schematic. Main reason for the component choice (including the microcontroller) is for it to be available in my pile of components.

The code can be compiled with XC8 (https://www.microchip.com/mplab/compilers) or SDCC (http://sdcc.sourceforge.net/), see the 'CC' define in the [Makefile](./Makefile)