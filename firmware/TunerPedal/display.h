// ---------------------------------------------------------------------------
// Output: the seven meter LEDs.
// ---------------------------------------------------------------------------
#pragma once
#include <Arduino.h>

#include "config.h"

// ----- Meter LEDs -----------------------------------------------------------

static const int kNoLed = -1;

// Lights the given LEDs and turns the rest off. flatLed and sharpLed count
// from the display outwards (0 = inner, 2 = outer), or kNoLed.
static void setMeterLeds(int flatLed, bool green, int sharpLed) {
  for (int i = 0; i < 3; ++i) {
    digitalWrite(FLAT_PINS[i], i == flatLed ? HIGH : LOW);
    digitalWrite(SHARP_PINS[i], i == sharpLed ? HIGH : LOW);
  }
  digitalWrite(GREEN_PIN, green ? HIGH : LOW);
}

// Which red LED a reading this far off lights: 0 = inner, 1 = middle, 2 = outer.
static int meterLedFor(float centsOff) {
  if (centsOff < METER_STEP1_CENTS) return 0;
  if (centsOff < METER_STEP2_CENTS) return 1;
  return 2;
}

static void showCents(float cents) {
  float off = fabsf(cents);
  if (off <= IN_TUNE_CENTS) {
    setMeterLeds(kNoLed, true, kNoLed);
  } else if (cents < 0) {
    setMeterLeds(meterLedFor(off), false, kNoLed);
  } else {
    setMeterLeds(kNoLed, false, meterLedFor(off));
  }
}

static void showNothing() {
  setMeterLeds(kNoLed, false, kNoLed);
}

static void setupDisplay() {
  for (uint8_t pin : FLAT_PINS) pinMode(pin, OUTPUT);
  for (uint8_t pin : SHARP_PINS) pinMode(pin, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  showNothing();
}
