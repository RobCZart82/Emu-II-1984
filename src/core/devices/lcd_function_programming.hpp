// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <optional>

namespace emu {
// HD44780-compatible research front end for completed scanner BVD writes.
// Starts ready in 8-bit mode; power-on supply/reset sequencing is not modeled.
class LcdFunctionProgramming {
public:
    enum class Result { accepted, requires_command, requires_data, busy, invalid_time };
    // Nominal 37 us at the scanner's 4 MHz clock, not a measured LCD oscillator.
    static constexpr std::uint64_t function_ticks = 148;
    [[nodiscard]] Result write(bool data_register, std::uint8_t bus, std::uint64_t tick);
    [[nodiscard]] bool eight_bit() const noexcept { return eight_bit_; }
    [[nodiscard]] std::optional<std::uint8_t> function() const noexcept { return function_; }
    [[nodiscard]] std::optional<std::uint8_t> pending_nibble() const noexcept { return high_nibble_; }
    [[nodiscard]] std::uint64_t ready_at() const noexcept { return ready_at_; }
    void reset() noexcept { *this = LcdFunctionProgramming{}; }
private:
    bool eight_bit_ = true;
    std::optional<std::uint8_t> function_;
    std::optional<std::uint8_t> high_nibble_;
    std::optional<std::uint64_t> last_tick_;
    std::uint64_t ready_at_ = 0;
};
}
