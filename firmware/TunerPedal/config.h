// ---------------------------------------------------------------------------
// Tuner pedal settings. Pins, display type and tuning behaviour all live here,
// so nothing else needs editing to adapt the firmware to your build.
// ---------------------------------------------------------------------------
#pragma once
#include <stdint.h>

// ===== Pins ================================================================
// RP2040-Zero GPIO numbers ("GPx" on the silkscreen). GP9-GP13 sit on the
// bottom edge of the module and can't reach a breadboard, so they are unused.

// ----- Meter LEDs
// Three red "flat" LEDs left of the display and three red "sharp" LEDs right of
// it, each listed from the one next to the display outwards.
//
//   GP0 L3  GP1 L2  GP2 L1  GP3 green  GP4 R1  GP5 R2  GP6 R3
static const uint8_t FLAT_PINS[3] = {2, 1, 0};
static const uint8_t SHARP_PINS[3] = {4, 5, 6};
static const uint8_t GREEN_PIN = 3;  // "in tune", above the display

// ----- Audio input
// Must be an ADC pin: GP26..GP29 (ADC0..ADC3).
#define AUDIO_PIN 26

// ===== Pitch detection ======================================================

// The ADC runs at SAMPLE_RATE * OVERSAMPLE, and each group of OVERSAMPLE
// readings is summed into one sample: a cheap low-pass filter that also adds
// about one bit of resolution.
#define SAMPLE_RATE 20000
#define OVERSAMPLE 4

// Lowest and highest note we look for. 60 Hz covers drop tunings and 7-strings.
#define MIN_FREQ 60
#define MAX_FREQ 1200

// Samples compared per lag. 1024 at 20 kHz = 51 ms, three periods of the lowest note.
#define YIN_WINDOW 1024

// YIN "aperiodicity" threshold. Lower is stricter: fewer wrong readings, but the
// tuner gives up sooner as the string decays. 0.10-0.20 is the sensible range.
#define YIN_THRESHOLD 0.15f

// Minimum peak-to-peak signal, in raw ADC steps (0.8 mV each), before we try
// to detect a pitch at all. Raise it if notes flicker with nothing plugged in:
// the Serial Monitor prints the current level ("quiet, level N") to help.
#define MIN_SIGNAL_P2P 30

// ===== Tuning behaviour =====================================================

#define A4_REF_HZ 440.0f

// Within this many cents: green LED.
#define IN_TUNE_CENTS 3.0f
// Up to STEP1: inner red LED. Up to STEP2: middle red LED. Beyond: outer red LED.
#define METER_STEP1_CENTS 12.0f
#define METER_STEP2_CENTS 25.0f
