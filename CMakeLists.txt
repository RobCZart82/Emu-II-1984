# SPDX-License-Identifier: AGPL-3.0-only
cmake_minimum_required(VERSION 3.24)
project(EmuII1984 VERSION 0.1.0 LANGUAGES CXX)
add_library(emu_core src/core/devices/pio_programming.cpp src/core/devices/ctc_programming.cpp src/core/scheduler.cpp src/core/formats.cpp src/core/main_memory.cpp src/core/scanner_memory.cpp)
target_include_directories(emu_core PUBLIC src)
target_compile_features(emu_core PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(emu_core PRIVATE /W4 /WX)
else()
  target_compile_options(emu_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
include(CTest)
if(BUILD_TESTING)
  find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
  add_test(NAME input_inspector COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/input_inspector_tests.py)
  include(FetchContent)
  FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG fa43b77429ba76c462b1898d6cd2f2d7a9416b14)
  FetchContent_MakeAvailable(Catch2)
  # Populate the header without configuring the upstream examples and tools.
  FetchContent_Declare(KosarevZ80
    GIT_REPOSITORY https://github.com/kosarev/z80.git
    GIT_TAG 51d07188fd4ababfb465848b66ca65cc102b92cd
    SOURCE_SUBDIR emu-header-only)
  FetchContent_MakeAvailable(KosarevZ80)
  add_executable(z80_rom_trace tools/z80_rom_trace.cpp)
  target_include_directories(z80_rom_trace SYSTEM PRIVATE ${kosarevz80_SOURCE_DIR})
  target_link_libraries(z80_rom_trace PRIVATE emu_core)
  add_test(NAME z80_rom_trace COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/rom_trace_tests.py $<TARGET_FILE:z80_rom_trace>)
  add_test(NAME z80_rom_trace_optimized COMMAND ${Python3_EXECUTABLE} -O
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/rom_trace_tests.py $<TARGET_FILE:z80_rom_trace>)
  add_executable(core_tests tests/unit/pio_programming_tests.cpp tests/unit/ctc_programming_tests.cpp tests/unit/scheduler_tests.cpp tests/unit/core_tests.cpp tests/unit/main_memory_tests.cpp tests/unit/z80_probe_tests.cpp)
  target_include_directories(core_tests SYSTEM PRIVATE ${kosarevz80_SOURCE_DIR})
  target_link_libraries(core_tests PRIVATE emu_core Catch2::Catch2WithMain)
  include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
  catch_discover_tests(core_tests)
endif()
