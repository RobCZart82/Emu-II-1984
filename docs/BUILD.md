# Build the research core

Requirements: CMake 3.24+, a C++20 compiler, Git and network access on first
configuration (Catch2 and the kosarev/z80 header are fetched at fixed commits).
No ROMs or plugin SDK needed.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

Use Release instead of Debug to verify optimized builds. Disable tests with
`-DBUILD_TESTING=OFF` for a dependency-free static core library.

The static core builds scanner memory and conservative filename/size hints.
The test target also executes synthetic Z80 instructions on two independent
buses. Disk validation, firmware boot and audio remain future milestones.
Windows/macOS/Linux CI runs both configurations. A configured workflow is not
proof of success; see its actual run before claiming platform support.
