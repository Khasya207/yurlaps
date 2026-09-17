# YurLaps independent one-loop PSoC 5LP lap decoder

This directory contains a deliberately minimal decoder for one receiver loop on
a **CY8CKIT-059 / CY8C5888LTI-LP097**. It recognizes the published 5 MHz,
96-bit differential-phase telegram used by legacy RCHourglass transponders and
emits the published RCHourglass passage-record format from one TX pin.

It has no modes, learn function, command RX, USB CDC, registration interface,
voltage/temperature reader, or decoder configuration manager.

It does **not** contain the unavailable RCHourglass PSoC source, a decompilation,
or any RCHourglass binary. The implementation is BSD-3-Clause licensed. A
commercial product still needs an independent legal review of relevant protocol,
patent, trademark, and third-party hardware questions.

## Exact stage-one behavior

- one input channel on **P12[2]**;
- low-delay hardware feedback from P12[2] to **P12[3]** through the existing
  external R11 (4.87 kOhm) hysteresis connection;
- 20 MS/s DMA capture of the P12[2] Port Status bit;
- differential-delay BPSK demodulation at 1.25 Mbit/s;
- `F9 16` preamble search and strict 12-byte/96-bit packet validation;
- legacy transponder ID and status extraction;
- interleaved-passage tracking for up to 16 IDs;
- one ECO/PLL-derived quarter-millisecond timebase;
- one **TX-only 57600 8-N-1 UART on P12[7]**;
- one RCHourglass-compatible passage record after each completed passage;
- P2[1] LED flash on valid packets and steady-on indication for initialization
  or DMA re-arm failure.

P12[6] is unused. No startup banner or diagnostic text can enter the ESP32 lap
stream.

## Output format

Every completed passage produces exactly:

```text
nnnnnntttttttt-idhhqqvvtm\r\n
```

All fields are uppercase ASCII hexadecimal:

| Field | Characters | Meaning in this firmware |
|---|---:|---|
| `nnnnnn` | 6 | transponder ID |
| `tttttttt` | 8 | first valid packet time, quarter milliseconds from startup |
| `id` | 2 | fixed decoder ID `01` |
| `hh` | 2 | validated packet hits, saturated at 255 |
| `qq` | 2 | average digital demodulation confidence, 0-100 |
| `vv` | 2 | `00`, voltage unavailable |
| `tm` | 2 | `00`, temperature unavailable |

Example from the published format, using 21 hits and quality value `0x2A`:

```text
23E35500075A06-01152A0000
```

The record is 25 characters plus CRLF. ESP32 may ignore timestamp, decoder ID,
voltage, and temperature, but retaining all fields keeps framing compatible with
the published RCHourglass record. Note that this implementation's `quality` is
an independently defined digital sample-vote confidence. The unavailable
upstream decoder source means numerical equivalence to its proprietary quality
algorithm is not claimed. It is not analog RSSI or signal strength.

Passage behavior:

- `hits` counts strictly valid telegrams collected for the ID;
- first-packet timestamp is retained;
- a passage closes after 8 ms without another valid packet;
- the same ID is suppressed for 300 ms from the first packet;
- values above decimal ID 9,999,999 are rejected rather than truncated.

## Receiver architecture

```text
loop -> loop amplifier -> original decoder input conditioner -> P12[2]
                                                               |       |
                                                               |       +--> 20 MHz DMA
                                                               +----------> P12[3] -> R11

samples -> y[n] XOR y[n-16] -> F9 16 -> 96 bits -> strict re-encoding
        -> ID -> passage hits/quality -> RCHourglass record -> P12[7] -> ESP32
```

No Comparator component is used with the original decoder PCB because Q1-Q4,
C1/C2, and R9/R10 already condition the signal before P12[2]. A passive or new
analog receiver requires a sufficiently fast external comparator/limiter before
P12[2]; raw coax must never connect directly to the pin.

## PSoC Creator components

The source expects these exact instance names:

| Component | Name | Configuration |
|---|---|---|
| Digital Input Pin | `LoopIn` | P12[2], high-Z digital, rising interrupt |
| Digital Output Pin | `HystOut` | P12[3], hardware driven by LoopIn |
| Interrupt | `LoopEdgeISR` | connected to LoopIn interrupt |
| Clock | `SampleClock` | 20 MHz |
| DMA | `SampleDMA` | rising-edge hardware request |
| UART | `LapUART` | TX only, 57600 8-N-1 |
| Digital Output Pin | `LapTx` | P12[7], connected to LapUART TX |
| Digital Output Pin | `StatusLED` | P2[1], software controlled, initial 1 |

The clock tree is 5 MHz ECO -> PLL x16 -> 80 MHz Master/BUS, with the sample
clock at BUS/4.

Follow the detailed Indonesian construction guide:

- [PANDUAN-PSOC-CREATOR-ID.md](PANDUAN-PSOC-CREATOR-ID.md)

## Verification level

| Item | State |
|---|---|
| Three published packet vectors | Passing |
| 1,000 deterministic randomized codec round trips | Passing |
| Every single-bit coded-payload corruption | Rejected |
| Synthetic carrier phase, polarity, ring wrap and sample errors | Passing |
| Multi-ID passage hits/holdoff | Passing |
| Exact 25-character TX record and no startup text | Passing |
| PSoC DMA wrapper state/API tests | Passing |
| DWT timebase wrap tests | Passing |
| PSoC-specific syntax against generated-API stubs | Passing |
| PSoC Creator Windows target build | **Not yet run** |
| Real CY8CKIT-059 and loop capture | **Not yet validated** |

Run host tests with GCC/Clang:

```sh
cd firmware/psoc5lp-one-loop/host-tests
make clean test
```

Expected final lines:

```text
All protocol and 20 MS/s demodulator tests passed.
All RCHourglass-compatible TX record tests passed.
All PSoC DMA-wrapper state tests passed.
All PSoC DWT timebase wrap tests passed.
```

## Directory layout

- `OneLoopDecoder.cydsn/` — source to add to the Creator project.
- `host-tests/` — deterministic host and generated-API syntax tests.
- `PANDUAN-PSOC-CREATOR-ID.md` — detailed Windows construction guide.
- `UPSTREAM-ANALYSIS.md` — clean implementation boundary and artifact findings.
- `LICENSE` — BSD-3-Clause license for this implementation.
