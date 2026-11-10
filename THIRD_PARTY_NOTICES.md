# Third-Party Sources and Licence Notes

This file lists **research references and candidate dependencies**, not necessarily code included in the repository. The build fetches Catch2 and kosarev/z80 for tests. **No third-party source code, firmware ROM, operating-system image, sample bank, or sound library is bundled by this documentation package.** Update this file when any dependency is actually added.

| Reference | Upstream | Observed licence / usage context | Included? |
| --- | --- | --- | --- |
| MAME Emulator II driver (`emu2.cpp`) | https://github.com/mamedev/mame/blob/master/src/mame/emusys/emu2.cpp | `BSD-3-Clause` file header; other MAME files may have other terms | No |
| kosarev/z80 | https://github.com/kosarev/z80 | MIT | Fetched for tests |
| ConvertWithMoss | https://github.com/git-moss/ConvertWithMoss | LGPL-3.0 | No |
| JUCE | https://github.com/juce-framework/JUCE | AGPLv3 or commercial framework licence | No |
| VST3 SDK | https://github.com/steinbergmedia/vst3sdk | MIT for VST3 SDK 3.8+; check the pinned version | No |

## Integration policy

1. Keep third-party copyright notices and licence text as required by each dependency.
2. Audit the **actual files**, transitive dependencies, and linking/distribution model rather than assuming a whole repository has one licence.
3. Do not copy external code merely because it is viewable on GitHub; establish permission and compatibility first.
4. Do not distribute original E-mu firmware ROMs, OS images, factory banks, or third-party samples without the necessary rights.
5. If a commercial JUCE licence is obtained in future, review the licence of **this project's own AGPL-covered code separately**; one does not automatically change the other.

The project's own source/documentation is offered under **AGPL-3.0-only** unless another notice applies. See [LICENSE](LICENSE) and [README.md](README.md).

## Test dependencies

Catch2 v3.7.1, BSL-1.0, is fetched only for tests at commit `fa43b77429ba76c462b1898d6cd2f2d7a9416b14`. See [retained license](licenses/Catch2.txt) and [audit](docs/SOURCE_AUDIT.md). It is not linked into emu_core.

kosarev/z80 is fetched at `51d07188fd4ababfb465848b66ca65cc102b92cd`. Only
`z80.h` is used by the test target. See the [retained MIT notice](licenses/KosarevZ80.txt)
and the [evaluation](docs/CPU_DECISION.md).
