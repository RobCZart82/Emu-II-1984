# Deterministic event scheduler — M1 foundation

## Contract

`Scheduler` orders events by unsigned 64-bit tick, phase, then a monotonically
assigned insertion sequence. `run_until(deadline, handler)` includes the deadline.
An empty queue still advances virtual time to that deadline. It does not wait
for wall-clock time. Tags are caller-defined unsigned 32-bit identifiers;
events are value records, not pointers to a device or CPU instance.

The initial same-tick policy is:

1. reset
2. bus ownership changes
3. device events (including future IRQ producers)
4. main CPU observation
5. scanner CPU observation

This is a **project contract**, not measured Emulator II arbitration. The
integer tick has no implicit conversion to seconds. A future 20 MHz master
clock adapter must explicitly map every device's clock and test edge alignment.
No CPU adapter or peripheral IRQ/DMA implementation is included here.

A handler may enqueue current/future events. Current-tick events must have the
same or a later phase than the last dispatched phase; earlier phases and past
ticks are rejected. This ordering floor survives repeated calls at the same
deadline, so splitting a run cannot silently reorder same-tick events. Within
one phase, newly queued events follow previously inserted events. If virtual
time advances beyond the last event, the floor is cleared for the new tick.

Run calls may not recurse. Empty handlers and backward deadlines fail before
changing the queue. An event is removed before its handler runs; if that handler
throws, time stays at its tick, its phase floor is retained and remaining events
stay queued. The failed event is not retried and device side effects are not
rolled back. Resume only if the caller can recover its own device state.
Sequence exhaustion throws rather than wrapping; the maximum sequence value
is reserved. Absolute deadlines permit UINT64_MAX without tick arithmetic.

## Trace and bus ownership

Each delivered record contains `(tick, phase, tag, sequence)`. Copying these
records produces a stable in-memory trace for identical scheduling inputs.
Compare complete records, not only event counts. This is not yet a portable
save-state format. Queues cannot be copied/moved; serialization is deferred.

The synthetic integration test applies a reset and DMA-ownership event before
a device writes shared main memory, then suppresses a CPU write while the bus
is owned. A later bus-release event allows the CPU write. Bus gating is done
by the test handler; `Scheduler` itself does not arbitrate or stall CPUs.
In a future adapter, register actual bus transactions at their deadlines;
do not call a whole Z80 instruction at a CPU event and claim cycle accuracy.
The current CPU probe cannot yield mid-instruction.

## Limits and evidence

Seven new tests cover phase/FIFO ordering, inclusive deadlines, handler-added
events, one-shot versus chunked trace equality, past-time rejection, recursion,
exception recovery, full-range timestamps and synthetic DMA ownership. Existing
memory and CPU tests remain part of the same suite.

This is single-threaded offline research code. Dynamic queue storage and the
handler wrapper may allocate. Finite dispatch is the caller's responsibility:
an unbounded stream of same-tick events will not terminate. There is no real-time
audio safety, queue-size cap, cancellation, save-state, reset-domain model or
hardware validation. Next: resolve CPU bus-yield boundaries, then device and
interrupt adapter contracts before attempting firmware boot.
