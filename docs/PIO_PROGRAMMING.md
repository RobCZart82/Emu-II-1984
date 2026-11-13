# Scanner PIO programming boundary

Evidence: [ZiLOG Z80 PIO User’s Manual, chapter 4 and chapter 8](https://manualzz.com/doc/10892478/zilog-z80-pio-peripheral-i-o-controller-user-manual?lang=da).
Address decoding follows the [pinned MAME hardware reference](HARDWARE_MAP.md),
including its alternate register order. No upstream implementation was copied.

`PioProgramming` handles early control writes for two independent ports.
Even control bytes load each port’s vector. Interrupt control words retain
conditions and request an optional following mask; that mask takes precedence
over command decoding. Enable-only commands change bit 7 without replacing
other control bits. The reset operation clears mask/control programming while
preserving vector contents. An initially unknown vector is represented by
an empty optional, not a fabricated address.

This is a programming front end, not a complete PIO. Mode commands record the requested mode and return `requires_io`. In mode 3,
the next control byte is retained as the direction mask regardless of its
command-shaped bits. A new mode command clears the previous direction mask.
These records describe programming requests, while pin behavior remains pending. An interrupt-enable
request returns `requires_interrupts`, conservatively even before a requested
mask arrives. Unknown commands return `unsupported_control`. Callers must stop
at every result other than `accepted`.

The scanner trace decodes control A/B at 51/53 with mirror mask 8C.
Mode and interrupt boundaries become `unmodelled_pio_mode` and
`unmodelled_pio_interrupts`; reads, data writes, main-side PIO and unknown
commands retain generic unmodelled I/O events. There is no pin drive,
handshake, data latch, interrupt generation, acknowledgement or RETI service.

## Validation and next boundary

Synthetic tests cover both vector registers, reset preservation, independent
mask sequences including command-shaped bytes, interrupt condition preservation,
invalid indices, all control mirrors and explicit unsupported boundaries.

Two verified local Main 2.3 / Scanner 3.1 runs produced identical CSV bytes.
The scanner passes the previous step-50 F8DD output and reaches step 76,
PC 0738, T-state 1100, at output 3BBE. Low byte BE is outside the pinned
scanner map. Its purpose is unresolved; do not assume this is a known device
or widen an address mirror without evidence. Main remains at step 256,
PC 000D. These independent CPU probes do not establish a firmware boot.
