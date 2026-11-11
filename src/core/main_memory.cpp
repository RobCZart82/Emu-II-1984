// SPDX-License-Identifier: AGPL-3.0-only
#include "core/main_memory.hpp"

#include <algorithm>
#include <stdexcept>

namespace emu {

void MainMemory::load_rom(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != rom_.size()) {
        throw std::invalid_argument("main ROM must be 8192 bytes");
    }
    std::copy(bytes.begin(), bytes.end(), rom_.begin());
}

void MainMemory::reset_paging() noexcept {
    force_page_zero_ = true;
}

void MainMemory::write_page_port(std::uint16_t port, std::uint8_t value) noexcept {
    const auto low_port = port & 0xffu;
    if (low_port < 0x68 || low_port > 0x6b) {
        return;
    }
    pages_[low_port - 0x68] = static_cast<std::uint8_t>(value & 0x0f);
    force_page_zero_ = false;
}

std::uint32_t MainMemory::physical_address(Bus bus, std::uint16_t address) const noexcept {
    const auto pair = bus == Bus::cpu ? 2u : 0u;
    const auto page = force_page_zero_ ? 0u : pages_[pair + (address >> 15)];
    return (static_cast<std::uint32_t>(page) << 15) | (address & 0x7fffu);
}

std::uint8_t MainMemory::read(Bus bus, std::uint16_t address) const noexcept {
    const auto physical = physical_address(bus, address);
    if (physical < rom_.size()) {
        return rom_[physical];
    }
    if (physical >= ram_base && physical < ram_base + ram_.size()) {
        return ram_[physical - ram_base];
    }
    return 0xff; // Unmapped read convention for the research model.
}

bool MainMemory::is_mapped(Bus bus, std::uint16_t address) const noexcept {
    return physical_address(bus, address) < ram_base + ram_.size();
}

void MainMemory::write(Bus bus, std::uint16_t address, std::uint8_t value) noexcept {
    const auto physical = physical_address(bus, address);
    if (physical >= ram_base && physical < ram_base + ram_.size()) {
        ram_[physical - ram_base] = value;
    }
}

}
