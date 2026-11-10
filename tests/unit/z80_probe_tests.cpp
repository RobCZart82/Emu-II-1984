// SPDX-License-Identifier: AGPL-3.0-only
#include "core/scanner_memory.hpp"

#include <catch2/catch_test_macros.hpp>
#include <z80.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace {

static_assert(z80::retry_input > 0xff, "Port data must not collide with the retry sentinel");

enum class AccessKind { write, input, output };

struct Access {
    AccessKind kind;
    std::uint16_t address;
    std::uint8_t value;
    std::uint64_t tick;

    bool operator==(const Access&) const = default;
};

// A synthetic main-side bus for the CPU probe. Main-memory paging belongs to
// a separate hardware model.
struct FlatMemory {
    std::array<std::uint8_t, 65536> bytes{};

    std::uint8_t read(std::uint16_t address) const { return bytes[address]; }
    void write(std::uint16_t address, std::uint8_t value) { bytes[address] = value; }
};

template<class Memory>
class CpuProbe : public z80::z80_cpu<CpuProbe<Memory>> {
public:
    CpuProbe(Memory& memory, std::uint8_t port_value)
        : memory_(memory), port_value_(port_value) {}

    z80::fast_u8 on_read(z80::fast_u16 address) {
        return memory_.read(static_cast<std::uint16_t>(address));
    }

    void on_write(z80::fast_u16 address, z80::fast_u8 value) {
        const auto addr = static_cast<std::uint16_t>(address);
        const auto byte = static_cast<std::uint8_t>(value);
        accesses.push_back({AccessKind::write, addr, byte, ticks});
        memory_.write(addr, byte);
    }

    z80::fast_u8 on_input(z80::fast_u16 port) {
        accesses.push_back({AccessKind::input, static_cast<std::uint16_t>(port),
                            port_value_, ticks});
        return port_value_;
    }

    void on_output(z80::fast_u16 port, z80::fast_u8 value) {
        accesses.push_back({AccessKind::output, static_cast<std::uint16_t>(port),
                            static_cast<std::uint8_t>(value), ticks});
    }

    void on_tick(unsigned elapsed) { ticks += elapsed; }

    std::uint64_t ticks = 0;
    std::vector<Access> accesses;

private:
    Memory& memory_;
    std::uint8_t port_value_;
};

std::array<std::uint8_t, 8192> program(std::uint8_t value) {
    // LD A,value; LD (8000h),A; IN A,(FEh); OUT (FDh),A
    return {0x3e, value, 0x32, 0x00, 0x80, 0xdb, 0xfe, 0xd3, 0xfd};
}

} // namespace

TEST_CASE("Two interleaved Z80s keep separate CPU and bus state") {
    FlatMemory main_memory;
    const auto main_program = program(0x42);
    std::copy(main_program.begin(), main_program.end(), main_memory.bytes.begin());

    emu::ScannerMemory scanner_memory;
    scanner_memory.load_rom(program(0x17));

    CpuProbe main_cpu(main_memory, 0xff);
    CpuProbe scanner_cpu(scanner_memory, 0x5a);

    REQUIRE(main_cpu.on_step() == 0);
    REQUIRE(main_cpu.ticks == 7);
    REQUIRE(main_cpu.get_a() == 0x42);
    REQUIRE(scanner_cpu.ticks == 0);
    REQUIRE(scanner_cpu.get_pc() == 0);

    REQUIRE(scanner_cpu.on_step() == 0);
    REQUIRE(scanner_cpu.on_step() == 0);
    REQUIRE(scanner_cpu.ticks == 20);
    REQUIRE(scanner_memory.read(0x8800) == 0x17);
    REQUIRE(main_memory.read(0x8000) == 0);

    REQUIRE(main_cpu.on_step() == 0);
    REQUIRE(main_cpu.ticks == 20);
    REQUIRE(main_memory.read(0x8000) == 0x42);

    REQUIRE(main_cpu.on_step() == 0);
    REQUIRE(scanner_cpu.on_step() == 0);
    REQUIRE(main_cpu.ticks == 31);
    REQUIRE(scanner_cpu.ticks == 31);
    REQUIRE(main_cpu.get_a() == 0xff);
    REQUIRE(scanner_cpu.get_a() == 0x5a);

    REQUIRE(scanner_cpu.on_step() == 0);
    REQUIRE(main_cpu.on_step() == 0);
    REQUIRE(main_cpu.ticks == 42);
    REQUIRE(scanner_cpu.ticks == 42);
    REQUIRE(main_cpu.get_pc() == 9);
    REQUIRE(scanner_cpu.get_pc() == 9);

    const std::vector<Access> main_accesses{
        {AccessKind::write, 0x8000, 0x42, 19},
        {AccessKind::input, 0x42fe, 0xff, 30},
        {AccessKind::output, 0xfffd, 0xff, 41},
    };
    const std::vector<Access> scanner_accesses{
        {AccessKind::write, 0x8000, 0x17, 19},
        {AccessKind::input, 0x17fe, 0x5a, 30},
        {AccessKind::output, 0x5afd, 0x5a, 41},
    };
    REQUIRE(main_cpu.accesses == main_accesses);
    REQUIRE(scanner_cpu.accesses == scanner_accesses);
}

TEST_CASE("Resetting one Z80 does not reset its bus or the other Z80") {
    emu::ScannerMemory first_memory;
    first_memory.load_rom(program(0x42));
    emu::ScannerMemory second_memory;
    second_memory.load_rom(program(0x17));
    CpuProbe first(first_memory, 0xff);
    CpuProbe second(second_memory, 0xff);

    REQUIRE(first.on_step() == 0);
    REQUIRE(first.on_step() == 0);
    REQUIRE(second.on_step() == 0);
    REQUIRE(second.on_step() == 0);

    first.on_reset();

    REQUIRE(first.get_pc() == 0);
    REQUIRE(first_memory.read(0x8000) == 0x42);
    REQUIRE(second.get_pc() == 5);
    REQUIRE(second.get_a() == 0x17);
    REQUIRE(second.ticks == 20);
    REQUIRE(second_memory.read(0x8000) == 0x17);
}
