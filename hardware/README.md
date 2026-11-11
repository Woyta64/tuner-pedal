# Tuner pedal hardware

The PCB for a guitar tuner pedal: a 7-segment note display, a
7-LED tuning meter, an op-amp input stage and a 9 V pedal power supply. A
Waveshare RP2040-Zero plugs in underneath. It was designed in EasyEDA Pro
and assembled by JLCPCB.

```
 L3   L2   L1    G    R1   R2   R3
 ●    ●    ●     ●    ●    ●    ●
 └─ flat ──┘          └─ sharp ─┘
              ┌─────┐
              │  8. │
              └─────┘
```

## Files

| File | What it is |
|---|---|
| `TunerPedal.epro2` | EasyEDA Pro project: schematic and PCB layout |

Open it in EasyEDA Pro (desktop or web) with *File → Import → EasyEDA Pro*.

## Board

- 56 × 48 mm, 2 layers, 1.6 mm, GND pour on both sides.
- **Top side:** only the display and the seven 3 mm LEDs, which show through
  the enclosure lid.
- **Bottom side:** every SMD part (0603/0805 passives, SOIC chips, SOT-223
  regulator), so JLCPCB's single-sided Economic assembly can place them all.
  The RP2040-Zero plugs into two 1×9 female headers on this side, with its
  USB-C port facing the front edge for reflashing.
- Four 3.2 mm mounting holes at (3.5, 3.5), (52.5, 3.5), (3.5, 28) and
  (52.5, 28) mm, each with a 7.5 mm keep-out for the standoffs.
- Signal tracks 0.25 mm, power tracks 0.5 mm, 0.3 mm vias. All within
  JLCPCB's standard process.

## Circuit

### Power

```
J1.1 (+9 V) ──D1──▶|── AMS1117-5.0 ──┬── D2 ──▶|── RP2040-Zero 5V
                     (U1)            │
                                    +5V_REG
```

- **D1** (SS14) protects against a reversed adapter: nothing happens instead
  of damage.
- **U1** (AMS1117-5.0) drops the pedal supply to 5 V. 10 µF + 100 nF on the
  input, 2 × 10 µF + 100 nF on the output, because the AMS1117 needs plenty of
  output capacitance to stay stable.
- **D2** (SS14) lets USB stay plugged in while the 9 V supply is on.
- The RP2040-Zero's own regulator makes the 3.3 V rail for everything else.

Any standard 9 V centre-negative pedal supply works. The AMS1117 accepts up
to 15 V.

### Input amplifier

The guitar signal is a few hundred millivolts centred on 0 V. The ADC reads
0–3.3 V, so an MCP6002 (U3, rail-to-rail on 3.3 V) buffers and amplifies it
around mid-rail:

- **R16** 1 MΩ to GND keeps the input at 0 V while it's switched out, so the
  footswitch doesn't pop.
- **C5** 100 nF + **R17** 1 MΩ AC-couple the guitar onto **VREF** (1.65 V,
  from R18/R19 100 kΩ, filtered by C6 10 µF). The input impedance is about
  1 MΩ, like a normal pedal, so it doesn't dull the guitar's tone.
- Non-inverting gain of 1 + 47k/10k ≈ **5.7** (R20, R21). C7 10 µF puts
  DC gain at 1, so the output stays centred.
- **R22** 1 kΩ + **C8** 10 nF low-pass the output (≈16 kHz) before GP26.
- The unused half of the MCP6002 is tied off as a follower on VREF.

The op-amp output can't leave 0–3.3 V, so the ADC pin needs no clamp diodes.

### Display

- **LED1**: BS-A554RD 0.56" common-anode 7-segment display. Both common
  pins go to 3.3 V.
- **U2**: 74HC595 shift register. Its outputs QA–QH drive segments a–g and
  dp through **R8–R15** (470 Ω). The latch (RCLK) stops the segments
  flickering while new bits shift in. OE is tied low and SRCLR high.

### Meter LEDs

Seven 3 mm LEDs on an arc above the display, each driven straight from a
GPIO through its own 470 Ω resistor (**R1–R7**, next to the RP2040-Zero's GP0
header). The LED order is mirrored compared to the breadboard version
(L3 = GP6 … R3 = GP0), so the seven traces fan out without crossing.

### RP2040-Zero pin map

| Header pin | GPIO | Net |
|---|---|---|
| J3.1 | 5V | +5V (from D2) |
| J3.2 | GND | GND |
| J3.3 | 3V3 | +3V3 |
| J3.4–6 | GP29–27 | not connected |
| J3.7 | GP26 | AUDIO (ADC0) |
| J3.8 | GP15 | ENGAGE |
| J3.9 | GP14 | SEG_LATCH |
| J4.1–3 | GP0–2 | LED_R3, LED_R2, LED_R1 |
| J4.4 | GP3 | LED_G |
| J4.5–7 | GP4–6 | LED_L1, LED_L2, LED_L3 |
| J4.8 | GP7 | SEG_DATA |
| J4.9 | GP8 | SEG_CLK |

GP9–GP13 are on the module's bottom edge and aren't used.

## Off-board wiring

The jacks, DC socket and footswitch mount in the enclosure and are wired to
two pin headers:

| Pad | Net | Goes to |
|---|---|---|
| J1.1 | +9 V in | DC jack + (the outer sleeve on a centre-negative jack) |
| J1.2 | GND | DC jack − (centre pin) |
| J2.1 | TUNER_IN | 3PDT lug 1 |
| J2.2 | ENGAGE | 3PDT lug 7 |
| J2.3 | GND | 3PDT lug 8 |
| J2.4 | GND | input jack sleeve |

### 3PDT footswitch

When the pedal is off it is true bypass: the guitar goes straight from the
input to the output jack. When it's on, the guitar goes to the tuner, the
output is shorted to GND (muted), and GP15 is pulled low so the display
turns on.

Lugs are numbered by pole: pole 1 = 1/2/3, pole 2 = 4/5/6, pole 3 = 7/8/9.
The middle lug (2, 5, 8) is the common. "Tune" is the lug the common
touches when the switch is on.

| Lug | Wire to |
|---|---|
| 2 (pole 1 common) | input jack tip |
| 1 (pole 1 tune) | J2.1 TUNER_IN |
| 3 (pole 1 bypass) | lug 6 |
| 5 (pole 2 common) | output jack tip |
| 4 (pole 2 tune) | lug 8 (GND, mutes the output) |
| 6 (pole 2 bypass) | lug 3 |
| 8 (pole 3 common) | J2.3 GND |
| 7 (pole 3 tune) | J2.2 ENGAGE |
| 9 (pole 3 bypass) | not connected |

Output jack sleeve goes to the input jack sleeve. Check which outer row is
"tune" on your switch with a continuity tester. If it's the other way round,
swap the outer lugs on all three poles.

## Parts

Placed by JLCPCB (all Basic parts except the MCP6002, which is a Promotional
Extended part with no extra fee):

| Qty | Part | LCSC | Refs |
|---|---|---|---|
| 15 | 470 Ω 0603 | C23179 | R1–R15 |
| 2 | 1 MΩ 0603 | C22935 | R16, R17 |
| 2 | 100 kΩ 0603 | C25803 | R18, R19 |
| 1 | 47 kΩ 0603 | C25819 | R20 |
| 1 | 10 kΩ 0603 | C25804 | R21 |
| 1 | 1 kΩ 0603 | C21190 | R22 |
| 5 | 100 nF 0603 | C14663 | C2, C4, C5, C9, C10 |
| 5 | 10 µF 0805 | C15850 | C1, C3, C3B, C6, C7 |
| 1 | 10 nF 0603 | C57112 | C8 |
| 2 | SS14 | C2480 | D1, D2 |
| 1 | AMS1117-5.0 | C6187 | U1 |
| 1 | 74HC595D | C5947 | U2 |
| 1 | MCP6002T-I/SN | C7377 | U3 |

Hand-soldered:

- Waveshare RP2040-Zero and 2 × 1×9 female headers (2.54 mm, about 8.5 mm tall)
- BS-A554RD 7-segment display
- 6 red + 1 green 3 mm LEDs
- Wires to J1 and J2: DC jack, two mono 6.35 mm jacks, 3PDT footswitch

### Assembly order

Solder the female headers first and trim their pins flush on the top side,
because the display sits right next to them. Then fit the display and the
LEDs. Without the footswitch wired, short J2.2 to J2.3 to keep the tuner on
for the first test.
