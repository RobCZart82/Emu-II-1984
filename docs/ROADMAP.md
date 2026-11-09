# Development status

## M0 — in progress

- [x] M0-01: supplied documentation/license package preserved on main.
- [x] M0-02: pinned reference hardware-map extraction (not hardware validation).
- [x] M0-03: format notes and conservative filename/length hints with synthetic tests.
- [ ] M0-04: CPU evaluation and dual-instance timing prototype.
- [x] M0-05: ROM metadata matrix; all combinations explicitly untested.
- [ ] M0-06: CMake/Catch2 memory and format tests ready; actual CPU tests pending.
- [x] M0-07: initial risk register.
- [ ] M0-08: pull request review and platform build results.

## Next ordered steps

1. Review this foundation and cross-platform CI; resolve failures before merge.
2. Audit and pin kosarev/z80; add dual-instance synthetic instruction tests.
3. Implement main memory page selectors with separate CPU/DMA tests.
4. Add deterministic scheduler and peripheral trace contracts (M1).
5. Reconstruct disk/OS flow (M2), then digital voices (M3), measured analog
   behavior (M4), host integration (M5), and release validation (M6).

No plugin framework, firmware boot, audio engine or release exists yet.
