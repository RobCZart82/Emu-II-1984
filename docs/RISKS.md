# Risk register — M0-07

| Priority | Unknown | Next evidence / mitigation |
| --- | --- | --- |
| Critical | IC135/IC137 sound-generator PROM logic unavailable | Research schematics and measured behavior; record functional approximations explicitly |
| High | ROM/OS pairing and provenance | Private lawful inputs, metadata and reproducible boot trace |
| High | CPU/DMA/device timing | Dual-instance tests, bus events and deterministic replay |
| High | Disk controller and FM alignment | Independent synthetic tracks, corrupt/truncated fixtures and CRC vectors |
| High | SSM2045 behavior | Circuit/measurement study before fidelity claims |
| High | External licenses and dependencies | Pin exact revisions, audit files and retain notices before import |
| Medium | Audio-thread allocation and host state | Keep file I/O outside callback; test state round trips in M5 |

No mitigation in this table implies a solved hardware behavior.
