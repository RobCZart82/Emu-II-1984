# Source inventory — 2026-11-09

| Source | Pinned revision | Use / audit scope |
| --- | --- | --- |
| [MAME](https://github.com/mamedev/mame/blob/d077ea24f0a73118bee9558bc939b99ae662bfc5/src/mame/emusys/emu2.cpp) | `d077ea24f0a73118bee9558bc939b99ae662bfc5` | Research only; emu2.cpp header BSD-3-Clause, AJR. No MAME code imported. |
| [ConvertWithMoss](https://github.com/git-moss/ConvertWithMoss/blob/aa89125fec206363cc3cc90ded8487a4acfd3397/documentation/design/EMULATOR2_FORMAT.md) | `aa89125fec206363cc3cc90ded8487a4acfd3397` | Format research only; no Java code ported. Document package reports LGPL-3.0; full dependency audit pending. |
| [kosarev/z80](https://github.com/kosarev/z80/tree/51d07188fd4ababfb465848b66ca65cc102b92cd) | `51d07188fd4ababfb465848b66ca65cc102b92cd` | Candidate only; MIT reported by supplied plan, integration/license verification pending. |
| [Catch2 v3.7.1](https://github.com/catchorg/Catch2/tree/fa43b77429ba76c462b1898d6cd2f2d7a9416b14) | `fa43b77429ba76c462b1898d6cd2f2d7a9416b14` | Test-only dependency fetched by CMake; upstream LICENSE.txt inspected, BSL-1.0. License retained upstream and copied to licenses/Catch2.txt. No core runtime dependency. |

Own source and new documentation: AGPL-3.0-only. Scanner map and size constants
express documented hardware/format facts; no upstream implementation copied.
Pinned sources make observations reproducible, not hardware-validated.

JUCE, VST3 SDK, HxC, libsamplerate and CLAP remain candidates from the original
plan. No versions chosen or code included; their audit remains pending.
Before integration record file-level license, authors, transitive dependencies,
linking/distribution constraints and retained notices. Do not infer approval
from presence in the supplied plan.
