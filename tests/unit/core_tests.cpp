// SPDX-License-Identifier: AGPL-3.0-only
#include "core/formats.hpp"
#include "core/scanner_memory.hpp"
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
TEST_CASE("Scanner ROM mirrors and rejects writes") {
    emu::ScannerMemory memory;
    std::array<std::uint8_t, 8192> rom{};
    rom[0] = 0x31; rom[8191] = 0xc9;
    memory.load_rom(rom);
    for (const auto address : {0x0000, 0x2000, 0x4000, 0x6000}) {
        REQUIRE(memory.read(static_cast<std::uint16_t>(address)) == 0x31);
        REQUIRE(memory.read(static_cast<std::uint16_t>(address + 8191)) == 0xc9);
        memory.write(static_cast<std::uint16_t>(address), 0);
        REQUIRE(memory.read(static_cast<std::uint16_t>(address)) == 0x31);
    }
}
TEST_CASE("Scanner RAM aliases are coherent and instances isolated") {
    emu::ScannerMemory a, b;
    a.write(0x8000, 0x42); a.write(0xffff, 0x7e);
    REQUIRE(a.read(0x8800) == 0x42);
    REQUIRE(a.read(0xf800) == 0x42);
    REQUIRE(a.read(0x87ff) == 0x7e);
    REQUIRE(b.read(0x8000) == 0);
    REQUIRE(a.read(0x8001) == 0);
}
TEST_CASE("Invalid ROM lengths leave existing contents intact") {
    emu::ScannerMemory memory;
    std::array<std::uint8_t, 8192> rom{}; rom[0] = 0x55;
    memory.load_rom(rom);
    REQUIRE_THROWS_AS(memory.load_rom(std::span(rom).first(8191)), std::invalid_argument);
    REQUIRE_THROWS_AS(memory.load_rom({}), std::invalid_argument);
    REQUIRE(memory.read(0) == 0x55);
}
TEST_CASE("Disk bank and OS hints stay distinct") {
    using enum emu::FormatHint;
    REQUIRE(emu::format_hint("test.IMG", 573440) == raw_disk);
    REQUIRE(emu::format_hint("test.emuiifd", 573440) == raw_disk);
    REQUIRE(emu::format_hint("test.eii", 485887) == bank);
    REQUIRE(emu::format_hint("test.E2O", 72704) == operating_system);
    REQUIRE(emu::format_hint("test.hfe", 512) == hfe_container);
}
TEST_CASE("Wrong size or extension never implies a supported image") {
    using enum emu::FormatHint;
    for (const auto size : {0, 573439, 573441})
        REQUIRE(emu::format_hint("bad.img", size) == unknown);
    REQUIRE(emu::format_hint("bank.eii", 573440) == unknown);
    REQUIRE(emu::format_hint("no-extension", 573440) == unknown);
    REQUIRE(emu::format_hint("folder.img/file", 573440) == unknown);
    REQUIRE(emu::format_hint("bad.hfe", 511) == unknown);
}

TEST_CASE("All scanner addresses preserve ROM and RAM alias boundaries") {
    emu::ScannerMemory memory;
    std::array<std::uint8_t, 8192> rom{};
    for (std::size_t i = 0; i < rom.size(); ++i) rom[i] = static_cast<std::uint8_t>((i * 17 + i / 256) & 255);
    memory.load_rom(rom);
    for (unsigned i = 0; i < 2048; ++i)
        memory.write(static_cast<std::uint16_t>(0x8000 + i), static_cast<std::uint8_t>((i * 7 + i / 256) & 255));
    for (unsigned address = 0; address <= 0xffff; ++address) {
        const auto expected = address < 0x8000 ? rom[address % 8192]
            : static_cast<std::uint8_t>(((address % 2048) * 7 + (address % 2048) / 256) & 255);
        REQUIRE(memory.read(static_cast<std::uint16_t>(address)) == expected);
    }
    std::array<std::uint8_t, 8193> oversized{};
    REQUIRE_THROWS_AS(memory.load_rom(oversized), std::invalid_argument);
    REQUIRE(memory.read(8191) == rom.back());
}
