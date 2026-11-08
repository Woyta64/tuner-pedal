// ---------------------------------------------------------------------------
// Guitar tuner pedal for the Waveshare RP2040-Zero.
//
// Board package: "Raspberry Pi Pico/RP2040" by Earle Philhower
// Board:         "Waveshare RP2040 Zero"
//
// Every frame (68 ms of audio) goes through the same steps:
//   audio.h      capture the guitar signal with the ADC and DMA
//   pitch.h      find its frequency (YIN) and turn that into a note + cents
//   tracker.h    smooth the reading and hold the last note
//   display.h    show the note and light the meter LEDs
//
// Settings live in config.h. Open the Serial Monitor at 115200 baud to see
// every reading and to use the test commands in diagnostics.h.
// ---------------------------------------------------------------------------
#include <Arduino.h>

#include "config.h"
#include "pitch.h"
#include "audio.h"
#include "tracker.h"
#include "display.h"
#include "diagnostics.h"

static PitchDetector detector;
static int16_t frame[kFrameLen];
static NoteTracker tracker;

// The footswitch pulls ENGAGE_PIN low while the pedal is on.
static bool tunerEngaged() {
  if (ENGAGE_PIN < 0) return true;  // no footswitch: always on
  return digitalRead(ENGAGE_PIN) == LOW;
}

// e.g. "E2  82.38 Hz  -0.6 cents  (shown -0.8, level 412, clarity 0.97, 38 ms)"
static void printReading(float freq, int level, uint32_t calcMs) {
  int note;
  float cents;
  freqToNote(freq, A4_REF_HZ, &note, &cents);
  Serial.printf("%s%d\t%.2f Hz\t%+.1f cents\t(shown %+.1f, level %d, clarity %.2f, %lu ms)\n",
                kNoteNames[notePitchClass(note)], noteOctave(note), freq, cents, tracker.cents(),
                level, detector.clarity(), (unsigned long)calcMs);
}

// Prints the signal level about once a second while there's nothing to tune.
// The "quiet" level is the noise floor, which is what MIN_SIGNAL_P2P has to beat.
static void printSilence(int level) {
  static uint32_t lastPrintMs = 0;
  if (millis() - lastPrintMs < 1000) return;
  lastPrintMs = millis();
  Serial.printf("%s, level %d\n", level >= MIN_SIGNAL_P2P ? "no clear pitch" : "quiet", level);
}

void setup() {
  Serial.begin(115200);
  setupDisplay();
  if (ENGAGE_PIN >= 0) pinMode(ENGAGE_PIN, INPUT_PULLUP);
  runSelfTest();
  startAudioCapture();
}

void loop() {
  const uint16_t* raw = waitForAudio();
  handleSerialCommands();

  if (!tunerEngaged()) {
    tracker.reset();
    showNothing();
    return;
  }

  uint32_t startMs = millis();
  int level = prepareFrame(raw, frame, kFrameLen, OVERSAMPLE);
  float freq = 0;
  if (level >= MIN_SIGNAL_P2P) freq = detector.detect(frame, SAMPLE_RATE, kMinLag, YIN_THRESHOLD);
  uint32_t calcMs = millis() - startMs;

  tracker.update(freq, millis());
  if (tracker.hasNote()) {
    showNote(tracker.note(), tracker.cents());
  } else {
    showNothing();
  }

  if (freq > 0) {
    printReading(freq, level, calcMs);
  } else {
    printSilence(level);
  }
}
