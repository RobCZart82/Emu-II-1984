// SPDX-License-Identifier: AGPL-3.0-only
#include "core/main_memory.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <stdexcept>

namespace {
using enum emu::MainMemory::Bus;
}

TEST_CASE("Main CPU and DMA select four separate address windows") {
    emu::MainMemory memory;
    memory.write_page_port(0x6868, 0xf1);
    memory.write_page_port(0x69, 2);
    memory.write_page_port(0x6a, 3);
    memory.write_page_port(0x6b, 0xff);

    REQUIRE(memory.physical_address(dma, 0x0000) == 0x08000);
    REQUIRE(memory.physical_address(dma, 0x7fff) == 0x0ffff);
    REQUIRE(memory.physical_address(dma, 0x8000) == 0x10000);
    REQUIRE(memory.physical_address(cpu, 0x7fff) == 0x1ffff);
    REQUIRE(memory.physical_address(cpu, 0x8000) == 0x78000);
    REQUIRE(memory.physical_address(cpu, 0xffff) == 0x7ffff);
}

TEST_CASE("Main paging reset forces page zero until a selector write") {
    emu::MainMemory memory;
    memory.write(cpu, 0xa000, 0x42);
    memory.write_page_port(0x68, 1);
    memory.write_page_port(0x69, 2);
    memory.write_page_port(0x6a, 3);
    memory.write_page_port(0x6b, 4);
    REQUIRE(memory.read(dma, 0x2000) == 0xff);

    memory.reset_paging();
    memory.write_page_port(0x67, 0xff);
    memory.write_page_port(0x6c, 0xff);
    REQUIRE(memory.read(dma, 0x2000) == 0x42);
    REQUIRE(memory.read(dma, 0xa000) == 0x42);
    REQUIRE(memory.read(cpu, 0x2000) == 0x42);
    REQUIRE(memory.read(cpu, 0xa000) == 0x42);

    memory.write_page_port(0x6a, 0);
    REQUIRE(memory.read(cpu, 0x2000) == 0x42);
    REQUIRE(memory.physical_address(cpu, 0x8000) == 0x20000);
    REQUIRE(memory.physical_address(dma, 0x0000) == 0x08000);
    REQUIRE(memory.physical_address(dma, 0x8000) == 0x10000);
}

TEST_CASE("Main backing memory shares RAM and protects ROM across buses") {
    emu::MainMemory memory;
    std::array<std::uint8_t, 8192> rom{};
    rom.front() = 0x31;
    rom.back() = 0xc9;
    memory.load_rom(rom);
    REQUIRE_THROWS_AS(memory.load_rom(std::span(rom).first(8191)), std::invalid_argument);
    REQUIRE_THROWS_AS(memory.load_rom({}), std::invalid_argument);
    memory.write(dma, 0x8000, 0);
    REQUIRE(memory.read(cpu, 0) == 0x31);
    REQUIRE(memory.read(dma, 0x9fff) == 0xc9);

    memory.write(cpu, 0x2000, 0x11);
    memory.write(dma, 0xa7ff, 0x22);
    memory.write(dma, 0xa800, 0x33);
    REQUIRE(memory.read(dma, 0xa000) == 0x11);
    REQUIRE(memory.read(cpu, 0x27ff) == 0x22);
    REQUIRE(memory.read(cpu, 0x2800) == 0xff);

    memory.write_page_port(0x68, 1);
    memory.write(dma, 0x2000, 0x33);
    REQUIRE(memory.read(dma, 0x2000) == 0xff);
    REQUIRE(memory.read(cpu, 0x2000) == 0x11);
}
