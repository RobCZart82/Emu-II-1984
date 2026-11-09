# Build the research core

Requirements: CMake 3.24+, a C++20 compiler, Git and network access on first
configuration (Catch2 is fetched at a fixed commit). No ROMs or plugin SDK needed.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

Use Release instead of Debug to verify optimized builds. Disable tests with
`-DBUILD_TESTING=OFF` for a dependency-free static core library.

This builds scanner memory and conservative filename/size hints. It does not
execute Z80 instructions, validate a disk, boot firmware or generate audio.
Synthetic test data are constructed in memory; no factory data are required.
Windows/macOS/Linux CI runs both configurations. A configured workflow is not
proof of success; see its actual run before claiming platform support.
