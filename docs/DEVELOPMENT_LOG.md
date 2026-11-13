# Development log

## DEV-0009 — 2026-11-13 — PIO mode/direction programming records

The PIO front end now retains requested modes and consumes the mode-3
direction byte before command decoding. Mode changes clear stale direction
data and reset clears the programming sequence while preserving vectors.
The offline trace still stops at mode selection with requires_io.
Synthetic tests cover command-shaped direction bytes, port isolation and reset.

## DEV-0008 — 2026-11-13 — Optimized Python regression coverage

The ROM trace test now runs through CTest with both normal Python and
explicit -O. This extends the preceding assertion fix with recurring
Windows/macOS/Linux coverage in Debug and Release configurations.
Both entries use the same synthetic trace scenarios and built runner.

## DEV-0007 — 2026-11-11 — Repository audit and memory trace boundaries

Audited baseline `3b310fece03bbbed60001ed312c18802d046281d`.
The offline trace previously continued through unmapped main memory using
the memory model's 0xFF fallback. Added a mapping query and stop records for
unmodelled reads and writes, including physical addresses. Tests exercise
unmapped opcode fetch, data reads, writes and page-select transitions.
See [audit findings](AUDIT_2026-11-11.md) for scope and remaining work.

## DEV-0006 — 2026-11-10 — Local input evidence inspector

Added a read-only Python inventory tool with ROM reference-hash identities,
SHA-256 records, conservative media hints and HFE v1 storage-bound checks.
ZIPs are read without extraction; CRC errors, duplicate names and resource
limits are handled explicitly. Unknown inputs are not automatically trusted.

Eleven synthetic Python tests join the existing 18 C++ cases through CTest.
Local macOS Debug and Release pass all 19 CTest entries (18 C++ cases plus the
11-case Python suite); BUILD_TESTING=OFF core builds independently.
The local downloaded 19-ZIP corpus produced 41 records and zero inspection
errors, including matches for all five MAME main/scanner/diagnostic identities.
Firmware and media remain local; CI uses only synthetic data.

Corrected the ROM matrix: local IC135/IC137 candidates exist, but pin mapping,
polarity and authenticity remain unverified. NO_DUMP is the upstream MAME
state rather than proof of global absence. No original PDF/binary was copied
into the repository. No firmware was executed and no compatible boot triple
was claimed. Runtime loader integration remains a separate next step.


## DEV-0005 — 2026-11-10 — Deterministic event scheduling

Baseline: ff506c8 (main CPU/DMA paging and earlier dual-Z80 probe already merged).
Added a single-threaded uint64 event scheduler, inclusive deadlines, explicit
same-tick phase/FIFO ordering and stable trace records. Handler-enqueued events,
phase inversion, recursion and exceptions have defined behavior. A synthetic
bus-ownership example exercises the existing MainMemory model. This is not
cycle-level CPU integration or an implemented DMA controller.

Local macOS arm64 Debug and Release: 18/18 test cases pass in each configuration.
The dependency-free BUILD_TESTING=OFF core also builds. No external source or
dependency changes. Remote platform evidence is attached to this pull request.

CI fix: feature branches run checks via pull_request; push runs are retained
for main. This avoids duplicate feature push/PR jobs with identical check names.
Manual workflow dispatch remains available. Required check names are unchanged.

Next: select a CPU bus-yield strategy, then add IRQ/device and DMA arbitration
integration tests; hardware boot and audio remain unimplemented.


## DEV-0004 — 2026-11-10 — Main CPU/DMA memory paging

The CPU probe landed as `cf4233e8ddb178f6adffc45af255a0b249e507d7`.
[Main CI](https://github.com/RobCZart82/Emu-II-1984/actions/runs/42777875404)
passed. Implemented the four page selectors, page-zero reset override, and
shared ROM/RAM backing. Corrected the port assignment in the hardware map
after checking the pinned reference's CPU and DMA accessors. Synthetic tests
cover four windows, latch persistence, shared RAM, and ROM write protection.

## DEV-0003 — 2026-11-10 — M0 CPU evaluation

Reviewed the source, synthetic tests, reference maps and build instructions
at baseline `dc1e9f5b51390e633a2cc84d2759f124b654744e`.
[Baseline CI](https://github.com/RobCZart82/Emu-II-1984/actions/runs/42708948184)
passed all six Debug/Release jobs on Windows, macOS and Linux.

Pinned the MIT-licensed kosarev/z80 header for a test-only, dual-instance
instruction probe. The tests cover register and bus isolation, T-state
totals, write and I/O timestamps, and independent CPU reset. See
[CPU evaluation](CPU_DECISION.md) for limitations and upstream results.
The next hardware task is the independent main CPU/DMA page map.

## DEV-0002 — 2026-11-09 — M0 foundation

Supplied README/license package preserved on main in commit 35bcf70. Original
DEVELOPMENT_PLAN_HU.txt remains unchanged as historical planning evidence.

Added source revisions, hardware map, untested ROM/OS metadata matrix, format
notes, CPU decision criteria, risk register and ordered roadmap. Implemented
scanner ROM/RAM address aliases and conservative format hints. CPU execution
and a dual-CPU scheduler remain pending, so M0 is not declared complete.

Validation: local macOS arm64, AppleClang 21.0.0, C++20; Debug and Release
builds each pass all 6 Catch2 test cases. Tests cover all 65536 scanner addresses,
ROM write protection, rejected load lengths, memory-instance isolation and
file type/size boundaries. No copyrighted binary fixtures used.

Windows/macOS/Linux Debug/Release workflow added. Remote results are recorded
in the pull request checks, independently of local validation. Review is the
next acceptance gate; no release or hardware fidelity is claimed.

Initial remote macOS jobs could not start: GitHub reported the macos-14 runner
configuration unsupported. Changed to macos-latest; no source/test failure was
reported by those cancelled jobs.
