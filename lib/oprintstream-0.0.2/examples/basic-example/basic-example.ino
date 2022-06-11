// "Copyright 2021 Francisco Aguilera"

#include <Arduino.h>

#include <oprintstream.hpp>

using namespace falven::ad;

// Our printing object.
OPrintStream<> print(USBDevice);

void setup() {
  // Your typical Serial initialization.
  print.begin(9600UL);
  while (!print) {
  }

  // Which looks better, more succint, and extendable!?
  print.print("Testing... ");
  print.print(1, BIN);
  print.print(" ");
  print.print(2, BIN);
  print.println();
  print.flush();

  // I think you guessed correctly...
  print << bin << "Testing... " << 1 << " " << 2 << endl;
}

void loop() {}
