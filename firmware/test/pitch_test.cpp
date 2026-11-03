// PC test for the pitch detector: feeds synthetic guitar-like signals through the
// exact same code the pedal runs and reports the error in cents.
//
//   g++ -O2 -std=c++17 -I../TunerPedal pitch_test.cpp -o pitch_test && ./pitch_test
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

#include "config.h"
#include "pitch.h"

constexpr int kMaxLag = SAMPLE_RATE / MIN_FREQ + 2;
using Detector = Yin<YIN_WINDOW, kMaxLag>;
constexpr int kFrameLen = Detector::kInputLen;
constexpr int kRawLen = kFrameLen * OVERSAMPLE;

// Worst allowed error for a single frame, before the firmware's smoothing.
// On inharmonic (real-string-like) tones about 1.4 cents of this is the string
// itself: the overtones are slightly sharp and pull the waveform's period.
constexpr float kPassCents = 3.0f;

struct Tone {
  float freq;
  float amplitude;      // peak, in ADC steps
  float harmonics[8];   // relative amplitude of harmonic 1..8
  float noise;          // RMS noise, in ADC steps
  float inharmonicity;  // B in f_h = h * f0 * sqrt(1 + B h^2); steel strings ~0.0001
};

// Simulates what the ADC would capture: 12-bit samples around mid-rail.
static std::vector<uint16_t> capture(const Tone& t, std::mt19937& rng) {
  std::normal_distribution<float> noise(0.0f, t.noise);
  std::uniform_real_distribution<float> phase(0.0f, 6.2831853f);
  float phases[8];
  for (float& p : phases) p = phase(rng);
  const double fs = (double)SAMPLE_RATE * OVERSAMPLE;
  std::vector<uint16_t> raw(kRawLen);
  for (int i = 0; i < kRawLen; ++i) {
    double v = 0;
    for (int h = 0; h < 8; ++h) {
      // slight inharmonicity, like a real steel string: f_h = h * f0 * sqrt(1 + B h^2)
      double fh = t.freq * (h + 1) * sqrt(1.0 + t.inharmonicity * (h + 1) * (h + 1));
      v += t.harmonics[h] * sin(6.283185307 * fh * i / fs + phases[h]);
    }
    int s = (int)lround(2048.0 + t.amplitude * v + noise(rng));
    raw[i] = (uint16_t)(s < 0 ? 0 : s > 4095 ? 4095 : s);
  }
  return raw;
}

int main() {
  std::mt19937 rng(1234);
  Detector yin;
  std::vector<int16_t> frame(kFrameLen);
  const int minLag = SAMPLE_RATE / MAX_FREQ;

  const float bright[8] = {1.0f, 0.6f, 0.4f, 0.3f, 0.2f, 0.15f, 0.1f, 0.05f};
  // Bridge pickup on a low string: 2nd/3rd harmonic louder than the fundamental.
  const float weakFund[8] = {0.3f, 1.0f, 0.8f, 0.5f, 0.4f, 0.2f, 0.1f, 0.1f};
  const float sine[8] = {1.0f};

  struct Case { const char* name; const float* harm; float amp; float noise; float inharm; };
  const Case cases[] = {
      {"ideal harmonics", bright, 300, 2, 0},
      {"bright, strong", bright, 300, 2, 0.0001f},
      {"weak fundamental", weakFund, 200, 2, 0.0001f},
      {"pure sine, quiet", sine, 25, 2, 0},
      {"bright, noisy", bright, 80, 6, 0.0001f},
  };
  // Open strings E2 A2 D3 G3 B3 E4, plus some extremes.
  const float notes[] = {61.74f, 73.42f, 82.41f, 110.0f, 146.83f, 196.0f,
                         246.94f, 329.63f, 440.0f, 659.26f, 987.77f};
  const float detunes[] = {-30, -7, -1.5f, 0, 1.5f, 7, 30};

  int failures = 0;
  for (const Case& c : cases) {
    printf("\n== %s (amplitude %.0f, noise %.0f)\n", c.name, c.amp, c.noise);
    for (float base : notes) {
      float worst = 0;
      int wrongNote = 0, misses = 0;
      int expectedNote = 0;
      for (float detune : detunes) {
        for (int trial = 0; trial < 5; ++trial) {
          Tone t{base * powf(2.0f, detune / 1200.0f), c.amp, {}, c.noise, c.inharm};
          for (int h = 0; h < 8; ++h) t.harmonics[h] = c.harm[h];
          auto raw = capture(t, rng);
          int p2p = prepareFrame(raw.data(), frame.data(), kFrameLen, OVERSAMPLE);
          if (p2p < MIN_SIGNAL_P2P) { ++misses; continue; }
          float f = yin.detect(frame.data(), SAMPLE_RATE, minLag, YIN_THRESHOLD);
          if (f <= 0) { ++misses; continue; }
          int note, refNote;
          float cents, refCents;
          freqToNote(f, A4_REF_HZ, &note, &cents);
          // the truth is the frequency of the fundamental partial
          float f1 = t.freq * sqrtf(1.0f + t.inharmonicity);
          freqToNote(f1, A4_REF_HZ, &refNote, &refCents);
          expectedNote = refNote;
          if (note != refNote) { ++wrongNote; continue; }
          float err = fabsf(cents - refCents);
          if (err > worst) worst = err;
        }
      }
      const int total = 5 * (int)(sizeof(detunes) / sizeof(detunes[0]));
      bool ok = worst <= kPassCents && wrongNote == 0 && misses == 0;
      if (!ok) ++failures;
      printf("  %7.2f Hz  %-2s%d  worst error %5.2f cents  wrong note %2d/%d  no reading %2d/%d  %s\n",
             base, kNoteNames[notePitchClass(expectedNote)], noteOctave(expectedNote), worst,
             wrongNote, total, misses, total, ok ? "ok" : "<-- check");
    }
  }
  printf("\n%d case(s) over %.1f cents / with wrong notes or misses\n", failures, kPassCents);
  return failures ? 1 : 0;
}
