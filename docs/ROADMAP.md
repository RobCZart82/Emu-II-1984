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

The CPU probe has passed CI on Windows, macOS and Linux. The main CPU/DMA
page selectors now have independent synthetic tests.

1. Deterministic event queue and trace contract added (see SCHEDULER.md).
   Resolve cycle-yielding CPU adapters, IRQ and actual DMA ownership before
   treating it as an integrated dual-CPU scheduler (M1).
2. Local ROM/media inspection, explicit main/scanner ROM selection and bounded
   offline startup tracing are implemented (see LOCAL_INPUTS.md). The trace
   stops at unmodelled memory or I/O. Next integrate the scanner CTC, shared
   CPU/device timing and verify PROM wiring against schematics.
3. Reconstruct disk/OS flow (M2), then digital voices (M3), measured analog
   behavior (M4), host integration (M5), and release validation (M6).

No plugin framework, firmware boot, audio engine or release exists yet.
