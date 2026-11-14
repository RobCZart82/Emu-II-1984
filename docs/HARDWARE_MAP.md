# Initial hardware map — M0-02

Evidence: [MAME emu2.cpp at d077ea24f0a7](https://github.com/mamedev/mame/blob/d077ea24f0a73118bee9558bc939b99ae662bfc5/src/mame/emusys/emu2.cpp).
Research model, not a complete verified hardware specification. Scanner decode
is refined from the original schematic in [SCANNER_IO.md](SCANNER_IO.md); the
port list below retains the original reference windows.

Two 4 MHz Z80s; main and scanner have separate address spaces.
DMA (ports 68–69) and main CPU (6A–6B) each have two 32 KiB page
selectors, for the lower and upper halves of their 16-bit address space.
The reference latches each selector's low four bits. Reset forces all four
windows to page zero until the first normal selector write and preserves
previous latch contents. Only ROM 00000–01FFF and RAM 02000–027FF
are mapped in the reference's 19-bit backing space; the rest is incomplete.
Scanner ROM: 0000–1FFF mirrored through 7FFF; RAM: 8000–87FF mirrored through FFFF.

| Main I/O (hex, low byte) | Device |
| --- | --- |
| 00–03 read / 04–07 write | SCN2651 |
| 08–1B write, mirrors | Three 8254 timers |
| 20–27 | DMA |
| 28–2F | CTC |
| 30–37 | PIO |
| 38–3F | Disk SIO |
| 68–6B write | Page selectors |

Scanner: ADC at 34 read / 36 write (mirror 81), scan buffer 38 read
(mirror 81), PIO 50–53 and CTC 60–63 (mirror 8C).
Main IRQ priority: disk SIO, DMA, CTC, PIO; scanner: CTC then PIO.
PIO alternate register order is data A, control A, data B, control B
(50, 51, 52, 53 on the scanner; main base 30). PIO A links CPUs. Scanner PIO B selects scan input; ADC EOC triggers CTC.

Unknown: disk handshake/completion, sound-engine logic, IC135/IC137 PROMs.
The project models scanner memory and the main CPU/DMA page translation.
Both main-side buses share the same ROM and RAM backing. Unmapped reads return
0xFF and writes are ignored as a model convention. Clocks and peripheral I/O
remain future work.

The scanner decoder now models the active-low SA5 PIO and SA4 CTC selects,
and the SA6-gated IC47 outputs. It reports simultaneous selections instead
of assuming exclusive devices. The scanner's observed BE output selects
IC47 Q7 / +SCBVD.D; its receiving circuit is the write-only LCD. The
[function-programming subset](LCD_FUNCTION_PROGRAMMING.md) is implemented;
other LCD commands and electrical/power-up timing remain pending.
