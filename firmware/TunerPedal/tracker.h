// ---------------------------------------------------------------------------
// Note tracking: turns the raw reading of each frame into what the display
// shows.
//   - The cents value is smoothed, so the meter doesn't jitter.
//   - A new note has to be heard a few frames in a row before the display
//     switches to it, so one bad frame can't flash a wrong letter.
//   - The last note stays on for a moment after the string stops ringing.
// ---------------------------------------------------------------------------
#pragma once
#include <stdint.h>

#include "config.h"
#include "pitch.h"

class NoteTracker {
 public:
  // Feed one frame's result: the frequency in Hz, or 0 if there was no clear pitch.
  void update(float freq, uint32_t nowMs) {
    if (freq <= 0) {
      if (hasNote() && nowMs - lastHeardMs_ > HOLD_MS) note_ = kNoNote;
      return;
    }
    lastHeardMs_ = nowMs;

    int note;
    float cents;
    freqToNote(freq, A4_REF_HZ, &note, &cents);

    if (note == note_) {
      cents_ += CENTS_SMOOTHING * (cents - cents_);
      candidate_ = kNoNote;
      return;
    }

    // A different note: only switch once it has been heard a few frames in a row.
    if (note == candidate_) {
      ++candidateFrames_;
    } else {
      candidate_ = note;
      candidateFrames_ = 1;
    }
    if (candidateFrames_ >= NOTE_CONFIRM_FRAMES) {
      note_ = note;
      cents_ = cents;
      candidate_ = kNoNote;
    }
  }

  void reset() { *this = NoteTracker(); }

  bool hasNote() const { return note_ != kNoNote; }
  int note() const { return note_; }  // MIDI note number
  float cents() const { return cents_; }

 private:
  static const int kNoNote = -1;

  int note_ = kNoNote;       // the note on the display
  float cents_ = 0;          // smoothed cents for that note
  int candidate_ = kNoNote;  // a different note we've started hearing
  int candidateFrames_ = 0;  // ...for this many frames in a row
  uint32_t lastHeardMs_ = 0;
};
