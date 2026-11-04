// ---------------------------------------------------------------------------
// Output: the 7-segment note display and the seven meter LEDs.
// ---------------------------------------------------------------------------
#pragma once
#include <Arduino.h>

#include "config.h"
#include "pitch.h"

// ----- 7-segment display ----------------------------------------------------
//
// One bit per segment:
//
//      aaa
//     f   b
//      ggg
//     e   c
//      ddd  dp
static const uint8_t kSegA = 1 << 0, kSegB = 1 << 1, kSegC = 1 << 2, kSegD = 1 << 3,
                     kSegE = 1 << 4, kSegF = 1 << 5, kSegG = 1 << 6, kSegDot = 1 << 7;
static const uint8_t kAllSegments = 0xFF;

static const uint8_t kLetterA = kSegA | kSegB | kSegC | kSegE | kSegF | kSegG;
static const uint8_t kLetterB = kSegC | kSegD | kSegE | kSegF | kSegG;  // lower-case b
static const uint8_t kLetterC = kSegA | kSegD | kSegE | kSegF;
static const uint8_t kLetterD = kSegB | kSegC | kSegD | kSegE | kSegG;  // lower-case d
static const uint8_t kLetterE = kSegA | kSegD | kSegE | kSegF | kSegG;
static const uint8_t kLetterF = kSegA | kSegE | kSegF | kSegG;
static const uint8_t kLetterG = kSegA | kSegC | kSegD | kSegE | kSegF;

// What to show for each pitch class, C first. A sharp is its letter plus the dot.
static const uint8_t kNoteGlyphs[12] = {
    kLetterC, kLetterC | kSegDot, kLetterD, kLetterD | kSegDot, kLetterE, kLetterF,
    kLetterF | kSegDot, kLetterG, kLetterG | kSegDot, kLetterA, kLetterA | kSegDot, kLetterB};

static constexpr bool kHasDisplay = SEG_DATA_PIN >= 0;

static void showSegments(uint8_t segments) {
  if (!kHasDisplay) return;

  // Common anode: a segment lights when its output is LOW.
  uint8_t outputs = SEG_COMMON_ANODE ? (uint8_t)~segments : segments;

  // Only shift when something changed. The 74HC164 has no output latch, so the
  // segments flicker for a few microseconds while the bits move through.
  static int lastOutputs = -1;
  if (outputs == lastOutputs) return;
  lastOutputs = outputs;

  // MSB first, so after 8 clocks bit 0 (segment a) sits on the first output.
  shiftOut(SEG_DATA_PIN, SEG_CLOCK_PIN, MSBFIRST, outputs);
}

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

// ----- Whole display --------------------------------------------------------

static void showNote(int midiNote, float cents) {
  showSegments(kNoteGlyphs[notePitchClass(midiNote)]);
  showCents(cents);
}

static void showNothing() {
  showSegments(0);
  setMeterLeds(kNoLed, false, kNoLed);
}

static void showEverything() {
  showSegments(kAllSegments);
  for (uint8_t pin : FLAT_PINS) digitalWrite(pin, HIGH);
  for (uint8_t pin : SHARP_PINS) digitalWrite(pin, HIGH);
  digitalWrite(GREEN_PIN, HIGH);
}

static void setupDisplay() {
  if (kHasDisplay) {
    pinMode(SEG_DATA_PIN, OUTPUT);
    pinMode(SEG_CLOCK_PIN, OUTPUT);
  }
  for (uint8_t pin : FLAT_PINS) pinMode(pin, OUTPUT);
  for (uint8_t pin : SHARP_PINS) pinMode(pin, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);

  // The shift register powers up with random segments lit.
  showNothing();
}
