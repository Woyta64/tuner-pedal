# Tuner pedal firmware

Firmware for a guitar tuner pedal built around a Waveshare
RP2040-Zero. It listens to the guitar on one ADC pin, finds the pitch with the
YIN algorithm and shows the note on a 7-segment display with a 7-LED meter.

```
 L3   L2   L1    G    R1   R2   R3
 ●    ●    ●     ●    ●    ●    ●
 └─ flat ──┘          └─ sharp ─┘
              ┌─────┐
              │  8. │
              └─────┘
```

| Reading           | What lights               |
|-------------------|---------------------------|
| within ±3 cents   | green                     |
| 3–12 cents off    | L1 / R1 (next to display) |
| 12–25 cents off   | L2 / R2                   |
| more than 25 off  | L3 / R3 (outermost)       |
| sharp note (C#…)  | letter + decimal point    |

Notes show as `C d E F G A b`. The thresholds are in `config.h`.

## How it works

1. **Capture** (`audio.h`): the ADC samples the guitar at 80 kHz. DMA copies the
   samples into one of two buffers while the CPU analyses the other one, so no
   audio is lost. Groups of 4 readings are summed into one sample, so the
   detector sees 20 kHz with a little less noise.
2. **Pitch** (`pitch.h`): YIN compares the signal with delayed copies of itself
   and picks the shortest delay where it repeats. Parabolic interpolation and a
   multi-period refinement keep a single frame within 3 cents (about 1 cent
   on average in the PC test). The inner loop is integer-only, because the
   RP2040's Cortex-M0+ has no FPU. Detection covers 60–1200 Hz.
3. **Tracking** (`tracker.h`): the cents value is smoothed, a new note must
   show up in 2 frames in a row before the display switches, and the last note
   stays on for 1.5 s after the string stops.
4. **Display** (`display.h`): the note letter goes out through a 74HC595 shift
   register (3 pins) and the meter LEDs are driven straight from GPIO.

One frame is 68 ms of audio. The pitch maths for a frame has to finish before
the next one is captured, and the Serial Monitor prints how long it took.

`pitch.h` and `tracker.h` don't depend on Arduino, so the detector also runs
on a PC (see [Testing on a PC](#testing-on-a-pc)).

## Files

| File | What it does |
|---|---|
| `TunerPedal/TunerPedal.ino` | `setup()` and the main loop |
| `TunerPedal/config.h` | Pins, display type, thresholds. **The only file you should need to edit.** |
| `TunerPedal/audio.h` | ADC + DMA double-buffered capture |
| `TunerPedal/pitch.h` | YIN pitch detector and note maths |
| `TunerPedal/tracker.h` | Smoothing and note hold |
| `TunerPedal/display.h` | 7-segment display and meter LEDs |
| `TunerPedal/diagnostics.h` | Boot self-test and Serial Monitor test commands |
| `test/pitch_test.cpp` | Runs the pitch detector on synthetic guitar tones on a PC |

## Building and flashing

You need the **Raspberry Pi Pico/RP2040** board package by Earle Philhower.

### Arduino IDE

1. *File → Preferences → Additional boards manager URLs*, add
   `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
2. *Tools → Board → Boards Manager*, install **Raspberry Pi Pico/RP2040**.
3. Open `TunerPedal/TunerPedal.ino` and pick the board **Waveshare RP2040 Zero**.
4. Click *Upload*.

### arduino-cli

```bash
arduino-cli config add board_manager.additional_urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
arduino-cli core update-index
arduino-cli core install rp2040:rp2040

arduino-cli compile --fqbn rp2040:rp2040:waveshare_rp2040_zero TunerPedal
arduino-cli upload  --fqbn rp2040:rp2040:waveshare_rp2040_zero -p /dev/ttyACM0 TunerPedal
```

### If the upload fails

- **Board not found:** hold **BOOT**, tap **RESET** (or plug the USB cable in
  while holding BOOT), then upload again. The board shows up as a USB drive
  called `RPI-RP2`. You can also drag the `.uf2` from the build folder onto it.
- **Permission denied on `/dev/ttyACM0` (Linux):** add yourself to the serial
  group (`uucp` on Arch, `dialout` on Debian/Ubuntu), then log out and back in.

On the PCB the RP2040-Zero's USB-C port faces the front edge, so you can
reflash it with the enclosure lid off.

## Configuration

Everything is in `config.h`. It is set up for the PCB in `../hardware`. For
the breadboard version, change these lines:

| Setting | PCB | Breadboard |
|---|---|---|
| `FLAT_PINS` | `{4, 5, 6}` | `{2, 1, 0}` |
| `SHARP_PINS` | `{2, 1, 0}` | `{4, 5, 6}` |
| `SEG_LATCH_PIN` | `14` (74HC595) | `-1` (74HC164) |
| `ENGAGE_PIN` | `15` (3PDT) | `-1` (always on) |

Full pin map on the PCB:

| GPIO | Function |
|---|---|
| GP0 / GP1 / GP2 | sharp LEDs R3 / R2 / R1 |
| GP3 | green LED |
| GP4 / GP5 / GP6 | flat LEDs L1 / L2 / L3 |
| GP7 | 74HC595 data (SER) |
| GP8 | 74HC595 clock (SRCLK) |
| GP14 | 74HC595 latch (RCLK) |
| GP15 | footswitch sense, low = tuning |
| GP26 (ADC0) | audio from the input amplifier |

## First power-up

1. **Self-test.** At boot every segment lights one at a time, then the letters
   C d E F G A b, then the LEDs L3 → R3, then everything for half a second.
   A missing or wrong segment means a wiring or soldering problem. If
   everything *except* the expected segment lights, flip `SEG_COMMON_ANODE`.
2. **Engage.** The display stays dark in bypass. If the 3PDT isn't wired yet,
   short J2.2 (ENGAGE) to J2.3 (GND) to keep the tuner on.
3. **Serial Monitor at 115200 baud.** With nothing playing it prints
   `quiet, level N` once a second. N is the noise floor and must stay below
   `MIN_SIGNAL_P2P`. Raise that value if notes flicker with the guitar silent.
4. **Play a string.** Each frame prints a line like this:

   ```
   E2	82.38 Hz	-0.6 cents	(shown -0.8, level 412, clarity 0.97, 38 ms)
   ```

   `shown` is the smoothed value on the meter, `level` is the signal strength
   and `ms` is how long the pitch maths took. It has to stay under 68 ms.

### Serial test commands

Type a letter in the Serial Monitor and press Enter:

| Key | Test |
|---|---|
| `t` | Run the self-test again |
| `a` | Everything on for 5 seconds |
| `m` | Meter sweep: fake readings from −35 to +35 cents, one LED each |
| `n` | Note sweep: C, C#, D … B (sharps light the dot) |
| `d` | Display check: all off, all on, then only a, only g, only the dot |

## Testing on a PC

`test/pitch_test.cpp` feeds synthetic guitar-like tones through the same
`pitch.h` and `config.h` the pedal uses. The tones are bright, weak-fundamental,
quiet and noisy, with real-string inharmonicity. It reports the worst error
in cents for each note:

```bash
cd test
g++ -O2 -std=c++17 -I../TunerPedal pitch_test.cpp -o pitch_test && ./pitch_test
```

It exits non-zero if any note reads the wrong letter, gives no reading, or is
off by more than 3 cents.

## Troubleshooting

| Symptom | Fix |
|---|---|
| Random notes with nothing plugged in | Raise `MIN_SIGNAL_P2P` above the `quiet, level` number |
| Note disappears too fast as the string rings out | Lower `MIN_SIGNAL_P2P`, or raise `YIN_THRESHOLD` towards 0.20 |
| Wrong octave in the Serial Monitor | Harmless, the display only shows the letter. If the meter jumps, lower `YIN_THRESHOLD` to 0.10 |
| Meter too jumpy / too slow | `CENTS_SMOOTHING` (higher = faster) |
| Green zone too wide / narrow | `IN_TUNE_CENTS` |
| Display always dark | Pedal is in bypass: check the 3PDT wiring, or short J2.2 to J2.3 |
