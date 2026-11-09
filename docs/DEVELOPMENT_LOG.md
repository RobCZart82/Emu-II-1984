# Development log

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
