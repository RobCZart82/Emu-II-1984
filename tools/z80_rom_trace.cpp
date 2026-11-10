// SPDX-License-Identifier: AGPL-3.0-only
#include "core/main_memory.hpp"
#include "core/scanner_memory.hpp"
#include <z80.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <type_traits>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {
struct UnmodelledPort {
    const char* direction;
    std::uint16_t address;
};

template<class Memory>
class TraceCpu : public z80::z80_cpu<TraceCpu<Memory>> {
public:
    explicit TraceCpu(Memory& memory) : memory_(memory) {}

    z80::fast_u8 on_read(z80::fast_u16 address) {
        const auto addr = static_cast<std::uint16_t>(address);
        if constexpr (std::is_same_v<Memory, emu::MainMemory>)
            return memory_.read(emu::MainMemory::Bus::cpu, addr);
        else
            return memory_.read(addr);
    }

    void on_write(z80::fast_u16 address, z80::fast_u8 value) {
        const auto addr = static_cast<std::uint16_t>(address);
        const auto byte = static_cast<std::uint8_t>(value);
        if constexpr (std::is_same_v<Memory, emu::MainMemory>)
            memory_.write(emu::MainMemory::Bus::cpu, addr, byte);
        else
            memory_.write(addr, byte);
    }

    z80::fast_u8 on_input(z80::fast_u16 port) {
        throw UnmodelledPort{"in", static_cast<std::uint16_t>(port)};
    }

    void on_output(z80::fast_u16 port, z80::fast_u8 value) {
        if constexpr (std::is_same_v<Memory, emu::MainMemory>) {
            const auto low_port = port & 0xffu;
            if (low_port >= 0x68 && low_port <= 0x6b) {
                memory_.write_page_port(static_cast<std::uint16_t>(port),
                                        static_cast<std::uint8_t>(value));
                return;
            }
        }
        throw UnmodelledPort{"out", static_cast<std::uint16_t>(port)};
    }

    void on_tick(unsigned elapsed) { ticks += elapsed; }
    std::uint64_t ticks = 0;

private:
    Memory& memory_;
};

template<class Memory>
void trace(const char* name, Memory& memory) {
    TraceCpu<Memory> cpu(memory);
    constexpr unsigned max_steps = 256;
    for (unsigned step = 1; step <= max_steps; ++step) {
        const auto pc = cpu.get_pc();
        try {
            cpu.on_step();
        } catch (const UnmodelledPort& port) {
            std::cout << name << ',' << step << ',' << pc << ",," << cpu.ticks
                      << ",unmodelled_" << port.direction << ',' << port.address << '\n';
            return;
        }
        const bool halted = cpu.on_is_halted();
        const char* event = halted ? "halt" : step == max_steps ? "step_limit" : "step";
        std::cout << name << ',' << step << ',' << pc << ',' << cpu.get_pc()
                  << ',' << cpu.ticks << ',' << event << ",\n";
        if (halted) return;
    }
}
} // namespace

int main() {
#ifdef _WIN32
    if (_setmode(_fileno(stdin), _O_BINARY) == -1) return 1;
#endif
    std::array<std::uint8_t, 8192> main_bytes{}, scanner_bytes{};
    if (!std::cin.read(reinterpret_cast<char*>(main_bytes.data()), main_bytes.size()) ||
        !std::cin.read(reinterpret_cast<char*>(scanner_bytes.data()), scanner_bytes.size()) ||
        std::cin.get() != std::char_traits<char>::eof() || std::cin.bad()) {
        std::cerr << "Expected exactly two 8192-byte ROM images on stdin\n";
        return 1;
    }
    emu::MainMemory main_memory;
    emu::ScannerMemory scanner_memory;
    main_memory.load_rom(main_bytes);
    scanner_memory.load_rom(scanner_bytes);
    std::cout << "cpu,step,pc_before,pc_after,t_states,event,port\n";
    trace("main", main_memory);
    trace("scanner", scanner_memory);
    return std::cout ? 0 : 1;
}
