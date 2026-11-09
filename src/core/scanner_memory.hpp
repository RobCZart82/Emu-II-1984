// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstdint>
#include <span>
namespace emu {
// Initial scanner address-space model only. Power-on RAM = zero is a test
// convention, not a verified hardware power-on state.
class ScannerMemory {
public:
    void load_rom(std::span<const std::uint8_t> bytes);
    [[nodiscard]] std::uint8_t read(std::uint16_t address) const noexcept;
    void write(std::uint16_t address, std::uint8_t value) noexcept;
private:
    std::array<std::uint8_t, 8192> rom_{};
    std::array<std::uint8_t, 2048> ram_{};
};
}
