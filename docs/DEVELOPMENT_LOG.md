# Development log

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
