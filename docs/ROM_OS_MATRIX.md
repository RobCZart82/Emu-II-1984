# ROM / OS evidence matrix — M0-05

Metadata source: [pinned MAME ROM declarations](https://github.com/mamedev/mame/blob/d077ea24f0a73118bee9558bc939b99ae662bfc5/src/mame/emusys/emu2.cpp).
These identifiers are reference metadata, not files present in this project.

| Role | Version | Bytes | Reference SHA-1 | Compatibility |
| --- | --- | ---: | --- | --- |
| Main boot | 2.3 | 8192 | 4ec4bf6df95638d4503ab8ff7e859c420ec0720e | Untested |
| Main boot | 2.1 | 8192 | a2332266d6a12eade584b5da85817f3b3ed9037c | Untested |
| Main diagnostics | debug | 8192 | d572d88c8788e25154a3bb8fd2ba3ff8a300ae40 | Untested |
| Scanner | 2.1 | 8192 | 4485e5423606014be73f121c2ee1f053a6eb148a | Untested |
| Scanner | 3.1 | 8192 | e354388cd694a1ce55a5b7d89c546308628090c7 | Untested |
| OS floppy/E2O | No input selected | Unknown | Not measured | Untested with every pair |
| IC135/IC137 | NO_DUMP | 512 each | Unknown | Critical blocker |

The reference loads two scanner versions into one region and marks selection
as TODO. It is not evidence that either scanner works with every main ROM.

For each lawful private input, record provenance, permission, local SHA-256,
version, chosen main/scanner/OS triple and reproducible trace. Keep binaries in
ignored user-data outside tracked fixtures. A hash does not grant redistribution
rights. Commit only synthetic fixtures and metadata. No ROM acquisition,
firmware execution or compatibility claim has been made.
