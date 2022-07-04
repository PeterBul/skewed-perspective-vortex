/*  OctoWS2811 Rainbow.ino - Rainbow Shifting Test
    http://www.pjrc.com/teensy/td_libs_OctoWS2811.html
    Copyright (c) 2013 Paul Stoffregen, PJRC.COM, LLC

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.


  Required Connections
  --------------------
    pin 2:  LED Strip #1    OctoWS2811 drives 8 LED Strips.
    pin 14: LED strip #2    All 8 are the same length.
    pin 7:  LED strip #3
    pin 8:  LED strip #4    A 100 ohm resistor should used
    pin 6:  LED strip #5    between each Teensy pin and the
    pin 20: LED strip #6    wire to the LED strip, to minimize
    pin 21: LED strip #7    high frequency ringining & noise.
    pin 5:  LED strip #8
    pin 15 & 16 - Connect together, but do not use
    pin 4 - Do not use
    pin 3 - Do not use as PWM.  Normal use is ok.
    pin 1 - Output indicating CPU usage, monitor with an oscilloscope,
            logic analyzer or even an LED (brighter = CPU busier)
*/

#include <FastLED.h>
#include <PrintStream.h>

const bool develop = false;

#if defined(__MK66FX1M0__)

#    include <OctoWS2811.h>
#    define NUM_LEDS 1120

namespace octoUtils {
    CRGB ledsArray[NUM_LEDS];
    const int ledsPerStrip = 140;

    DMAMEM int displayMemory[ledsPerStrip * 6];
    int drawingMemory[ledsPerStrip * 6];

    const int config = WS2811_RGB | WS2811_800kHz;

    OctoWS2811 octoLeds(ledsPerStrip, displayMemory, drawingMemory, config);

    void show() {
        octoLeds.show();
    }

    void begin() {
        octoLeds.begin();
    }

    void setLightsFromArray() {
        for (int i = 0; i < NUM_LEDS; i++) {
            if (i >= 140 && i < 2 * 140) {
                continue;
            }
            octoLeds.setPixel(i, ledsArray[i]);
        }
    }

    uint32_t getPixelValue(uint32_t x, uint32_t y, uint32_t z) {
        uint32_t val = 20 * x + 140 * y + z;
        if (val >= 140) {
            return val + 140;
        }
        return val;
    }

    CRGB* getLed(int x, int y, int z) {
        // const int stripIndex = (y + y % 2) / 2 * NUM_X + (1 - 2 * (y % 2)) * ((x - x % 2) / 2) - (y % 2);
        // const int ledIndex = ((x + (y % 2)) % 2) * NUM_Z + z;
        return &ledsArray[getPixelValue(x, y, z)];
    }

    void setPixel(uint32_t x, uint32_t y, uint32_t z, int color) {
        if (develop) {
            Serial << "Setting pixel:" << x << ", " << y << ", " << z << " in color: int(" << color << ")\n";
        } else {
            ledsArray[getPixelValue(x, y, z)] = color;
            octoLeds.setPixel(getPixelValue(x, y, z), color);
        }
    }

    void setPixel(uint32_t x, uint32_t y, uint32_t z, uint8_t red, uint8_t green, uint8_t blue) {
        if (develop) {
            Serial << "Setting pixel:" << x << ", " << y << ", " << z << " in color: rgb(" << red << "," << green << "," << blue << ")";
        } else {
            ledsArray[getPixelValue(x, y, z)] = octoLeds.color(red, green, blue);
            octoLeds.setPixel(getPixelValue(x, y, z), red, green, blue);
        }
    }

    void setPixel(uint32_t num, int color) {
        ledsArray[num] = color;
        octoLeds.setPixel(num, color);
    }

    int getPixel(uint32_t x, uint32_t y, uint32_t z) {
        return octoLeds.getPixel(getPixelValue(x, y, z));
    }

    void setAllPixels(int color) {
        if (develop) {
            Serial << "Setting all pixels to: int(" << color << ")";
        } else {
            for (uint32_t i = 0; i < 7 * 7 * 20; i++) {
                octoLeds.setPixel(i, color);
            }
        }
    }

    void setAllPixels(uint8_t red, uint8_t green, uint8_t blue) {
        if (develop) {
            Serial << "Setting all pixels to: rgb(" << red << "," << green << "," << blue << ")";
        } else {
            for (uint32_t i = 0; i < 7 * 7 * 20; i++) {
                octoLeds.setPixel(i, red, green, blue);
            }
        }
    }
}

#else
namespace octoUtils {
    void show() {
    }

    void begin() {
    }

    void setPixel(uint32_t x, uint32_t y, uint32_t z, int color) {
        Serial << "Setting pixel:" << x << ", " << y << ", " << z << " in color: int(" << color << ")\n";
    }

    void setPixel(uint32_t x, uint32_t y, uint32_t z, uint8_t red, uint8_t green, uint8_t blue) {
        Serial << "Setting pixel:" << x << ", " << y << ", " << z << " in color: rgb(" << red << "," << green << "," << blue << ")";
    }

    void setAllPixels(int color) {

        Serial << "Setting all pixels to: int(" << color << ")";
    }

    void setAllPixels(uint8_t red, uint8_t green, uint8_t blue) {
        Serial << "Setting all pixels to: rgb(" << red << "," << green << "," << blue << ")";
    }
}

#endif
