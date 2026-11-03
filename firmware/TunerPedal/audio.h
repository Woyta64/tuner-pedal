// ---------------------------------------------------------------------------
// Audio capture. The ADC samples AUDIO_PIN non-stop and DMA copies every
// reading into one of two buffers. While DMA fills one buffer, the main loop
// analyses the other, so no audio is lost while the pitch maths runs.
// ---------------------------------------------------------------------------
#pragma once
#include <Arduino.h>

#include "hardware/adc.h"
#include "hardware/dma.h"

#include "config.h"
#include "pitch.h"

// Shortest and longest period the detector looks for, in samples.
static constexpr int kMinLag = SAMPLE_RATE / MAX_FREQ;
static constexpr int kMaxLag = SAMPLE_RATE / MIN_FREQ + 2;
using PitchDetector = Yin<YIN_WINDOW, kMaxLag>;

// One frame is what the detector needs. The ADC takes OVERSAMPLE readings for
// every sample in the frame.
static constexpr int kFrameLen = PitchDetector::kInputLen;
static constexpr int kRawLen = kFrameLen * OVERSAMPLE;

static uint16_t rawBuffers[2][kRawLen];
static int fillingBuffer = 0;  // the buffer DMA is writing to right now
static int dmaChannel;
static dma_channel_config dmaConfig;

static void startFilling(int buffer) {
  fillingBuffer = buffer;
  adc_fifo_drain();  // start the frame with fresh readings, not leftovers
  dma_channel_configure(dmaChannel, &dmaConfig, rawBuffers[buffer], &adc_hw->fifo, kRawLen, true);
}

static void startAudioCapture() {
  adc_init();
  adc_gpio_init(AUDIO_PIN);
  adc_select_input(AUDIO_PIN - 26);  // GP26 = ADC0 ... GP29 = ADC3
  adc_fifo_setup(true,    // write conversions to the FIFO
                 true,    // raise DMA requests
                 1,       // ...as soon as 1 reading is waiting
                 false,   // no error bit in the reading
                 false);  // keep all 12 bits
  // The ADC clock is 48 MHz, and it converts once every (clkdiv + 1) cycles.
  adc_set_clkdiv(48000000.0f / (SAMPLE_RATE * OVERSAMPLE) - 1.0f);

  dmaChannel = dma_claim_unused_channel(true);
  dmaConfig = dma_channel_get_default_config(dmaChannel);
  channel_config_set_transfer_data_size(&dmaConfig, DMA_SIZE_16);
  channel_config_set_read_increment(&dmaConfig, false);  // always read the same FIFO register
  channel_config_set_write_increment(&dmaConfig, true);  // fill the buffer in order
  channel_config_set_dreq(&dmaConfig, DREQ_ADC);         // one transfer per conversion

  startFilling(0);
  adc_run(true);
}

// Waits until the current buffer is full, starts filling the other one and
// returns the full one. Its contents stay valid until the next call.
static const uint16_t* waitForAudio() {
  dma_channel_wait_for_finish_blocking(dmaChannel);
  int full = fillingBuffer;
  startFilling(full ^ 1);
  return rawBuffers[full];
}
