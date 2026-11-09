// SPDX-License-Identifier: AGPL-3.0-only
#include "core/scanner_memory.hpp"
#include <algorithm>
#include <stdexcept>
namespace emu {
void ScannerMemory::load_rom(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != rom_.size()) throw std::invalid_argument("scanner ROM must be 8192 bytes");
    std::copy(bytes.begin(), bytes.end(), rom_.begin());
}
std::uint8_t ScannerMemory::read(std::uint16_t address) const noexcept {
    if (address < 0x8000) return rom_[address % rom_.size()];
    return ram_[address % ram_.size()];
}
void ScannerMemory::write(std::uint16_t address, std::uint8_t value) noexcept {
    if (address >= 0x8000) ram_[address % ram_.size()] = value;
}
}
