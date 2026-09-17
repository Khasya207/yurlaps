# YurLaps independent one-loop PSoC 5LP decoder

This directory contains the first clean implementation for one receiver loop on
a **CY8CKIT-059 / CY8C5888LTI-LP097**. It targets the published 5 MHz,
96-bit differential-phase telegram used by legacy RCHourglass transponders.

It does **not** contain the unavailable RCHourglass PSoC source, a decompilation,
or any RCHourglass binary. The implementation is BSD-3-Clause licensed so it
can be used in a commercial product, subject to an independent legal review of
protocol/patent and third-party hardware questions.

## What is implemented

- one input channel on **P12[2]**;
- low-delay hardware feedback from the input to **P12[3]** through the existing
  external R11 (4.87 kOhm) hysteresis connection;
- a 20 MS/s DMA capture of P12[2];
- differential-delay BPSK demodulation at 1.25 Mbit/s;
- `F9 16` preamble search and 12-byte/96-bit capture;
- strict payload-code validation using polynomial `0x776107`;
- legacy 30-bit transponder-number and ten status-bit extraction;
- support for interleaved packets from up to 16 cars during a passage;
- a single ECO/PLL-derived quarter-millisecond timestamp;
- 57600-baud, 8-N-1 UART on P12[6]/P12[7];
- Cano-compatible 14-character passage records;
- raw packet, quality, hit and capture diagnostics;
- deterministic Linux/Windows host tests.

## Receiver architecture

```text
loop -> loop amplifier -> decoder analog phase/input stage -> P12[2]
                                                           |       |
                                                           |       +--> 20 MHz DMA samples
                                                           +----------> P12[3] -> R11 feedback

samples -> y[n] XOR y[n-16] -> F9 16 search -> 96 bits -> strict validation
        -> ID/status -> passage aggregation -> 57600 UART -> YurLaps/terminal
```

At 20 MS/s, one 5 MHz carrier cycle is four samples and one encoded bit is 16
samples. Differential encoding means that, away from a transition,
`sample[n] XOR sample[n-16]` is the transmitted bit. Trying every raw sample
offset removes packet, carrier and bit-phase assumptions. Five adjacent samples
vote on each bit, and a packet is accepted only if rebuilding all 80 coded
payload bits exactly matches the received telegram.

## Current verification level

| Item | State |
|---|---|
| Published packet vectors | Passing |
| ID/status encode/decode | Passing |
| Damaged packet rejection | Passing |
| Synthetic 20 MS/s carrier, phase reversal and polarity | Passing |
| Circular DMA wrap | Passing |
| Deterministic sample errors | Passing |
| PSoC DMA wrapper state/API tests | Passing |
| DWT timebase wrap/quarter-millisecond tests | Passing |
| PSoC-specific source syntax with API stubs | Passing |
| PSoC Creator target build | **Needs Windows/PSoC Creator** |
| Real CY8CKIT-059 + loop hardware | **Not yet validated** |

PSoC Creator stores `TopDesign.cysch` in a proprietary binary format. It cannot
be generated or target-built in this Linux agent environment. The target source
is complete and expects fixed component names, but the Creator schematic must be
created once by following [PSoC-Creator-Setup.md](PSoC-Creator-Setup.md). Do not
interpret the host tests as proof that hardware testing is complete.

## Directory layout

- `OneLoopDecoder.cydsn/` — C source to add to the Creator project.
- `host-tests/` — protocol, demodulator, passage and PSoC API syntax tests.
- `PSoC-Creator-Setup.md` — novice-oriented Windows setup, schematic, clocks,
  pins, build, programming, serial test and recovery instructions.
- `UPSTREAM-ANALYSIS.md` — what was and was not learned from the published
  artifacts.
- `LICENSE` — BSD-3-Clause license for this independent implementation.

## Run deterministic tests

Linux/macOS/MSYS2/WSL with `make` and a C compiler:

```sh
cd firmware/psoc5lp-one-loop/host-tests
make clean test
```

Expected final lines:

```text
All protocol and 20 MS/s demodulator tests passed.
All UART console and Cano-format tests passed.
All PSoC DMA-wrapper state tests passed.
All PSoC DWT timebase wrap tests passed.
```

## UART output

The production default is Cano mode:

```text
4BB5B900001234
```

The first six hexadecimal characters are the transponder number. The following
eight are time since decoder startup in quarter milliseconds. Every line ends
in CRLF.

In a 57600-baud terminal, type:

```text
HELP
SELFTEST
STATUS
MODE DIAG
```

Diagnostic mode also emits lines such as:

```text
PKT raw=F916EFDA353B28290B0BCF3C id=4961721 status=0 t_qms=1234 q=100 sample=811 valid=1
PASS id=4961721 t_qms=1234 hits=3 q=98 status=510 raw=F916...
```

Return to software-safe output with `MODE CANO`.

## Scope boundaries for this first stage

This version intentionally does not yet implement RC4 learning, AMB/TranX
emulation, target-native USB CDC, voltage/temperature interpretation, four
channels, RSSI, or the new YurLaps transponder protocol. Those should be added
after one-loop captures are measured on real hardware and retained as regression
fixtures.
