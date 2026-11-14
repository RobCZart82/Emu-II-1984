# Scanner I/O chip selects and IC47 strobes

## Evidence

The user's local `EmulatorII_Schematics.pdf`, E-mu Systems S-6149-001:

- PDF page 7, printed 7-6, revision 0.0 dated 850605: scanner CPU,
  IC45 CTC and IC47 74LS138 chip-select logic.
- PDF page 9, printed 7-8: scanner pushbutton buffer and ADC.
- PDF page 10, printed 7-9: IC46 PIO and scanner/main interface.

Local PDF SHA-256:
`86ccbebcaeb0649535ca17af74a8c33f1f6104ce06446c7c2f27471fbba3fe69`.
The document and rendered pages are not distributed in this repository.
The [TI SN74LS138 datasheet](https://www.ti.com/lit/ds/symlink/sn74ls138.pdf)
provides the decoder's enable polarity and truth table. The previous
[MAME map](HARDWARE_MAP.md) describes ordinary device windows but omits
several IC47 outputs and overlapping select combinations.

## Implemented decode

`decode_scanner_io` accepts a 16-bit port for an ordinary active I/O read or
write cycle. It does not represent idle, memory or interrupt-acknowledge cycles.

IC46 /CE is SA5, IC45 /CE is SA4. A low bit selects the corresponding chip.
IC47 /G2A is SA6, /G2B is /SIORQ and G1 is the NAND of /SWR and /SRD.
Thus an ordinary I/O cycle selects one IC47 output when SA6 is low.
SA3..SA1 select Q0..Q7. SA0, SA7 and the port's high byte do not affect IC47.
The PIO/CTC register index remains SA1..SA0.

| IC47 output | Pure-strobe low ports (SA5=SA4=1) | Wire label |
| --- | --- | --- |
| Q0 | 30/31/B0/B1 | -SCWL0.D |
| Q1 | 32/33/B2/B3 | -SCWL1.D |
| Q2 | 34/35/B4/B5 | -SCRAC.D |
| Q3 | 36/37/B6/B7 | -SCWC.D |
| Q4 | 38/39/B8/B9 | -SCRPB.D |
| Q5 | 3A/3B/BA/BB | -SCWL2.D |
| Q6 | 3C/3D/BC/BD | -SCMET.D |
| Q7 | 3E/3F/BE/BF | +SCBVD.D (Q7 inverted by IC25) |

+SCBVD.D leaves the CPU board on CN3 pin 25. This identifies the decoded
signal at the observed BE access, not the receiving peripheral's protocol.
The model does not expand BVD into an assumed functional name.

PIO and CTC are not mutually exclusive with each other or IC47. For example,
port 00 selects both chips and Q0; 11 selects PIO and Q0; 20 selects CTC and
Q0; 40 selects both chips without IC47. Selection records retain every active
path. Do not replace overlaps with a priority decoder.

## Trace boundaries and validation

The trace checks overlaps before invoking any device programming method.
Overlaps stop as `unmodelled_scanner_overlap`. A sole Q7 output now reaches
the [LCD function front end](LCD_FUNCTION_PROGRAMMING.md);
Q7 inputs stop because the peripheral is write-only. Other sole IC47 outputs stop as
`unmodelled_scanner_strobe`. IC47 decodes either ordinary I/O cycle; a CPU
read does not make the LCD
readable because its R/W pin is grounded. Existing PIO/CTC handling is
retained for their non-overlapping windows. No strobe is silently ignored.

Tests enumerate all 65536 port values against an independent selection truth
table, retain ordinary PIO/CTC mirrors, and exercise every IC47 output and
SA0/SA7 alias through read/write synthetic CPU programs. Overlap programs
verify the trace stops instead of accepting a vector write.

The initial decoder-only Main 2.3 / Scanner 3.1 runs were byte-identical. Main stopped
at step 256, PC 000D; scanner at step 76, PC 0738, T-state 1100, port 3BBE.
This was the decoder-only boundary. CN3.25 has since been traced to the LCD
enable input, and [function programming](LCD_FUNCTION_PROGRAMMING.md) now
passes step 76 and reaches the next LCD command at step 110. Power-up and
electrical timing remain unverified; no firmware boot or audio is claimed.
