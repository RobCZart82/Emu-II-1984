// SPDX-License-Identifier: AGPL-3.0-only
#include "core/devices/lcd_function_programming.hpp"
#include <limits>

namespace emu {
LcdFunctionProgramming::Result LcdFunctionProgramming::write(bool data_register, std::uint8_t bus, std::uint64_t tick) {
    if (last_tick_ && tick < *last_tick_) return Result::invalid_time;
    if (tick < ready_at_) return Result::busy;
    // SA0 -> panel CN3.24 -> LCD CN4.4 (RS); R/W is tied low.
    if (data_register) return Result::requires_data;
    if (!eight_bit_ && !high_nibble_) {
        high_nibble_ = static_cast<std::uint8_t>(bus & 0xf0);
        last_tick_ = tick;
        return Result::accepted;
    }
    const auto command = eight_bit_ ? bus
        : static_cast<std::uint8_t>(*high_nibble_ | (bus >> 4));
    if ((command & 0xe0) != 0x20) return Result::requires_command;
    if (tick > std::numeric_limits<std::uint64_t>::max() - function_ticks)
        return Result::invalid_time;
    function_ = command;
    eight_bit_ = (command & 0x10) != 0;
    high_nibble_.reset();
    last_tick_ = tick;
    ready_at_ = tick + function_ticks;
    return Result::accepted;
}
}
