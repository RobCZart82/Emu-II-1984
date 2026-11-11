// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace emu {

class MainMemory {
public:
    enum class Bus { cpu, dma };

    void load_rom(std::span<const std::uint8_t> bytes);
    void reset_paging() noexcept;
    void write_page_port(std::uint16_t port, std::uint8_t value) noexcept;
    [[nodiscard]] std::uint32_t physical_address(Bus bus, std::uint16_t address) const noexcept;
    [[nodiscard]] std::uint8_t read(Bus bus, std::uint16_t address) const noexcept;
    [[nodiscard]] bool is_mapped(Bus bus, std::uint16_t address) const noexcept;
    void write(Bus bus, std::uint16_t address, std::uint8_t value) noexcept;

private:
    static constexpr std::uint32_t ram_base = 0x2000;
    std::array<std::uint8_t, 8192> rom_{};
    std::array<std::uint8_t, 2048> ram_{};
    // Port order: DMA low/high, then CPU low/high.
    std::array<std::uint8_t, 4> pages_{};
    bool force_page_zero_ = true;
};

}
