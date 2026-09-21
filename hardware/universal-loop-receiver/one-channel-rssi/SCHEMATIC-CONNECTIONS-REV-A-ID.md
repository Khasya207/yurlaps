# ULR-RSSI-1 Rev A — daftar koneksi schematic

Status: netlist manusia untuk review; belum menggantikan schematic CAD/ERC

## 1. Pass-through RF/DC

```text
J1.1 RF_IN_CENTER  <-> J2.1 RF_THRU_CENTER
J1.2 CHASSIS       <-> J2.2 CHASSIS
CHASSIS            <-> AGND melalui opsi 0R/RC sesuai hasil enclosure test
```

Trace J1.1–J2.1 harus sangat pendek. Jalur ini membawa RF 5 MHz dan DC phantom power untuk amplifier Cano.

## 2. High-impedance tap

```text
J2.1 RF_THRU_CENTER -> R1 2.21k 1% -> C1 10n C0G -> U1.4 INHI
AGND                                  -> C2 10n C0G -> U1.5 INLO
```

Optional footprints:

```text
D1 low-capacitance ESD from node R1/C1 to AGND, DNP first prototype
C3 10p C0G across INHI/INLO, DNP
R2 1k across INHI/INLO, DNP; U1 already has nominal 1k input
```

## 3. AD8309 U1

Package: AD8309ARUZ, TSSOP-16.

| Pin | Name | Rev A connection |
|---:|---|---|
| 1 | COM2 | AGND, short return for VLOG |
| 2 | VPS1 | R3 10R from +3V3A; C4 100n + C5 1u to AGND |
| 3 | PADL | AGND with short via |
| 4 | INHI | from C1 |
| 5 | INLO | from C2 |
| 6 | PADL | AGND with short via |
| 7 | COM1 | AGND, short input return |
| 8 | ENBL | R4 10k to +3V3A; TP_EN; optional pull-down R5 100k |
| 9 | LMDR | DNP/open in Rev A |
| 10 | FLTR | C6 33p C0G to VLOG pin 16 |
| 11 | PADL | AGND with short via |
| 12 | LMLO | tied directly to VPS2 in Rev A |
| 13 | LMHI | tied directly to VPS2 in Rev A |
| 14 | PADL | AGND with short via |
| 15 | VPS2 | R6 10R from +3V3A; C7 100n + C8 1u to AGND |
| 16 | VLOG | to RSSI output network |

Pins 12/13 tied to VPS2 and LMDR open disable the limiter path according to the datasheet recommendation when only RSSI is used.

## 4. RSSI output

```text
U1.16 VLOG -> R7 47R -> net RSSI_OUT -> J4.3
                                      -> TP_RSSI
RSSI_OUT   -> C9 1n C0G -> AGND
AGND       ----------------> J4.2
+3V3A      ----------------> J4.1 (measurement only; do not back-power)
```

J4 pin 3 goes to provisional PSoC P3[0] configured high-impedance analog.

## 5. Analog power

```text
J3.1 +5V_IN -> C10 4.7u -> U2 LP5907-3.3 IN
J3.2 GND    ---------------- U2 GND
U2 OUT +3V3A -> C11 1u -> AGND
U2 EN tied to +5V_IN
```

Add reverse/current protection only after its noise impact is evaluated. Rev A is powered from decoder +5 V, not RF coax center.

## 6. Test points

```text
TP1 RF_IN_CENTER
TP2 AGND
TP3 +5V_IN
TP4 +3V3A
TP5 RSSI_OUT
TP6 ENBL
```

TP1 must be a small RF pad, not a long header stub.

## 7. Layout constraints

- U1, R1, C1, and C2 inside shield-can footprint.
- Keep LMHI/LMLO copper minimal even though disabled.
- Separate RF input area from VLOG/output connector.
- Continuous analog ground plane; no split under U1.
- Multiple vias at every PADL and decoupling ground.
- J1–J2 pass-through trace does not run under ESP32 or digital cables.
- No switching regulator on this PCB.
- Guard/fence vias around shield perimeter.
- Keep UART/ESP32 off this board in Rev A.
