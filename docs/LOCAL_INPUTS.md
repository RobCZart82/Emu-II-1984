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
member paths and parent traversals are rejected. Filesystem symlinks are not
inspected. Duplicate member names remain separate records with member indices.

Directory discovery includes known media/firmware extensions and ZIPs, in stable
path order. Explicit file arguments can inspect any file. Relative paths in a
report retain the caller's spelling; archive member names are never output paths.

## Tests and observed local corpus

Run `python3 tests/unit/input_inspector_tests.py`, or the normal CTest suite.
Thirteen synthetic Python tests cover identity despite misleading names, size
boundaries, malformed HFE storage, duplicate ZIP entries, corrupt CRC, path/link
rejection, resource limits, deterministic reports and CLI error status.

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
records for a future boot harness. Pair verification establishes the two
identities. Compatibility, PROM mapping and boot behavior remain open.
