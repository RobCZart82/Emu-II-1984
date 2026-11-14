# LCD function programming boundary

## Wiring and controller evidence

The local `EmulatorII_Schematics.pdf` identified in SCANNER_IO.md includes
left-panel S-6153-001 sheets 7-3 and 7-5 (PDF pages 4 and 6), and scanner
interface S-6149-001 sheet 7-9 (PDF page 10). The connection chain is:

- IC47 Q7 inverted -> +SCBVD.D -> CN3.25 -> LCD connector CN4.6 (enable).
- Scanner SA0 -> CN3.24 -> LCD CN4.4 (register select).
- CN4.5 is grounded (write direction only).
- SDB7..SDB0 -> CN3.29..36 -> CN4.14..7, respectively.

SA0 is independent of SDB2; connector pins 24 and 34 are distinct.
The data-buffer sheet 7-7 and left-panel sheet 7-3 establish the data path.
No original schematic or rendered image is distributed here.

Command behavior follows the Hitachi [HD44780U manual, ADE-207-272(Z)](https://www.sparkfun.com/datasheets/LCD/HD44780.pdf),
sections on 4/8-bit transfers, function set, reset and Table 6.
HD44780 compatibility is a research hypothesis for the LCD216 module,
consistent with its interface and the observed function command. The actual
installed controller and oscillator have not been measured or identified.

## Implemented subset

`LcdFunctionProgramming` models function-set writes only. In 8-bit mode a
whole byte is decoded. In 4-bit mode the upper nibble of two consecutive
control writes forms a command; lower bus bits are ignored. Bus-width changes
apply only when the command is complete. Raw function bits are retained;
there is no rendered font or display geometry yet.

The model starts ready in 8-bit mode with no observed function command.
This is an explicit post-power-up research convention. It does not simulate
cold-start voltage conditions, the initial reset delay or the required
software power-up initialization sequence.

A completed function write makes the model busy for 148 scanner T-states
(37 microseconds at 4 MHz, based on the manual's nominal timing reference).
This is not a measurement of the original module's oscillator. A partial
4-bit transfer does not start that interval. The offline adapter timestamps
the upstream CPU output callback as its transaction time; electrical enable
edges, pulse widths and setup/hold times are not resolved.

Other commands, data writes, too-early writes, non-monotonic timestamps and
deadline overflow return explicit boundaries without changing model state.
Callers must stop at a result other than `accepted`. reset() restores the
research convention, not a validated hardware reset sequence.

## Offline trace integration

Only a non-overlapping BVD output is routed here. The port's low bit selects
control/data. A CPU input at BVD still pulses a write-only peripheral while
CPU bus data are unknown, so it stops as `unmodelled_lcd_bus_read`.
Other boundaries are `unmodelled_lcd_command`, `unmodelled_lcd_data`,
`unmodelled_lcd_busy` and `unmodelled_lcd_time`. Other scanner strobes and
all overlapping device selects keep their previous boundaries.

Synthetic tests cover width changes in both directions, nibble assembly,
RS selection independently of bus bit 2, exact busy deadlines, rejected
commands/data, reset/isolation, reversed time and overflow. The integration
suite exercises all four BVD aliases and a too-early second write.

Two verified local Main 2.3 / Scanner 3.1 runs produced identical CSV bytes.
Scanner step 76 now completes at PC 073A and T-state 1101. It reaches step
110, PC 0738, T-state 1454, output 0CBE, and stops with
`unmodelled_lcd_command` (byte 0C). Main remains at step 256, PC 000D.
This advances initialization; it does not establish cold boot, display text
or audio. Next implement display control and the remaining LCD commands,
then validate the power-up/timing assumptions against hardware evidence.
