# Local ROM / OS inspection

`tools/inspect_inputs.py` is a read-only Python 3.9+ command-line tool. It reads
local files/ZIPs, hashes their contents and emits a JSON evidence inventory to
stdout. It never extracts archives, executes firmware, fetches data, modifies
input files or uploads anything. Run it outside the audio callback.

```sh
mkdir -p user-data
python3 tools/inspect_inputs.py /path/to/local/eprom > user-data/manifest.json
```

`user-data/` is ignored by Git. Manifests contain local paths; keep them local.
There is deliberately no built-in download or automatic ROM/OS pair selection.
Use only inputs you are entitled to use; this command does not grant permission
to redistribute them. No original firmware or disk fixture is part of CI.

## Evidence levels

- `reference_hash_match`: SHA-1 and length match a ROM identity from the pinned
  MAME declaration. SHA-256 is also recorded. Identity does not prove that a
  given main/scanner/OS combination boots or is accurate on real hardware.
- `extension_and_size_only`: an E2O, EII or raw disk has the expected common
  size. An all-zero file can satisfy this check; content remains unvalidated.
- `header_and_ranges_only`: HFE v1 signature, geometry and non-overlapping
  track storage bounds were checked. `eii_geometry_candidate` is only a header
  hint; no FM bitstream decoding, track CRC or OS boot is performed.
- `hashes_only`: unknown input, including unrecognized ROMs, PROM candidates,
  Intel HEX and documentation. It must not be promoted to firmware automatically.

Exit 0 means no inspection errors, **not** that every input is identified or
bootable. Exit 1 means one or more records have errors. Argument errors and
fatal I/O failures use exit 2. An error record may have no hashes. Never select
an input for use solely by the process exit code.

## Archive and resource handling

Direct files are limited to 8 MiB. ZIPs are limited to 64 MiB compressed and
expanded metadata total, 512 members, and 8 MiB per member. Limits are checked
before member decompression and reads are capped as a second boundary. Every
non-directory member is read for CRC verification, including non-media files.
Nested archives are inventoried as bytes, not recursively expanded. Encrypted,
unsupported-compression and corrupt entries are errors. ZIP links, absolute
member paths (including Windows drive and UNC forms) and parent traversals
are rejected. Filesystem symlinks are not
inspected. Duplicate member names remain separate records with member indices.

Directory discovery includes known media/firmware extensions and ZIPs, in stable
path order. Explicit file arguments can inspect any file. Relative paths in a
report retain the caller's spelling; archive member names are never output paths.

## Tests and observed local corpus

Run `python3 tests/unit/input_inspector_tests.py`, or the normal CTest suite.
Fifteen synthetic Python tests cover identity despite misleading names, size
boundaries, malformed HFE storage, duplicate ZIP entries, corrupt CRC, path/link
rejection, resource limits, deterministic reports and CLI error status.
A corrupt LZMA member is reported alongside later valid members and files.

The user's local 19-archive research corpus produced 41 records with zero
inspection errors. All five expected MAME ROM identities were found. These
counts describe one local snapshot; none of its bytes are committed here.

## Select an explicit ROM pair

Extract the two ROM files into separate local paths, then run:

```sh
python3 tools/inspect_inputs.py --main-rom user-data/main.bin --scanner-rom user-data/scanner.bin
```

Each file is read into memory and checked for its exact 8192-byte size,
reference hash and assigned role. The command accepts known main boot and
scanner identities, and rejects diagnostic images, unknown variants and
swapped roles. It prints a JSON report only after both files pass. The Python
entry point `load_rom_pair` returns the same verified bytes and identity
records for the offline Z80 trace tool. Pair verification establishes the two
identities. Compatibility, PROM mapping and boot behavior remain open.

## Trace the first Z80 steps

Build with testing enabled, then point the wrapper at the built executable:

```sh
python3 tools/trace_roms.py --runner build/z80_rom_trace \
  --main-rom user-data/main.bin --scanner-rom user-data/scanner.bin \
  > user-data/startup-trace.csv
```

With a multi-configuration Windows build, use the executable under
`build/Debug` or `build/Release`. The wrapper passes the verified bytes
to the runner through stdin. Keep the CSV local alongside the ROM identities.

The runner starts each CPU separately using its existing memory model. Each
CSV row records the CPU, step number, program counter and cumulative T-states.
Each step is one upstream `on_step()` call. DD and FD prefixes can occupy
their own rows. For `DD 21 34 12`, the prefix row ends at PC 1 with 4
T-states, and the next row ends at PC 4 with 14 cumulative T-states.
Main page-select outputs, scanner CTC vector/reset programming and early PIO
control programming are handled. The trace ends at other I/O, CTC timing
activation, PIO mode selection or interrupt enable, access outside mapped main
ROM/RAM, HALT or 256 steps per CPU. An unmodelled port row records the partial
step with no final program counter. The appended CSV field `memory_address`
records the translated physical address for `unmodelled_memory_read` and
`unmodelled_memory_write`. Port rows leave it empty. Earlier memory writes in
an interrupted instruction may already have happened. Addresses are printed
in decimal.

Two local runs with Main 2.3 and Scanner 3.1 yielded identical traces. Main
reached the 256-step limit at PC 0x000D. Scanner reached an output to port
0xF0EC on step 48. The low byte 0xEC mirrors scanner CTC channel 0
(0x60) in the [reference hardware map](HARDWARE_MAP.md).
The two CPUs are run independently, with shared peripheral timing, IRQ, DMA
and the floppy/OS boot path still pending.

The [CTC programming front end](CTC_PROGRAMMING.md) advances the scanner trace
past the recorded step-48 boundary to a PIO output at step 50 (F8DD). Timing,
CTC readback and interrupts remain future work.

The [PIO programming front end](PIO_PROGRAMMING.md) now passes the step-50
vector output. Repeated local runs reach step 76, an unsupported output to
3BBE (PC 0738). Its low byte BE was outside the pinned MAME scanner I/O map. The original
schematic now identifies it as IC47 Q7 / +SCBVD.D; see [SCANNER_IO.md](SCANNER_IO.md).
The subsequent [LCD function front end](LCD_FUNCTION_PROGRAMMING.md) handles
that first output and now reaches step 110, output 0CBE, with
`unmodelled_lcd_command`. The receiving circuit is the write-only LCD.
Other IC47 strobes and overlapping chip selects also stop with explicit events. PIO modes, data
transfers, readback and interrupt service are not implemented.
