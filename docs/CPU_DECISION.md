# CPU decision — M0-04 evaluation

Evaluate [kosarev/z80 at `51d07188fd4ababfb465848b66ca65cc102b92cd`](https://github.com/kosarev/z80/tree/51d07188fd4ababfb465848b66ca65cc102b92cd)
in the test target. The candidate exposes a C++ CPU in a single header, which
uses only C/C++ standard headers. See the [source inventory](SOURCE_AUDIT.md)
and [retained MIT notice](../licenses/KosarevZ80.txt).

## What the probe covers

The main-side CPU uses a flat synthetic bus and the scanner CPU uses
`ScannerMemory`. Each CPU advances by one upstream `on_step()` call. The
probe interleaves the instances, checks their registers and memory isolation,
and verifies 7, 13, 11 and 11 T-states for immediate load, memory store,
input and output. Memory writes and I/O are observed at cumulative ticks
19, 30 and 41. Port callbacks preserve the full 16-bit address and can
return a data byte of `0xff`. A second test resets one CPU while checking
that both external memory instances and the other CPU retain their state.

## Integration limits

- DD and FD prefixes consume separate `on_step()` calls at this revision.
  For example, `DD 21 34 12` (`LD IX,1234h`) advances to PC 1 after 4
  T-states, then to PC 4 after 14 cumulative T-states. Trace rows and the
  step limit count these upstream steps. Consumers that count complete
  instructions must account for prefix state.
- `on_tick(unsigned)` reports elapsed T-states in batches. Bus callbacks run
  synchronously during the instruction. An instruction can span another
  device's deadline, so alternating calls to `on_step()` is insufficient
  for a cycle-ordered scheduler.
- `z80::retry_input` is the maximum value of `fast_u8` at this pin. A ready
  port byte and the retry sentinel must stay distinct across host types.
  The probe checks that distinction. Retry rolls back CPU execution state
  in the upstream machine when its state hooks are enabled. It is a
  separate mechanism from hardware WAIT and external device rollback.
- Maskable interrupts require explicit polling via `on_handle_active_int()`.
  The source exposes NMI and an IM 2 vector callback. The IM 0 path has a
  fixed RST 38h fallback. Emulator II IRQ priority, daisy-chain behavior,
  WAIT sampling, and DMA ownership still require integration tests.
- Resetting the CPU and clearing external RAM are separate operations.
  The probe's monotonic tick counter belongs to the harness, not the CPU.
- [Upstream C/C++ CI](https://github.com/kosarev/z80/actions/runs/40180150189)
  passed for this exact revision on Linux. Its combined Z80/8080 test
  driver fails to compile with local MSVC 19.44 because two overloads
  use `fast_u8` and `fast_u16`, which alias the same type on that host.
  That driver is outside the project's test target.

The main CPU/DMA page selectors now have a separate memory model. The offline
[scheduler contract](SCHEDULER.md) now defines phase/FIFO ordering and tests
one-shot/chunked traces and synthetic reset/bus ownership. It does not connect
the CPU probe to cycle-level events. IRQ boundaries and actual DMA/WAIT remain
unvalidated.
Defer runtime adoption until these integration constraints are resolved.
