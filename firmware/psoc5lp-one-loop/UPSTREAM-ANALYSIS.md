# Published RCHourglass artifact analysis

## Clean-implementation boundary

The upstream repository publishes compiled PSoC decoder HEX images, schematics,
a Windows manager, and PIC transponder source. It does **not** publish the PSoC
Creator decoder project or decoder C/Verilog source. Therefore:

- no editable decoder source was recovered;
- no upstream HEX is included in this firmware directory;
- strings/static inspection are evidence of externally visible behavior, not
  source code;
- the YurLaps implementation uses a new sampled-signal architecture and new C
  source;
- commercial release still deserves legal review. A clean rewrite addresses
  source-code copyright dependence but is not, by itself, a patent/trademark or
  hardware-design legal opinion.

Upstream repository: <https://github.com/mv4wd/RCHourglass>

## Decoder HEX inventory

Eight decoder images are published:

| File | Embedded version string |
|---|---|
| `Version01_BETA.hex` | `RC Hourglass v0.1 beta` |
| `Version03_BETA.hex` | `RC Hourglass v0.3 beta` |
| `Version05_beta.hex` | `RC Hourglass v0.5 beta` |
| `Version06_beta.hex` | `RC Hourglass v0.6 beta` |
| `Version07_BETA.hex` | `RC Hourglass v0.7 beta` |
| `Version08_beta.hex` | `RC Hourglass v0.8 beta` |
| `Version09_beta.hex` | `RC Hourglass v0.9 beta` |
| `Version010_beta.hex` | `RC Hourglass v0.10 beta` |

Each Intel HEX contains 295,190 data bytes across several PSoC address spaces:

- flash `0x00000000-0x0003FFFF`;
- EEPROM/configuration `0x80000000-0x80007FFF`;
- metadata/configuration regions beginning at `0x90000000`.

This is normal for a programmable-system-on-chip image. It does not turn the HEX
into a Creator project.

### Useful v0.10 strings

Static string extraction from the latest flash found:

- modes: `MONITOR`, `CANO`, `CANO CLASSIC`, `RCHOURGLASS`, `AMBRC`, `TRANX`,
  and `LOOPBACK`;
- commands: `GET TIME`, `SET TIME`, `VERSION`, `SET SERIAL`, `SET USB`,
  `GET ID`, `SET ID`, `GET VMIN`, `SET VMIN`, `SET LED`, `SET BEEP`, plus PIC
  programming and transponder-registration commands;
- Cano format string: `%06lX%08lX`;
- extended format string: `%06lX%08lX-%02X%02X%02X%02X%02X`;
- raw monitor format:
  `%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X`;
- version: `RC Hourglass v0.10 beta`.

The strings agree with the published protocol wiki. They do not reveal the
closed PSoC demodulator implementation. The new firmware does not copy the
command processor or emulate all of these modes.

## Exact decoder pin connections

From the published decoder schematic and build guide:

### P12[2] — detector input

The 75-ohm coax center is not wired directly to P12[2]. The route is:

```text
loop-amplifier coax center
  -> +5 V/75-ohm feed point
  -> C1 (470 pF)
  -> Q1/Q2/Q3/Q4 phase-detector/input-amplifier
  -> C2 (470 pF) and R9/R10 bias node
  -> P12[2]
```

R9 and R10 are 2.4 kOhm bias resistors. For the first digital implementation,
P12[2] is a high-impedance digital input with its input buffer enabled.

### P12[3] — hysteresis feedback

P12[3] connects through R11 = 4.87 kOhm to the same node that feeds P12[2]. In
this independent design, PSoC routing makes P12[3] follow P12[2] directly in
digital hardware. That positive feedback provides a Schmitt-like hysteresis
effect. P12[3] is not another loop input and must not be tied directly to power
or ground.

This direct hardware mirror is the new implementation choice; it is not a claim
that the unavailable upstream source used the identical internal circuit.

### P15[0]/P15[1] — 5 MHz ECO

A 5 MHz crystal is fitted across P15[0] and P15[1], with one 22 pF capacitor from
each end to ground. It is the precision carrier/sample and passage-time
reference. The new clock tree uses 5 MHz ECO -> PLL x16 -> 80 MHz master/bus,
then BUS_CLK /4 -> 20 MHz DMA sample clock.

## Published transponder wire behavior

The published PIC16F18313 assembly and packet vectors establish:

- 5 MHz carrier generated from the external reference;
- complementary differential carrier outputs on RA0 and RA1;
- 12 bytes / 96 encoded bits per telegram;
- bytes sent most-significant bit first;
- preamble bytes `F9 16` for the RCHourglass telegrams considered here;
- logical one reverses carrier phase by 180 degrees;
- logical zero retains the current phase;
- one bit occupies four carrier periods, 0.8 microseconds;
- bit rate 1.25 Mbit/s;
- three ID packets followed by one rotating status packet;
- approximately 2.0-2.8 ms between telegrams.

Public discussion for older decoder changes also mentions `79 16` detection for
some RC4 Hybrid behavior. That is outside this first strict `F9 16` packet
implementation and must not be treated as the same validated payload format.

## Legacy payload behavior and vectors

The published manager/transponder artifacts expose a coding polynomial of
`0x776107`. Thirty ID/data bits are punctuated with ten status bits (three ID
bits then one status bit), and the resulting 40 bits become 80 coded bits.

Regression vectors:

| Telegram | Expected fields |
|---|---|
| `F9 16 EF DA 35 3B 28 29 0B 0B CF 3C` | ID 4,961,721; status 0 |
| `F9 16 E2 9B AD 5D 7B 44 79 87 7F 03` | ID 5,843,805; status 510 |
| `F9 16 DA E7 94 77 E9 3C 91 D7 C3 CC` | ID 7,632,095; status 76 |

The host test rebuilds and decodes these vectors and rejects a modified coded
payload.

## Why the new demodulator uses DMA samples

Polling P12[2] in ordinary C is not a reliable way to process a 5 MHz square
wave or a 1.25 Mbit/s phase stream. The new one-loop architecture therefore:

1. samples the P12 port at 20 MS/s by DMA;
2. keeps a circular 4096-byte window (204.8 microseconds);
3. triggers on P12[2] activity and freezes 100 microseconds later;
4. tries every sample offset in the frozen ring;
5. computes the differential relation `y[n] XOR y[n-16]`;
6. votes over five adjacent offsets;
7. looks for `F9 16`, captures 96 bits, and re-encodes the payload for strict
   validation.

A telegram lasts about 76.8 microseconds, so the post-trigger window contains a
complete packet while remaining small enough for PSoC 5LP SRAM.

## Output compatibility selected for stage one

The public serial wiki specifies 57600 baud, 8-N-1 and a 25-character
RCHourglass passage record:

```text
nnnnnntttttttt-idhhqqvvtm\r\n
```

This minimal firmware transmits that format from P12[7] only. It supplies the
six-digit ID, eight-digit quarter-millisecond timestamp, fixed decoder ID `01`,
packet hits, and independently calculated digital demodulation quality.
Voltage and temperature are `00` because those measurements are unavailable.
No RX pin, modes, learn commands, startup banner, configuration manager, or USB
CDC are implemented.
