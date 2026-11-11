# Scanner CTC programming boundary

Evidence: ZiLOG Z80 Family CPU Peripherals User Manual, UM008101-0601,
CTC programming pages 18–23, available through this
[manual mirror](https://manualzz.com/doc/11065388/zilog-z84c10-microcontroller-user-manual).
Port mirrors follow the pinned MAME reference in HARDWARE_MAP.md.

The front end retains four channel programming records and the channel-zero
interrupt vector base. A pending constant consumes the next byte regardless
of its low bit. A zero constant denotes 256. Vector bits 7–3 are retained.
Control writes with the software-reset bit set are accepted. Loading a
constant or writing a control word without reset returns requires_timing.
Even words outside channel zero return unsupported_vector_channel.

The offline scanner trace uses this front end for 60–63 with mirror mask 8C.
It continues past vector/reset programming and stops with
unmodelled_ctc_timing when execution needs timing behavior. CTC reads keep
the existing unmodelled_in boundary. CPU and trace formats are preserved.

Records start with last_control=3, no constant and no vector as a deterministic
research convention. last_control retains the last written control byte.
reset() clears this research state. Hardware power-on values require separate
validation. Timer/counter clocks, readback, interrupts and daisy-chain service
are upcoming integration work. A requires_timing result ends the trace.

Local Main 2.3 / Scanner 3.1 runs gave byte-identical traces on repetition.
The scanner passes its previous step-48 vector output and stops at step 50,
port F8DD, whose low byte DD mirrors scanner PIO register 1 (51). Main remains
at the same step limit. This is progress through isolated initialization.

Tests cover vector masking, all four channels, constant precedence and range,
reset, invalid indices, all eight channel-zero mirrors, timing activation,
unsupported reads and adjacent PIO accesses. CI uses synthetic programs.
