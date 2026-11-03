// ---------------------------------------------------------------------------
// Guitar tuner pedal for the Waveshare RP2040-Zero.
//
// Board package: "Raspberry Pi Pico/RP2040" by Earle Philhower
// Board:         "Waveshare RP2040 Zero"
//
// Every frame (68 ms of audio) goes through the same steps:
//   audio.h      capture the guitar signal with the ADC and DMA
//   pitch.h      find its frequency (YIN) and turn that into a note + cents
//
// Settings live in config.h. Open the Serial Monitor at 115200 baud to see
// every reading.
// ---------------------------------------------------------------------------
#include <Arduino.h>

#include "config.h"
#include "pitch.h"
#include "audio.h"

static PitchDetector detector;
static int16_t frame[kFrameLen];

// e.g. "E2  82.38 Hz  -0.6 cents  (level 412, clarity 0.97, 38 ms)"
static void printReading(float freq, int level, uint32_t calcMs) {
  int note;
  float cents;
  freqToNote(freq, A4_REF_HZ, &note, &cents);
  Serial.printf("%s%d\t%.2f Hz\t%+.1f cents\t(level %d, clarity %.2f, %lu ms)\n",
                kNoteNames[notePitchClass(note)], noteOctave(note), freq, cents, level,
                detector.clarity(), (unsigned long)calcMs);
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
  startAudioCapture();
}

void loop() {
  const uint16_t* raw = waitForAudio();

  uint32_t startMs = millis();
  int level = prepareFrame(raw, frame, kFrameLen, OVERSAMPLE);
  float freq = 0;
  if (level >= MIN_SIGNAL_P2P) freq = detector.detect(frame, SAMPLE_RATE, kMinLag, YIN_THRESHOLD);
  uint32_t calcMs = millis() - startMs;

  if (freq > 0) {
    printReading(freq, level, calcMs);
  } else {
    printSilence(level);
  }
}
