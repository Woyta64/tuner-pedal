// ---------------------------------------------------------------------------
// Pitch detection (YIN algorithm) and note math.
//
// Plain C++ with no Arduino dependencies, so the same code can be tested on a
// PC (see ../test/pitch_test.cpp).
// ---------------------------------------------------------------------------
#pragma once
#include <math.h>
#include <stdint.h>

// Turns a block of raw 12-bit ADC samples into a DC-free signal, summing each
// group of `oversample` samples into one. `out` gets `outLen` samples, `raw`
// must hold outLen * oversample. Returns the peak-to-peak level in raw ADC steps.
inline int prepareFrame(const uint16_t* raw, int16_t* out, int outLen, int oversample) {
  int32_t total = 0;
  int32_t lo = INT32_MAX, hi = INT32_MIN;
  for (int i = 0; i < outLen; ++i) {
    int32_t s = 0;
    for (int k = 0; k < oversample; ++k) s += raw[i * oversample + k] & 0x0FFF;
    out[i] = (int16_t)s;
    total += s;
    if (s < lo) lo = s;
    if (s > hi) hi = s;
  }
  int16_t mean = (int16_t)(total / outLen);
  for (int i = 0; i < outLen; ++i) out[i] -= mean;
  return (hi - lo) / oversample;
}

// YIN pitch detector (de Cheveigné & Kawahara, 2002).
//   kWindow: samples compared per lag
//   kMaxLag: longest period we look for, in samples (= sampleRate / minFreq)
// The difference function is pure integer math, which matters on the RP2040's
// Cortex-M0+ (no FPU). Only a handful of float ops happen per lag.
template <int kWindow, int kMaxLag>
class Yin {
 public:
  // Number of samples detect() needs.
  static constexpr int kInputLen = kWindow + kMaxLag + 1;

  // Returns the detected frequency in Hz, or 0 if the signal has no clear pitch.
  // `x` must hold kInputLen DC-free samples, each within +/-16383.
  float detect(const int16_t* x, float sampleRate, int minLag, float threshold) {
    uint64_t runningSum = 0;
    int best = -1;
    cmndf_[0] = 1.0f;
    for (int tau = 1; tau <= kMaxLag; ++tau) {
      // d(tau) = sum of squared differences between the signal and itself shifted by tau.
      uint64_t d = difference(x, tau);
      // Cumulative mean normalized difference: d(tau) / mean(d(1..tau)).
      runningSum += d;
      cmndf_[tau] = runningSum ? (float)d * (float)tau / (float)runningSum : 1.0f;

      if (tau < minLag) continue;
      if (best < 0) {
        if (cmndf_[tau] < threshold) best = tau;  // first dip under the threshold
      } else if (cmndf_[tau] < cmndf_[best]) {
        best = tau;                               // still going down into the dip
      } else {
        break;                                    // bottom of the dip found
      }
    }
    if (best < 0 || best >= kMaxLag) {
      clarity_ = 0.0f;
      return 0.0f;
    }

    clarity_ = 1.0f - cmndf_[best];
    float period = (float)best + parabolaShift(cmndf_[best - 1], cmndf_[best], cmndf_[best + 1]);
    return sampleRate / refinePeriod(x, period);
  }

  // 0..1, how periodic the last detected signal was (1 = perfectly periodic).
  float clarity() const { return clarity_; }

 private:
  static uint64_t difference(const int16_t* x, int tau) {
    uint64_t d = 0;
    for (int j = 0; j < kWindow; ++j) {
      int32_t diff = x[j] - x[j + tau];
      d += (uint32_t)(diff * diff);
    }
    return d;
  }

  // Offset of a parabola's minimum from the middle of three equally spaced points.
  static float parabolaShift(float y0, float y1, float y2) {
    float denom = y0 - 2.0f * y1 + y2;
    float shift = denom != 0.0f ? 0.5f * (y0 - y2) / denom : 0.0f;
    return shift > 1.0f ? 1.0f : shift < -1.0f ? -1.0f : shift;
  }

  // High notes have short periods (e.g. 20 samples at 1 kHz), so a sub-sample
  // error is a lot of cents. Instead of measuring one period, measure the
  // longest whole number of periods that fits in kMaxLag and divide: the
  // error shrinks by the same factor.
  static float refinePeriod(const int16_t* x, float period) {
    const int kSearch = 3;
    int k = (int)((float)(kMaxLag - kSearch - 1) / period);
    if (k < 2) return period;
    int center = (int)lroundf(period * (float)k);
    uint64_t d[2 * kSearch + 1];
    int best = 0;
    for (int i = 0; i <= 2 * kSearch; ++i) {
      d[i] = difference(x, center - kSearch + i);
      if (d[i] < d[best]) best = i;
    }
    if (best == 0 || best == 2 * kSearch) return period;  // no clear dip, keep the coarse value
    float lag = (float)(center - kSearch + best) +
                parabolaShift((float)d[best - 1], (float)d[best], (float)d[best + 1]);
    return lag / (float)k;
  }

  float cmndf_[kMaxLag + 1];  // cumulative mean normalized difference per lag
  float clarity_ = 0.0f;
};

// Note numbers are MIDI numbers: 69 = A4, 40 = low E string (E2).
inline void freqToNote(float freq, float a4, int* midiNote, float* cents) {
  float m = 69.0f + 12.0f * log2f(freq / a4);
  int n = (int)lroundf(m);
  *midiNote = n;
  *cents = (m - (float)n) * 100.0f;
}

inline int notePitchClass(int midiNote) { return ((midiNote % 12) + 12) % 12; }  // 0 = C
inline int noteOctave(int midiNote) { return midiNote / 12 - 1; }

static const char* const kNoteNames[12] = {"C", "C#", "D", "D#", "E", "F",
                                           "F#", "G", "G#", "A", "A#", "B"};
