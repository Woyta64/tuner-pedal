// ---------------------------------------------------------------------------
// Tuner pedal settings. Pins, display type and tuning behaviour all live here,
// so nothing else needs editing to adapt the firmware to your build.
//
// The pin values are for the PCB in hardware/. The breadboard build uses a
// different LED order and has no latch or footswitch: see the notes below.
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
//   PCB:         GP6 L3  GP5 L2  GP4 L1  GP3 green  GP2 R1  GP1 R2  GP0 R3
//   Breadboard:  GP0 L3  GP1 L2  GP2 L1  GP3 green  GP4 R1  GP5 R2  GP6 R3
//                -> FLAT_PINS = {2, 1, 0}, SHARP_PINS = {4, 5, 6}
//
// The PCB order is mirrored so the seven LED traces don't cross.
static const uint8_t FLAT_PINS[3] = {4, 5, 6};
static const uint8_t SHARP_PINS[3] = {2, 1, 0};
static const uint8_t GREEN_PIN = 3;  // "in tune", above the display

// ----- 7-segment display
// BS-A554RD, driven through a shift register so it only needs 2-3 pins.
// Shift register outputs 1..8 (QA..QH on a 74HC595, Q0..Q7 on a 74HC164) go to
// segments a, b, c, d, e, f, g, dp. Set SEG_DATA_PIN to -1 if there's no display.
#define SEG_DATA_PIN 7   // 74HC595 pin 14 (SER)   / 74HC164 pins 1 + 2 (A, B)
#define SEG_CLOCK_PIN 8  // 74HC595 pin 11 (SRCLK) / 74HC164 pin 8 (CLK)

// 74HC595 pin 12 (RCLK). The outputs only change when this pulses, so the
// segments don't flicker while the bits shift in.
// Breadboard (74HC164, which has no latch): -1.
#define SEG_LATCH_PIN 14

// true  = common anode: common pin to 3V3, a segment lights when its output is LOW.
// false = common cathode: common pin to GND, a segment lights when its output is HIGH.
// The BS-A554RD is common anode. If the self-test lights everything except the
// expected segment, flip this.
#define SEG_COMMON_ANODE true

// ----- Footswitch
// The 3PDT's third pole pulls this pin to GND while the pedal is on (tuning).
// In bypass the display and LEDs stay dark. Without the switch wired up, short
// J2.2 (ENGAGE) to J2.3 (GND) on the PCB.
// Breadboard (no switch): -1, the tuner is always on.
#define ENGAGE_PIN 15

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

// How quickly the meter follows new readings (0..1, higher = faster but jumpier).
#define CENTS_SMOOTHING 0.4f
// A new note must be heard this many frames in a row before the display switches to it.
#define NOTE_CONFIRM_FRAMES 2
// Keep showing the last note this long after the string stops ringing.
#define HOLD_MS 1500
