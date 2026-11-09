# CPU decision — M0-04 pending

First candidate: kosarev/z80, exact research revision in SOURCE_AUDIT.md.
No CPU dependency has been integrated and no CPU instruction tests pass yet.
The scanner-memory tests must not be described as CPU validation.

Before adoption: inspect z80.h and full license, run upstream instruction tests,
prove two isolated CPU instances, document bus callbacks and cycle granularity,
IRQ/WAIT/DMA handover and reset. Then define integer master-tick ordering for
main CPU, scanner and devices; compare uninterrupted and chunked execution.
Persist and replay state with identical traces before attempting real ROM boot.

M0-06 is partial until at least one actual CPU test is added. Framework and DSP
decisions stay open until later milestones.
