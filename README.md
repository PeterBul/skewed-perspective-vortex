# Skewed Perspective Vortex

This is the driver of 980 WS2811 LEDs oritented as a cube of 7 x 7 x 20.

The project used a Teensy 4.1 chip with parallel outputs for higher update rates, but the code has also been tested on an Arduino Mega.

The lights were oriented as a 2m x 2m x2m cube.
The lighst have programmed animations and state that changes over time and reacts to an accelerometer which can move a digital entity within the cube.

The code is collected in a main file to deal with import headaches when using cpp files in arduino/teensy.
