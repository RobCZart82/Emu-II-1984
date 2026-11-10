# Development status

## M0 — in progress

- [x] M0-01: supplied documentation/license package preserved on main.
- [x] M0-02: pinned reference hardware-map extraction (not hardware validation).
- [x] M0-03: format notes and conservative filename/length hints with synthetic tests.
- [ ] M0-04: dual-instance instruction probe and dependency audit added. Runtime adoption and device scheduling remain open.
- [x] M0-05: ROM metadata matrix; all combinations explicitly untested.
- [x] M0-06: CMake/Catch2 memory, format and synthetic CPU tests.
- [x] M0-07: initial risk register.
- [ ] M0-08: pull request review and platform build results.

## Next ordered steps

1. Review the CPU probe and its cross-platform CI results.
2. Implement main memory page selectors with separate CPU/DMA tests.
3. Add deterministic scheduler and peripheral trace contracts (M1).
4. Reconstruct disk/OS flow (M2), then digital voices (M3), measured analog
   behavior (M4), host integration (M5), and release validation (M6).

No plugin framework, firmware boot, audio engine or release exists yet.
