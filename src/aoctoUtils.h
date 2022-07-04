#include <Arduino.h>
#include <FastLED.h>

namespace octoUtils {
    void show();

    void begin();

    void setPixel(uint32_t x, uint32_t y, uint32_t z, int color);
    void setPixel(uint32_t x, uint32_t y, uint32_t z, uint8_t red, uint8_t green, uint8_t blue);
    void setPixel(uint32_t num, int color);
    int getPixel(uint32_t x, uint32_t y, uint32_t z);

    void setAllPixels(int color);

    void setAllPixels(uint8_t red, uint8_t green, uint8_t blue);

    CRGB* getLed(int x, int y, int z);
    void setLightsFromArray();
}