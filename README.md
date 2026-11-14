# Tuner pedal

A guitar tuner in a stompbox. Step on it and the guitar goes to the tuner
while the output is muted. Step on it again and the signal goes straight
through (true bypass). The note shows on a 7-segment display, and seven LEDs
show how far off it is.

```
 L3   L2   L1    G    R1   R2   R3
 ●    ●    ●     ●    ●    ●    ●
 └─ flat ──┘          └─ sharp ─┘
              ┌─────┐
              │  8. │
              └─────┘
```

- **Waveshare RP2040-Zero**, plugged into headers on the PCB
- **YIN pitch detection** on 20 kHz audio captured with the ADC and DMA,
  accurate to a few cents from 60 to 1200 Hz (drop tunings and 7-strings included)
- **MCP6002 input stage** with 1 MΩ input impedance, so it doesn't load the guitar
- **74HC595-driven display** and a 7-LED meter: green within ±3 cents, then
  three red steps on each side
- **9 V pedal power** with reverse-polarity protection. USB can stay plugged in
  for reflashing.
- **3PDT footswitch:** true bypass when off, output muted while tuning

## Repository

| Folder | What's inside |
|---|---|
| [`hardware/`](hardware/) | EasyEDA Pro project (schematic + PCB), circuit description, pin map, footswitch wiring and parts list |
| [`firmware/`](firmware/) | Arduino firmware for the RP2040-Zero, a PC test for the pitch detector, and build and flash instructions |

## Getting started

1. Order the board from the EasyEDA project in `hardware/`. All SMD parts
   are on the bottom side, so JLCPCB's Economic assembly places them. See
   [hardware/README.md](hardware/README.md).
2. Solder the RP2040-Zero headers, the display and the LEDs, then wire up
   the jacks, the DC socket and the 3PDT.
3. Flash the firmware with the Arduino IDE or arduino-cli. See
   [firmware/README.md](firmware/README.md).
4. The boot self-test lights every segment and LED in turn. After that, plug
   in a guitar and tune.
