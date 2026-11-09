# Format investigation (M0-03)

Reference: ConvertWithMoss revision recorded in SOURCE_AUDIT.md,
`documentation/design/EMULATOR2_FORMAT.md`. These are research observations,
not a guarantee that every historical variant has the same layout.

| Input | Observed size | Role |
| --- | ---: | --- |
| IMG / EMUIIFD | 573440 | Raw disk, 160 tracks of 3584 bytes |
| E2O | 72704 | Separate operating-system data |
| EII | 485887 | Common extracted bank, excludes OS |
| HFE | variable | Track container; requires header and track validation |

Raw bank region starts at disk offset 0x13400 (track 22). The saved memory
extent is 485888 bytes; the common extracted EII file is one byte shorter.
Do not silently pad or assume the missing byte's meaning. Header-prefixed bank
variants remain unsupported by the initial hint function.

`format_hint` is deliberately only a filename/length candidate classifier.
Even a correctly sized all-zero file produces a hint. M2 must validate content,
track bounds, container encoding and CRC before loading. No API boots a bank.

Next: independently specified synthetic raw-track fixtures and malformed input
cases; reflected CRC polynomial 0xA001, initial zero, payload-only, low byte first
is a research lead requiring independent vectors before implementation.
