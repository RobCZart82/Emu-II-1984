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
| IC135 | Local archive candidate; MAME NO_DUMP | 512 | a2adbfb412897307277b331cbda2c250def6d172 | Bit mapping / authenticity unverified |
| IC137 | Local archive candidate; MAME NO_DUMP | 512 | 0bd6a6acab799231454c001b1d600000f78de345 | Bit mapping / authenticity unverified |

The reference loads two scanner versions into one region and marks selection
as TODO. It is not evidence that either scanner works with every main ROM.

For each lawful private input, record provenance, permission, local SHA-256,
version, chosen main/scanner/OS triple and reproducible trace. Keep binaries in
ignored user-data outside tracked fixtures. A hash does not grant redistribution
rights. Commit only synthetic fixtures and metadata. The offline trace observations in LOCAL_INPUTS.md are a later research step.
ROM/OS compatibility remains unverified.

## Local research evidence (2026-11-10)

All five ROM identities above were found in a local input corpus. Aliases named
Scanner 3-0 and Scanner 3-1 contain identical 3.1 reference bytes; Main 2-0b and
Main 2-1 aliases likewise match the 2.1 reference. Another Mainboot 2.1-labelled
file has SHA-1 f3b92110a87cf1e57f4cc7f6b082c26ce15519c3 and differs by nine bytes;
keep it unclassified rather than silently treating it as the reference.

The local IC135/IC137 candidates are nonconstant 512-byte files labelled
82S147N. MAME names the missing parts 74S472. Check the actual circuit revision,
address/data pin mapping and polarity before adoption. NO_DUMP does not mean
that no archive candidate exists; it does not authenticate these local files.

Local 3.0, 3.1, 2.6HD and 3.1HD E2O files match the first 72704 bytes of their
respective raw OS disks. This consistency is not proof of successful boot.
The reproducible [input inspector](LOCAL_INPUTS.md) records evidence without
bundling binaries or selecting an untested ROM/OS triple.
