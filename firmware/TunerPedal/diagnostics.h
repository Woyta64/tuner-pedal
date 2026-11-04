// ---------------------------------------------------------------------------
// Wiring checks: the boot self-test and the Serial Monitor test commands.
// ---------------------------------------------------------------------------
#pragma once
#include <Arduino.h>

#include "display.h"
#include "pitch.h"

// Lights every segment and LED in turn so you can check the wiring. Runs at boot.
static void runSelfTest() {
  if (kHasDisplay) {
    // Segments a..g and the dot, one at a time.
    for (int bit = 0; bit < 8; ++bit) {
      showSegments(1 << bit);
      delay(150);
    }
    // Every letter the tuner can show.
    static const uint8_t kLetters[7] = {kLetterC, kLetterD, kLetterE, kLetterF,
                                        kLetterG, kLetterA, kLetterB};
    for (uint8_t letter : kLetters) {
      showSegments(letter);
      delay(300);
    }
    showSegments(0);
  }

  // Meter LEDs from left to right: L3 L2 L1 green R1 R2 R3.
  for (int led = 2; led >= 0; --led) {
    setMeterLeds(led, false, kNoLed);
    delay(150);
  }
  setMeterLeds(kNoLed, true, kNoLed);
  delay(150);
  for (int led = 0; led <= 2; ++led) {
    setMeterLeds(kNoLed, false, led);
    delay(150);
  }

  // Everything at once: nothing should go dim when all LEDs share the supply.
  showEverything();
  delay(600);
  showNothing();
}

// 'm': fake readings from -35 to +35 cents, one meter LED each.
static void runMeterSweep() {
  static const float kCents[7] = {-35, -18, -7, 0, 7, 18, 35};
  static const char* const kExpected[7] = {"L3", "L2", "L1", "green", "R1", "R2", "R3"};
  for (int i = 0; i < 7; ++i) {
    Serial.printf("meter %+.0f cents -> %s\n", kCents[i], kExpected[i]);
    showNote(69, kCents[i]);  // A4
    delay(1000);
  }
  showNothing();
}

// 'n': every note from C to B. Sharps light the dot.
static void runNoteSweep() {
  for (int pitchClass = 0; pitchClass < 12; ++pitchClass) {
    Serial.printf("display -> %s\n", kNoteNames[pitchClass]);
    showNote(60 + pitchClass, 0);  // C4 + pitchClass, in tune
    delay(1000);
  }
  showNothing();
}

// 'd': all off, all on, then a few single segments, 3 seconds each.
static void runDisplayCheck() {
  static const uint8_t kPatterns[5] = {0, kAllSegments, kSegA, kSegG, kSegDot};
  static const char* const kExpected[5] = {"all OFF", "all ON", "only a (top)",
                                           "only g (middle)", "only the dot"};
  for (int i = 0; i < 5; ++i) {
    Serial.printf("display -> %s\n", kExpected[i]);
    showSegments(kPatterns[i]);
    delay(3000);
  }
  showNothing();
}

// Serial Monitor commands, handy while wiring:
//   t  run the self-test again        a  everything on for 5 seconds
//   m  meter sweep                    n  note sweep
//   d  display check
static void handleSerialCommands() {
  while (Serial.available()) {
    switch (Serial.read()) {
      case 't':
        Serial.println("self-test: segments, letters, L3 L2 L1 green R1 R2 R3, then all");
        runSelfTest();
        break;
      case 'a':
        Serial.println("everything on for 5 s");
        showEverything();
        delay(5000);
        showNothing();
        break;
      case 'm':
        runMeterSweep();
        break;
      case 'n':
        runNoteSweep();
        break;
      case 'd':
        runDisplayCheck();
        break;
    }
  }
}
