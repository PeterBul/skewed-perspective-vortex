#include "aoctoUtils.h"
#include "colorUtils.h"
#include <Arduino.h>
#include <FastLED.h>
#include <cmath>

// Use if you want to force the software SPI subsystem to be used for some reason (generally, you don't)
// #define FASTLED_FORCE_SOFTWARE_SPI
// Use if you want to force non-accelerated pin access (hint: you really don't, it breaks lots of things)
// #define FASTLED_FORCE_SOFTWARE_SPI
// #define FASTLED_FORCE_SOFTWARE_PINS
#include "math.h"

///////////////////////////////////////////////////////////////////////////////////////////
//
// Move a white dot along the strip of leds.  This program simply shows how to configure the leds,
// and then how to turn a single pixel white and then off, moving down the line of pixels.
//

// How many leds are in the strip?
#define NUM_LEDS 40
#define NUM_LEDS_LAST 20
#define NUM_STRINGS 25
#define BRIGHTNESS 255
#define NUM_X 7
#define NUM_Y 7
#define NUM_Z 20

const int xPin = A14;
const int yPin = A15;
const int zPin = A16;

int accX;
int accY;
int accZ;

float accZMovingAvg = 0;

float ballX = 0;
float ballY = 0;
float ballZ = 0;

float persistence;

int scale;
uint16_t t;
int pos;

uint32_t c;

int minY = 0;
uint8_t maxY = NUM_Y;
int minX = 0;
uint8_t maxX = NUM_X;
int offsetY;
bool subtractInPlayBall = true;

const TProgmemRGBPalette16 SpaceColors_p =
    {
        CRGB::Purple,
        CRGB::Purple,
        CRGB::Purple,
        CRGB::Purple,

        CRGB::Black,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black,

        CRGB::Purple,
        CRGB::Purple,
        CRGB::Purple,
        CRGB::Purple,

        CRGB::Black,
        CRGB::Black,
        CRGB::Black,
        CRGB::Black};

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI

// #define CLOCK_PIN 13

int rainbowColors[180];
const int ledsPerStrip = 140;

void initPlastic();
void initRainbowColors();
void checkColors();
void printBall(float x, float y);
void printRedPill();

// This function sets up the ledsand tells the controller about them
void setup() {
    // sanity check delay - allows reprogramming if accidently blowing power w/leds
    // delay(2000);
    Serial.begin(9600);
    pinMode(1, OUTPUT);
    digitalWrite(1, HIGH);

    // octoUtils::setAllPixels(0, 0, 0);
    initPlastic();

    octoUtils::begin();

    digitalWrite(1, LOW);

    // persistence affects the degree to which the "finer" noise is seen
    persistence = 0.25;
    // octaves are the number of "layers" of noise that get computed;

    pos = 0;

    // for (int i = 0; i < NUM_STRINGS; i++) {
    //     for (int j = 0; j < NUM_LEDS; j++) {
    //         leds[i][j] = CRGB::Black;
    //     }
    // }

    pinMode(1, OUTPUT);
    digitalWrite(1, HIGH);
    // initRainbowColors();
    digitalWrite(1, LOW);
    octoUtils::begin();
    // checkColors();

    // theGameOfLifeInit();
    // gameOfLifeNeighbours2d(0,2,9);
}

void initRainbowColors() {
    for (int i = 0; i < 180; i++) {
        int hue = i * 2;
        int saturation = 100;
        int lightness = 50;
        // pre-compute the 180 rainbow colors
        rainbowColors[i] = hsl2rgb(hue, saturation, lightness);
    }
}

int startColorIndex = 0;

int startColorIndexSpeed = 1;

int shootingStarTrigger = random(1000);

// uint8_t star = 0;

int randomDelay = 5000;
int nStarsToRegister = 1;

uint16_t shootingStars[NUM_X][NUM_Z];
CRGB shootingStarsColors[NUM_X][NUM_Z];

bool isWallHit = false;

const uint8_t SPACE_TRAVEL = 0;
const uint8_t OCEAN = 1;
const uint8_t GLOBE = 2;
const uint8_t TREE = 3;
const uint8_t MATRIX = 4;
const uint8_t STRING_ITERATE = 5;

uint8_t state = OCEAN;
uint8_t nextState;

void spaceTravel();
void landscape(int colorIndex);
void playBall(bool useNoise, bool useCPU);
void globe();
void playWithTrees();
void stringIterate();
void portal();
void treeDancing();
void theMatrix();
void handleTravelState();

void rainbow(int phaseShift, int cycleTime);

void loop() {
    // checkColors();
    // rainbow(10, 2500);
    c++;
    t = millis() / 5;

    scale = 50;
    bool useCpu = true;
    switch (state) {
        case SPACE_TRAVEL:
            spaceTravel();
            delay(30);
            break;
        case OCEAN:
            landscape(startColorIndex);
            playBall(true, useCpu);
            delay(30);
            break;
        case GLOBE:
            globe();
            delay(3);
            playBall(false, useCpu);
            break;
        case TREE:
            playWithTrees();
            treeDancing();
            playBall(false, useCpu);
            delay(30);
            break;
        case MATRIX:
            theMatrix();
            playBall(false, useCpu);
            delay(30);
            break;
        case STRING_ITERATE:
            stringIterate();
            delay(30);
            break;
    }

    portal();
    handleTravelState();
    startColorIndex += startColorIndexSpeed;
    if (state != STRING_ITERATE) {
        octoUtils::show();
    }
}

// phaseShift is the shift between each row.  phaseShift=0
// causes all rows to show the same colors moving together.
// phaseShift=180 causes each row to be the opposite colors
// as the previous.
//
// cycleTime is the number of milliseconds to shift through
// the entire 360 degrees of the color wheel:
// Red -> Orange -> Yellow -> Green -> Blue -> Violet -> Red
//
void rainbow(int phaseShift, int cycleTime) {
    int color, x, y, offset, wait;

    wait = cycleTime * 1000 / ledsPerStrip;
    for (color = 0; color < 180; color++) {
        digitalWrite(1, HIGH);
        for (x = 0; x < ledsPerStrip; x++) {
            for (y = 0; y < 8; y++) {
                int index = (color + x + y * phaseShift / 2) % 180;
                octoUtils::setPixel(x + y * ledsPerStrip, rainbowColors[index]);
            }
        }
        octoUtils::show();
        digitalWrite(1, LOW);
        delayMicroseconds(wait);
    }
}

void checkColors() {
    for (int y = 0; y < 7; y++) {
        for (int x = 0; x < 7; x++) {
            for (int z = 0; z < 20; z++) {
                octoUtils::setPixel(x, y, z, CRGB::Aquamarine);
            }
            octoUtils::show();
            delay(30);
            for (int z = 0; z < 20; z++) {
                octoUtils::setPixel(x, y, z, 0, 0, 0);
            }
        }
    }
    // int i0 = 1;
    // for (int i0 = 0; i0 < 8; i0++) {
    //     for (int i = 0; i < 1120; i++) {
    //         digitalWrite(1, HIGH);
    //         if (i >= i0 * 140 && i < (i0 + 1) * 140) {
    //             octoUtils::setPixel(i, hsl2rgb(180, 100, 50));

    //         } else {
    //             octoUtils::setPixel(i, hsl2rgb(180, 100, 0));
    //         }
    //         digitalWrite(1, LOW);
    //         delayMicroseconds(100);
    //     }
    //     octoUtils::show();
    //     delay(3000);
    // }
}

CRGB getRandomColor();

void registerShoothingStar() {
    int x = random(NUM_X);
    int z = random(NUM_Z);

    bool hasShootingStar = !(shootingStars[x][z] + NUM_Y < c);
    if (!hasShootingStar) {
        Serial.print("Register shooting star: ");
        Serial.println(x);
        uint32_t randomNr = random(randomDelay);
        Serial.print("Shooting star (x: ");
        Serial.print(x);
        Serial.print(", z: ");
        Serial.print(z);
        Serial.print("): c + randomNr: ");
        Serial.print(c);
        Serial.print(" + ");
        Serial.println(randomNr);

        shootingStars[x][z] = c + NUM_Y;
        CRGB color = getRandomColor();
        Serial.print("Random color: ");
        Serial.println(color);
        shootingStarsColors[x][z] = CRGB::White;
    }
}

int matrixDrops[NUM_X][NUM_Y];

void registerMatrixDrop() {
    int x = random(NUM_X);
    int y = random(NUM_Y);
    if (matrixDrops[x][y] + NUM_Z < c) {
        matrixDrops[x][y] = c + random(20);
    }
}

// void registerStarWall(){
//    for (int x = 0; x < NUM_X; x++){
//       for (int z = 0; z < NUM_Z; z++) {
//          shootingStars[x][z] = c + 1;
//          shootingStarsColors[x][z] = CRGB::White;
//       }
//    }
// }

bool hasPrinted = false;

int8_t nStarsAcceleration = 1;
int8_t registerStarDelayChange = -20;
int spaceCStart = c;
float registerStarInterval = 200;

void spaceTravel() {
    if (state != SPACE_TRAVEL) {
        return;
    }

    if (!hasPrinted) {
        Serial.print("Random delay:\t");
        Serial.println(randomDelay);
        Serial.print("c:\t");
        Serial.println(c);
        Serial.print("nStarsToRegister:\t");
        Serial.println(nStarsToRegister);
        Serial.print("nStarsAcceleration:\t");
        Serial.println(nStarsAcceleration);
    }

    if (randomDelay == 0 && c % 50 == 0) {
        nStarsToRegister += nStarsAcceleration;
    }

    if (nStarsToRegister > 5) {
        nStarsAcceleration = -1;
    }

    if (nStarsToRegister < 0) {
        nStarsToRegister = 0;
    }

    if (nStarsToRegister == 0 && nStarsAcceleration == -1) {
        registerStarDelayChange = 20;
    }

    for (int i = 0; i < nStarsToRegister; i++) {
        // registerShoothingStar();
    }

    registerStarInterval -= 0.2 + (registerStarInterval / 800);
    registerStarInterval = max(1, registerStarInterval);

    if (c % int(registerStarInterval) == 0) {
        registerShoothingStar();
    }
    // if (nStarsToRegister > 12 && !isWallHit){
    //    isWallHit = true;
    //    registerStarWall();
    // }

    for (uint8_t x = 0; x < NUM_X; x++) {
        for (uint8_t y = 0; y < NUM_Y; y++) {
            for (uint8_t z = 0; z < NUM_Z; z++) {
                if (c - shootingStars[x][z] == y) {
                    Serial.print("Shooting star: ");
                    Serial.println(shootingStarsColors[x][z]);
                    octoUtils::setPixel(x, y, z, CRGB::White);
                    // octoUtils::setPixel(x, y, z, shootingStarsColors[x][z]);
                } else {
                    octoUtils::setPixel(x, y, z, CRGB::Black);
                }
            }
        }
    }

    randomDelay += registerStarDelayChange;

    if (randomDelay < 0) {
        randomDelay = 0;
    }

    // if (randomDelay > 0 && nStarsAcceleration == -1) {
    if (c - spaceCStart > 1000) {
        nStarsAcceleration = 1;
        registerStarDelayChange = -20;
        randomDelay = 5000;
        state = nextState;
        if (nextState == TREE) {
            offsetY = 3;
        }
        maxY = 0;
        minX = 3;
        maxX = 3;
    }
}

int8_t redPillX = -1;
int8_t redPillY = -1;
int8_t bluePillX = -1;
int8_t bluePillY = -1;

void registerPill() {
    redPillX = random(NUM_X);
    redPillY = random(NUM_Y);
    bluePillX = random(NUM_X);
    bluePillY = random(NUM_Y);
    while (redPillX == bluePillX || redPillY == bluePillY) {
        bluePillX = random(NUM_X);
        bluePillY = random(NUM_Y);
    }
}

CRGB getRandomColor() {
    CRGB color;
    switch (random(20)) {
        case 0:
            color = CRGB(255, 200, 255);
            break;
        case 1:
            color = CRGB(200, 255, 255);
            break;
        default:
            color = CRGB::White;
            break;
    }
    return color;
}

int plasticX;
int plasticY;
int plasticDepth;
int plasticCounter;
bool portalOpen;

void initPlastic() {
    plasticX = random(NUM_X);
    plasticY = random(NUM_Y);
    plasticDepth = NUM_Z - 1;
    plasticCounter = 0;
    portalOpen = false;
}

int nDrops = 2;
bool pillRegistered = false;
uint8_t pillCounter = 0;
void theMatrix() {
    if (state != MATRIX) {
        return;
    }

    if (!pillRegistered) {
        registerPill();
        pillRegistered = true;
    }

    for (int i = 0; i < nDrops; i++) {
        registerMatrixDrop();
    }

    for (uint8_t x = 0; x < NUM_X; x++) {
        for (uint8_t y = 0; y < NUM_Y; y++) {
            for (uint8_t z = 0; z < NUM_Z; z++) {
                if (x == redPillX && y == redPillY && (z == 10 || z == 11)) {
                    octoUtils::setPixel(x, y, z, CRGB::Red);
                } else if (x == bluePillX && y == bluePillY && (z == 10 || z == 11)) {
                    octoUtils::setPixel(x, y, z, CRGB::Blue);
                } else if (abs(c - matrixDrops[x][y] - z) < 3) {
                    octoUtils::setPixel(x, y, z, CRGB::Green);
                }
            }
        }
    }
    int intBallX = int(ballX);
    int intBallY = int(ballY);
    printBall(ballX, ballY);
    printRedPill();
    float lambda = 0.1;
    if (abs(ballX - redPillX) < lambda && abs(intBallY - redPillY) < lambda) {
        pillCounter++;
        registerPill();
    } else if (abs(intBallX - bluePillX) < lambda && (intBallY - bluePillY) < lambda) {
        pillCounter = 0;
        registerPill();
    }

    if (pillCounter > 5) {
        portalOpen = true;
    }
}

enum e_PortalMode { normal,
                    strobe,
                    iterate };

e_PortalMode portalMode = e_PortalMode::normal;

void portal() {
    // TODO: Return if portal is closed
    for (uint8_t x = 0; x < NUM_X; x++) {
        for (uint8_t y = 0; y < NUM_Y; y++) {
            for (uint8_t z = 0; z < NUM_Z; z++) {
                if (portalOpen && (y == minY || y == minY + 1) && (((x == 2 || x == 4) && z > 3) || (2 <= x && x <= 4 && z == 3))) {

                    if (state == MATRIX || state == TREE) {
                        if (portalMode == e_PortalMode::strobe && c % 10 < 5) {
                            octoUtils::setPixel(x, y, z, CRGB::Aquamarine);
                        } else {
                            octoUtils::setPixel(x, y, z, CRGB::Teal);
                        }
                    } else {
                        if (portalMode == e_PortalMode::strobe && c % 10 < 5) {
                            octoUtils::setPixel(x, y, z, CRGB::DarkGreen);
                        } else {
                            octoUtils::setPixel(x, y, z, CRGB::Green);
                        }
                    }
                }
                if (state == MATRIX && portalOpen && (y == minY || y == minY + 1) && x == 3 && z > 3) {
                    octoUtils::setPixel(x, y, z, CRGB::Black);
                }
            }
        }
    }
}

uint8_t getNoise(int x, int y);

void landscape(int colorIndex) {
    if (state != OCEAN) {
        return;
    }
    pos += 5;

    for (int x = minX; x < maxX; x++) {
        for (int y = 0; y < maxY; y++) {
            // int8_t prevNoise = -1;
            // if (x > 0)
            // {
            // 	prevNoise = getNoise(x - 1, y);
            // }
            uint8_t noise = getNoise(x, y);
            for (uint8_t z = 0; z < NUM_Z; z++) {
                if (y < minY) {
                    octoUtils::setPixel(x, y, z, CRGB::Black);
                    continue;
                }
                if (z == noise) {
                    // if (prevNoise <= noise){
                    //    octoUtils::setPixel(x, y, z, hsl2rgb(160, 80, 100));
                    // } else {
                    //    octoUtils::setPixel(x, y, z, hsl2rgb(160, 80, 50));
                    // }
                    // octoUtils::setPixel(x, y, z, ColorFromPalette(OceanColors_p, colorIndex, 255, LINEARBLEND));
                    octoUtils::setPixel(x, y, z, CRGB::Aquamarine);
                } else if (z < noise) {
                    // octoUtils::setPixel(x, y, z, CRGB::Black);
                } else {
                    octoUtils::setPixel(x, y, z, CRGB::Navy);
                }
                if (z == plasticDepth && abs(x - plasticX) <= 1 && abs(y - plasticY) <= 1 && !portalOpen) {
                    octoUtils::setPixel(x, y, z, CRGB::White);
                }
            }
        }
        colorIndex += 3;
    }

    int intBallX = round(ballX);
    int intBallY = round(ballY);

    if (intBallX == plasticX && intBallY == plasticY) {
        plasticDepth--;
        if (plasticDepth == 0) {
            plasticCounter++;
            plasticX = random(NUM_X);
            plasticY = random(NUM_Y);
            plasticDepth = NUM_Z - 1;
        }
    }

    if (plasticCounter > 5) {
        portalOpen = true;
    }

    if (maxY < NUM_Y) {
        maxY++;
    }
    if (minX > 0) {
        minX--;
    }
    if (maxX < NUM_X) {
        maxX++;
    }
}

uint8_t getNoise(int x, int y) {
    return uint8_t(inoise8(x * scale + pos, y * scale, t) / 17 + 5);
}

void printAcc() {
    Serial.print(accX);
    Serial.print("\t");
    Serial.print(accY);
    Serial.print("\t");
    Serial.println(accZ);
}

void testLightMapping() {
    for (uint8_t z = 0; z < NUM_Z; z++) {
        for (uint8_t x = 0; x < NUM_X; x++) {
            for (uint8_t y = 0; y < NUM_Y; y++) {
                octoUtils::setPixel(x, y, z, CRGB::White);
            }
        }
        octoUtils::show();
        for (uint8_t x = 0; x < NUM_X; x++) {
            for (uint8_t y = 0; y < NUM_Y; y++) {
                octoUtils::setPixel(x, y, z, CRGB::Black);
            }
        }
    }
}

void initSpaceTravel();

int stringIterateCounter = 0;
void stringIterate() {
    for (uint8_t x = 0; x < NUM_X; x++) {
        for (uint8_t y = 0; y < NUM_Y; y++) {
            for (uint8_t z = 0; z < NUM_Z; z++) {
                octoUtils::setPixel(x, y, z, CRGB::White);
            }
            octoUtils::show();
            for (uint8_t z = 0; z < NUM_Z; z++) {
                octoUtils::setPixel(x, y, z, CRGB(16, 6, 20));
            }
        }
    }
    stringIterateCounter++;
    Serial.println(stringIterateCounter);
    if (stringIterateCounter % 5 == 0) {
        nextState = OCEAN;
        initSpaceTravel();
        state = SPACE_TRAVEL;
    }
}

// void test() {
//     for (int i = 0; i < NUM_STRINGS; i++) {
//         for (int j = 0; j < NUM_LEDS; j++) {
//             if ((i == NUM_STRINGS - 1) && (j >= NUM_LEDS_LAST)) {
//                 break;
//             }
//             leds[i][j] = CRGB::White;
//         }
//         octoUtils::show();
//         for (int j = 0; j < NUM_LEDS; j++) {
//             if ((i == NUM_STRINGS - 1) && (j >= NUM_LEDS_LAST)) {
//                 break;
//             }
//             leds[i][j] = CRGB::Black;
//         }
//     }
// }

void printXYZ(float x, float y, float z) {
    Serial.print(x);
    Serial.print("\t");
    Serial.print(y);
    Serial.print("\t");
    Serial.println(z);
}

void readAcc() {
    float alpha = 0.1;
    bool zActive = 100 < accX && accX < 140 && 100 < accY && accY < 140;

    accX = analogRead(xPin) - 410;
    accY = analogRead(yPin) - 410;
    accZ = analogRead(zPin) - 410;

    // Serial.print(accX);
    // Serial.print(" ");
    // Serial.print(accY);
    // Serial.print(" ");
    // Serial.println(accZ);

    if (zActive) {
        accZMovingAvg = ((1 - alpha) * accZMovingAvg + alpha * accZ) / 2;
    }
}

void renderBall(bool useNoise, int decay) {
    int intBallX = round(ballX);
    int intBallY = round(ballY);
    int intBallZ = round(ballZ);
    for (uint8_t x = 0; x < NUM_X; x++) {
        for (uint8_t y = 0; y < NUM_Y; y++) {
            uint8_t surface;
            if (useNoise) {
                surface = getNoise(x, y);
            } else {
                surface = 10;
            }
            int manet = surface - intBallZ;
            for (uint8_t z = 0; z < NUM_Z; z++) {
                // float d = distance(x, y, z, ballX, ballY, ballZ);
                if ((x == intBallX || x - intBallX == 1) && (y == intBallY || y - intBallY == 1) && manet == z) {
                    // float dZ = abs(ballZ - z);
                    // int b = round(255 * dZ);
                    octoUtils::setPixel(x, y, z, CRGB::Purple);
                } else {
                    if (state == GLOBE && c % 200 < 40 && c % NUM_Z < z) {
                        continue;
                    }
                    if (subtractInPlayBall) {
                        // CRGB* led = getLed(x, y, z);
                        CRGB l = octoUtils::getPixel(x, y, z);
                        if (decay > 0) {

                            // octoUtils::setPixel(x, y, z, l);
                            l.subtractFromRGB(decay);
                            octoUtils::setPixel(x, y, z, l.r, l.g, l.b);
                        }
                    }
                }
            }
        }
    }
}

void resetShootingStars() {
    for (uint8_t x = 0; x < NUM_X; x++) {
        for (uint8_t z = 0; z < NUM_Z; z++) {
            shootingStars[x][z] = 0;
        }
    }
    spaceCStart = c;
}
void initSpaceTravel() {
    resetShootingStars();
    nStarsAcceleration = 1;
    registerStarDelayChange = -20;
    randomDelay = 5000;
    nStarsToRegister = 1;
    registerStarInterval = 200;
}

void moveBall(bool useCPU);

void playBall(bool useNoise, bool useCPU) {
    readAcc();
    moveBall(useCPU);
    renderBall(useNoise, 10);
}

void playBall(bool useNoise, bool useCPU, int decay) {
    readAcc();
    moveBall(useCPU);
    renderBall(useNoise, decay);
}

float zStep = 1;

void resetForest();

uint16_t ballCounter = 0;
int8_t transitionCounter = -1;
void handleTravelState() {
    int intBallX = round(ballX);
    int intBallY = round(ballY);

    if ((intBallX == 3 || intBallX == 2) && intBallY == 0 && portalOpen) {
        if (state == OCEAN || state == TREE || state == MATRIX) {
            ballCounter++;
        }
    } else if (intBallX == 3 && intBallY == 3 && state == GLOBE) {
        ballCounter++;
    } else {
        ballCounter = 0;
    }
    if (ballCounter > 20 && transitionCounter <= 0) {
        ballCounter = 0;
        switch (state) {
            case OCEAN:
                nextState = TREE;
                transitionCounter = NUM_Y;
                break;
            case TREE:
                nextState = GLOBE;
                transitionCounter = NUM_Y;
                resetForest();
                break;
            case GLOBE:
                if (random(5) < 4) {
                    nextState = MATRIX;
                } else {
                    nextState = STRING_ITERATE;
                }
                transitionCounter = 22;
                subtractInPlayBall = false;
                ballX = 3;
                ballY = 3;
                break;
            case MATRIX:
                nextState = OCEAN;
                transitionCounter = NUM_Y;
                break;
        }
    }
    if (transitionCounter > -1) {
        minY = NUM_Y - transitionCounter;
        transitionCounter--;
        delay(50);
    }
    if (transitionCounter == 0) {
        subtractInPlayBall = true;
        initPlastic();
        initSpaceTravel();
        portalOpen = false;
        state = SPACE_TRAVEL;
    }

    if (transitionCounter <= 0) {
        minY = 0;
    }
}

void moveBall(bool useCPU) {
    float step;
    if (useCPU) {
        switch (state) {
            case OCEAN:
                step = 0.01;
                break;
            default:
                step = 0.05;
                break;
        }
    } else {
        step = 0.2;
    }

    // bool zActive = 110 < accX && accX < 130 && 110 < accY && accY < 130;
    if (useCPU) {
        int targetX;
        int targetY;
        switch (state) {
            case OCEAN:
                if (portalOpen) {
                    targetX = 3;
                    targetY = 0;
                } else {
                    targetX = plasticX;
                    targetY = plasticY;
                }
                break;
            case TREE:

                if (ballX != 0 && ballY != 0 && ballX != NUM_X - 1 && ballY != NUM_Y - 1) {
                    targetX = 3;
                    targetY = 0;
                } else if (ballY == 0 && ballX != NUM_X - 1) {
                    targetX = NUM_X - 1;
                    targetY = 0;
                } else if (ballX == NUM_X - 1 && ballY != NUM_Y - 1) {
                    targetX = NUM_X - 1;
                    targetY = NUM_Y - 1;
                } else if (ballY == NUM_Y - 1 && ballX != 0) {
                    targetX = 0;
                    targetY = NUM_Y - 1;
                } else if (ballX == 0 && ballY != 0) {
                    targetX = 0;
                    targetY = 0;
                }
                break;
            case GLOBE:
                targetX = 3;
                targetY = 3;
                break;
            case MATRIX:
                if (portalOpen) {
                    targetX = 3;
                    targetY = 0;
                } else {
                    targetX = redPillX;
                    targetY = redPillY;
                }
                break;
            default:
                targetX = random(7);
                targetY = random(7);
                break;
        }

        if (targetX < ballX) {
            ballX -= step;
        } else if (targetX > ballX) {
            ballX += step;
        }
        if (targetY < ballY) {
            ballY -= step;
        } else if (targetY > ballY) {
            ballY += step;
        }

    } else {
        if (accX < 80) {
            ballX -= step;
        }
        if (accX > 160) {
            ballX += step;
        }

        if (accY < 80) {
            ballY += step;
        }
        if (accY > 160) {
            ballY -= step;
        }
    }

    // // 120
    // if (zActive && accZ < 200){
    //    ballZ -= step;
    // }
    // // 160
    // if (zActive && accZ > 206){
    //    ballZ += step;
    // }

    ballZ += zStep;

    ballX = min(NUM_X - 1, ballX);
    ballY = min(NUM_Y - 1, ballY);
    // ballZ = min(NUM_Z - 1, ballZ);
    if (ballZ < 2) {
        ballZ = 2;
        zStep = 0.6;
    }
    if (ballZ > 7) {
        ballZ = 7;
        zStep = -0.2;
    }

    ballX = max(0, ballX);
    ballY = max(0, ballY);
    ballZ = max(0, ballZ);
}

float distance(int x1, int y1, int z1, int x2, int y2, int z2) {
    double dX = x1 - x2;
    double dY = y1 - y2;
    double dZ = (z1 - z2) / 2.67;
    return sqrt(pow(dX, 2) + pow(dY, 2) + pow(dZ, 2)) * 2.67;
}

void printBall(float x, float y) {
    Serial.print("Ball:\t(");
    Serial.print(x);
    Serial.print(", ");
    Serial.print(y);
    Serial.println(")");
}

void printRedPill() {
    Serial.print("Red pill:\t(");
    Serial.print(redPillX);
    Serial.print(", ");
    Serial.print(redPillY);
    Serial.println(")");
}

void flush() {
    for (uint8_t y = 0; y < NUM_Y; y++) {
        for (uint8_t x = 0; x < NUM_X; x++) {
            for (uint8_t z = 0; z < NUM_Z; z++) {
                // float d = distance(x, y, z, ballX, ballY, ballZ);
                octoUtils::setPixel(x, y, z, CRGB::White);
            }
        }
        octoUtils::show();
        for (uint8_t x = 0; x < NUM_X; x++) {
            for (uint8_t z = 0; z < NUM_Z; z++) {
                // float d = distance(x, y, z, ballX, ballY, ballZ);
                octoUtils::setPixel(x, y, z, CRGB::Black);
            }
        }
    }
}

void accDisplay() {
    // for (int y = 0; y < NUM_Y; y++){
    //    for (int z = 0; z < NUM_Z; z++){
    for (uint8_t x = 0; x < NUM_X; x++) {
        if (accX / 35 < x) {
            octoUtils::setPixel(x, 0, 0, CRGB::White);
        } else {
            octoUtils::setPixel(x, 0, 0, CRGB::Black);
        }
    }
    //    }
    // }
    octoUtils::show();
}

void theGameOfLifeInit() {
    for (uint8_t y = 0; y < NUM_Y; y++) {
        for (uint8_t x = 0; x < NUM_X; x++) {
            for (uint8_t z = 0; z < NUM_Z; z++) {
                if (z == 9 || z == 11) {
                    if (x == 1 || x == 4) {
                        if (y == 2 || y == 3) {
                            octoUtils::setPixel(x, y, z, CRGB::White);
                        }
                    }
                    if (x == 2 || x == 3) {
                        if (y == 1 || y == 4) {
                            octoUtils::setPixel(x, y, z, CRGB::White);
                        }
                    }
                }
                if (z == 10) {
                    if ((x == 2 || x == 3) && (y == 2 || y == 3)) {
                        octoUtils::setPixel(x, y, z, CRGB::White);
                    }
                }
            }
        }
    }
}

int gameOfLifeNeighbours(int x, int y, int z) {
    int counter = 0;
    for (uint8_t _x = x - 1; _x <= x + 1; _x += 2) {
        for (uint8_t _y = y - 1; _y <= y + 1; _y += 2) {
            for (uint8_t _z = z - 1; _z <= z + 1; _z += 2) {
                if (_x > -1 && _y > -1 && _z > -1 && _x < NUM_X && _y < NUM_Y && _z < NUM_Z && !(_x == x && _y == y && _z == z)) {
                    CRGB* led = octoUtils::getLed(_x, _y, _z);
                    bool lightOn = led->getAverageLight() > 0;
                    if (lightOn) {
                        counter++;
                    }
                }
            }
        }
    }
    return counter;
}

void theGameOfLife() {
    for (uint8_t y = 0; y < NUM_Y; y++) {
        for (uint8_t x = 0; x < NUM_X; x++) {
            for (uint8_t z = 0; z < NUM_Z; z++) {
                int neighbourCount = gameOfLifeNeighbours(x, y, z);
                Serial.println(neighbourCount);
                switch (neighbourCount) {
                    case 3:
                    case 4:
                    case 7:
                        octoUtils::setPixel(x, y, z, CRGB::White);
                        break;
                    case 1:
                        break;
                    default:
                        octoUtils::setPixel(x, y, z, CRGB::Black);
                        break;
                }
            }
        }
    }
    octoUtils::show();
    delay(100);
}

void theGameOfLife2d() {
    int z = 10;
    for (uint8_t y = 0; y < NUM_Y; y++) {
        for (uint8_t x = 0; x < NUM_X; x++) {
            int neighbourCount = gameOfLifeNeighbours(x, y, z);
            Serial.println(neighbourCount);
            switch (neighbourCount) {
                case 2:
                case 3:
                    octoUtils::setPixel(x, y, z, CRGB::White);
                    break;
                case 0:
                case 1:
                    octoUtils::setPixel(x, y, z, CRGB::Black);
                    break;
                default:
                    break;
            }
        }
    }
}

void gameOfLifeNeighbours2d(uint32_t x, uint32_t y, uint32_t z) {
    int counter = 0;
    for (uint32_t _x = x - 1; _x <= x + 1; _x += 2) {
        for (uint32_t _y = y - 1; _y <= y + 1; _y += 2) {
            if (_x >= 0 && _y >= 0 && _x < NUM_X && _y < NUM_Y && !(_x == x && _y == y)) {
                CRGB* led = octoUtils::getLed(_x, _y, z);
                bool lightOn = led->getAverageLight() > 0;
                Serial.println(lightOn);
                if (lightOn) {
                    counter++;
                }
            }
        }
    }
    counter++;
}

int start = 0;
int rotation = 0;
void circle() {
    int centerX = round(NUM_X / 2);
    int centerY = round(NUM_Y / 2);
    // int centerZ = round(NUM_Z / 2);
    int counter = 0;
    for (uint8_t y = 0; y < NUM_Y; y++) {
        for (uint8_t x = 0; x < NUM_X; x++) {
            int manhattanDistance = abs(x - centerX) + abs(y - centerY);
            if (manhattanDistance == 3) {
                if (counter == rotation) {
                    for (uint8_t z = 0; z < NUM_Z; z++) {
                        octoUtils::setPixel(x, y, z, hsl2rgb(start + counter, 255, 255));
                    }
                } else {
                    for (uint8_t z = 0; z < NUM_Z; z++) {
                        octoUtils::setPixel(x, y, z, CRGB::Black);
                    }
                }
                counter += 1;
            } else {
                for (uint8_t z = 0; z < NUM_Z; z++) {
                    octoUtils::setPixel(x, y, z, CRGB::Black);
                }
            }
        }
    }
    start += 3;
    rotation++;
    rotation %= 12;
    octoUtils::show();
}

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
    uint8_t brightness = 255;

    for (uint8_t y = 0; y < NUM_Y; y++) {
        for (uint8_t x = 0; x < NUM_X; x++) {
            for (uint8_t z = 0; z < NUM_Z; z++) {
                // float d = distance(x, y, z, ballX, ballY, ballZ);
                octoUtils::setPixel(x, y, z, ColorFromPalette(CloudColors_p, colorIndex, brightness, LINEARBLEND));
                colorIndex += 3;
            }
        }
    }
}

void globe() {
    if (state != GLOBE) {
        return;
    }
    int radius1 = 1;
    int radius2 = 2;
    int radius3 = 3;
    double m = min(c / 100, 2);
    double rad = t * DEG_TO_RAD * m;

    int xPoint1 = round(3 + cos(rad) * radius1);
    int yPoint1 = round(3 + sin(rad) * radius1);
    int xPoint2 = round(3 + cos(rad) * radius2);
    int yPoint2 = round(3 + sin(rad) * radius2);
    int xPoint3 = round(3 + cos(rad) * radius3);
    int yPoint3 = round(3 + sin(rad) * radius3);

    for (uint8_t x = 0; x < NUM_X; x++) {
        for (uint8_t y = 0; y < NUM_Y; y++) {
            for (uint8_t z = 0; z < NUM_Z; z++) {
                if (x == xPoint3 && y == yPoint3 && 5 < z && z < 14) {
                    octoUtils::setPixel(x, y, z, CRGB::White);
                } else if (x == xPoint2 && y == yPoint2 && (z == 4 || z == 16)) {
                    octoUtils::setPixel(x, y, z, CRGB::White);
                } else if (x == xPoint1 && y == yPoint1 && (z == 1 || z == 18)) {
                    octoUtils::setPixel(x, y, z, CRGB::White);
                } else if (x == 3 && y == 3 && (z == 0 || z == 19)) {
                    octoUtils::setPixel(x, y, z, CRGB::White);
                }
            }
        }
    }
}

void spiral() {
    int radius = 3;
    double m = min(c / 100, 2);
    double rad = t * DEG_TO_RAD * m;

    int xPoint = round(3 + cos(rad) * radius);
    int yPoint = round(3 + sin(rad) * radius);
    int zPoint = (c / 10) % NUM_Z;

    CRGB color = CRGB(100, 100, 100);

    for (uint8_t x = 0; x < NUM_X; x++) {
        for (uint8_t y = 0; y < NUM_Y; y++) {
            for (uint8_t z = 0; z < NUM_Z; z++) {
                if (x == xPoint && y == yPoint && z == zPoint) {
                    octoUtils::setPixel(x, y, z, color);
                }
            }
        }
    }
}

int treeHeight;
int treeX;
int treeY;
boolean treePlanted = false;
int leafLength;
int leafHeight;

int rounds;
int sides;
int previousZone = -1;

uint8_t leafHue;
uint8_t leafDrop;

int treeCount = 0;
uint8_t trees[10][12];
int deadTrees[10][3];
int treeIndex = 0;
int nDeadTrees = 0;

void resetForest() {
    sides = 0;
    rounds = 0;
    nDeadTrees = 0;
    treeIndex = 0;
    treePlanted = false;
    treeCount = 0;
    // for (int i = 0; i < 10; i++) {
    //    plantTree(0, 0, 0);
    //    treeIndex++;
    //    treeIndex %= 10;
    // }
}

const uint8_t TREE_X = 0;
const uint8_t TREE_Y = 1;
const uint8_t TREE_HEIGHT = 2;
const uint8_t LEAF_LENGTH = 3;
const uint8_t LEAF_HEIGHT = 4;
const uint8_t LEAF_HUE = 5;
const uint8_t LEAF_DROP = 6;
const uint8_t TREE_DEAD = 7;
const uint8_t MAX_HEIGHT = 8;
const uint8_t GROWING_SPEED = 9;
const uint8_t TRUNK_COLOR = 10;
const uint8_t SHOULD_DROP = 11;

void plantTree(uint8_t x, uint8_t y, uint8_t maxHeight, boolean growFast = true) {
    trees[treeIndex][TREE_X] = x;
    trees[treeIndex][TREE_Y] = y;
    trees[treeIndex][TREE_HEIGHT] = 0;
    trees[treeIndex][LEAF_LENGTH] = 0;
    trees[treeIndex][LEAF_HEIGHT] = 1;
    trees[treeIndex][LEAF_DROP] = 0;
    trees[treeIndex][TREE_DEAD] = 0;
    trees[treeIndex][MAX_HEIGHT] = maxHeight;
    if (growFast) {
        trees[treeIndex][GROWING_SPEED] = 2;
        trees[treeIndex][LEAF_HUE] = 96;
        trees[treeIndex][TRUNK_COLOR] = 30;
        trees[treeIndex][SHOULD_DROP] = 1;
    } else {
        trees[treeIndex][GROWING_SPEED] = 1000;
        trees[treeIndex][LEAF_HUE] = 120;
        trees[treeIndex][TRUNK_COLOR] = 85;
        trees[treeIndex][SHOULD_DROP] = 0;
    }

    treeHeight = 0;
    treeX = x;
    treeY = y;
    leafLength = 0;
    leafHeight = 1;
    treePlanted = true;

    rounds = 0;
    sides = 0;
    previousZone = -1;
    leafHue = 96;
    leafDrop = 0;

    treeCount++;
    treeIndex++;
}

void growTree(int maxTreeHeight) {
    for (int i = 0; i < max(10, treeCount); i++) {
        if (c % trees[i][GROWING_SPEED] == 0) {
            trees[i][TREE_HEIGHT]++;
            trees[i][TREE_HEIGHT] = min(maxTreeHeight, trees[i][TREE_HEIGHT]);
        }
    }
}

void renderTree(int maxTreeHeight) {
    for (int i = 0; i < min(treeCount, 10); i++) {
        int tX = trees[i][TREE_X];
        int tY = trees[i][TREE_Y];
        int tHeight = trees[i][TREE_HEIGHT];
        int leafLen = trees[i][LEAF_LENGTH];
        int leafDrp = trees[i][LEAF_DROP];
        int lHeight = trees[i][LEAF_HEIGHT];
        int tHue = trees[i][TRUNK_COLOR];
        Serial.println(tHue);
        const int trunkX = min(NUM_X, max(0, tX));
        const int trunkY = min(NUM_Y, max(0, tY + offsetY));
        for (int z = 0; z <= tHeight; z++) {
            octoUtils::setPixel(trunkX, trunkY, NUM_Z - 1 - z, hsl2rgb(tHue, 100, 50));
        }
        if (leafLen > 0) {
            int zTop = max(NUM_Z - 1 - maxTreeHeight + leafDrp, 0);
            int zBottom = min(zTop + lHeight + leafDrp, NUM_Z);
            int _xMin = max(trunkX - leafLen, 0);
            int _yMin = max(trunkY - leafLen, 0);
            int _xMax = min(trunkX + leafLen, NUM_X - 1);
            int _yMax = min(trunkY + leafLen, NUM_Y - 1);
            for (int x = 0; x <= _xMax; x++) {
                for (int y = 0; y <= _yMax; y++) {
                    for (int z = zTop; z < zBottom; z++) {
                        if (x >= _xMin && y >= _yMin && (z == zTop || x == _xMin || x == _xMax || y == _yMin || y == _yMax)) {
                            // octoUtils::setPixel(x, y, z, 30, 175, 30);
                            octoUtils::setPixel(x, y, z, hsl2rgb(trees[i][LEAF_HUE], 100, 50));
                        }
                    }
                }
            }
        }
    }
    if (offsetY > 0) {
        offsetY--;
    }
}

void growLeaves(int maxTreeHeight, int maxLeafLength, int interval) {
    const int finalLeafHue = 10;
    for (int i = 0; i < treeCount; i++) {
        int tHeight = trees[i][TREE_HEIGHT];
        if (tHeight == maxTreeHeight - 1 && c % interval == 0) {
            trees[i][LEAF_LENGTH]++;
            trees[i][LEAF_LENGTH] = min(trees[i][LEAF_LENGTH], maxLeafLength);
        }
        if (trees[i][LEAF_LENGTH] == maxLeafLength - 1 && c % interval == 0) {
            trees[i][LEAF_HEIGHT]++;
            trees[i][LEAF_HEIGHT] = min(trees[i][LEAF_HEIGHT], 10);
        }

        if (trees[i][LEAF_HEIGHT] == 10 && trees[i][SHOULD_DROP] && c % interval == 0) {
            trees[i][LEAF_HUE] = max(finalLeafHue, trees[i][LEAF_HUE] - 1);
        }

        if (trees[i][LEAF_HUE] == finalLeafHue && c % interval == 0) {
            trees[i][LEAF_DROP]++;
            trees[i][LEAF_DROP] = min(trees[i][LEAF_DROP], tHeight);
        }

        if (trees[i][LEAF_DROP] == maxTreeHeight && !trees[i][TREE_DEAD]) {
            trees[i][TREE_DEAD] = 1;
            nDeadTrees++;
        }
    }
}

void registerTreeSoul(int x, int y, int leafLength);
void eruptTreeSouls();

void playWithTrees() {
    if (state != TREE) {
        return;
    }
    if (treeCount == 0) {
        plantTree(3, 3, NUM_Z - 1);
    }
    // else if (treeCount == 1 && trees[0][TREE_DEAD]) {
    //    plantTree(1,1, NUM_Z - 4);
    // } else if (treeCount == 2 && trees[1][TREE_DEAD]) {
    //    plantTree(5, 1, NUM_Z - 4);
    // } else if (treeCount == 3 && trees[2][TREE_DEAD]) {
    //    plantTree(5, 5, NUM_Z - 4);
    // } else if (treeCount == 4 && trees[3][TREE_DEAD]) {
    //    plantTree(1, 5, NUM_Z - 4);
    // }

    if (treeCount > 0) {
        growTree(NUM_Z - 1);
        growLeaves(NUM_Z - 1, 1, 2);
        renderTree(NUM_Z - 1);
        eruptTreeSouls();
        for (int i = 0; i < min(treeCount, 10); i++) {
            uint8_t treeDead = trees[i][TREE_DEAD];
            if (treeDead == 1) {
                uint8_t tX = trees[i][TREE_X];
                uint8_t tY = trees[i][TREE_Y];
                uint8_t lLength = trees[i][LEAF_LENGTH];
                registerTreeSoul(tX, tY, lLength);
            }
        }
    }
}

int treeSouls[NUM_X][NUM_Y];

void registerTreeSoul(int x, int y, int leafLength) {
    int xStart = max(x - leafLength, 0);
    int yStart = max(y - leafLength, 0);
    int xEnd = min(x + leafLength, NUM_X);
    int yEnd = min(y + leafLength, NUM_Y);

    int treeX = random(xStart, xEnd);
    int treeY = random(yStart, yEnd);
    if (treeSouls[treeX][treeY] + NUM_Z < c) {
        treeSouls[treeX][treeY] = c + random(300);
    }
}

void eruptTreeSouls() {
    for (uint8_t x = 0; x < NUM_X; x++) {
        for (uint8_t y = 0; y < NUM_Y; y++) {
            for (uint8_t z = 0; z < NUM_Z; z++) {
                if (NUM_Z - (c - treeSouls[x][y]) == z) {
                    octoUtils::setPixel(x, y, z, CRGB::White);
                }
            }
        }
    }
}

void treeDancing() {
    const int FRONT = 0;
    const int RIGHT = 1;
    const int BOTTOM = 2;
    const int LEFT = 3;
    const int MIDDLE = 4;

    const uint8_t intBallX = round(ballX);
    const uint8_t intBallY = round(ballY);
    int currentZone;
    if (intBallY <= 1 && intBallX > 1) {
        currentZone = FRONT;
    } else if (intBallX >= NUM_X - 2 && intBallY > 1) {
        currentZone = RIGHT;
    } else if (intBallY >= NUM_Y - 2 && intBallX < NUM_X - 2) {
        currentZone = BOTTOM;
    } else if (intBallX <= 1 && intBallY < NUM_Y - 2) {
        currentZone = LEFT;
    } else {
        currentZone = MIDDLE;
    }
    if (currentZone == MIDDLE) {
        rounds = 0;
        sides = 0;
    } else if ((currentZone - previousZone) % 4 == 1) {
        sides++;
    }
    if (sides == 4) {
        sides = 0;
        rounds++;
        if (treeCount == 1) {
            plantTree(1, 1, NUM_Z - 1, false);
        } else if (treeCount == 2) {
            plantTree(1, NUM_Y - 2, NUM_Z - 1, false);
        } else if (treeCount == 3) {
            plantTree(NUM_X - 2, 1, NUM_Z - 1, false);
        } else if (treeCount == 4) {
            plantTree(NUM_X - 2, NUM_Y - 2, NUM_Z - 1, false);
        }
    }
    if (rounds == 3) {
        portalOpen = true;
    }

    previousZone = currentZone;
}
