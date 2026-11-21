// SPDX-License-Identifier: AGPL-3.0-only
#include "core/devices/lcd_function_programming.hpp"
#include <limits>

namespace emu {
LcdFunctionProgramming::Result LcdFunctionProgramming::write(bool data_register, std::uint8_t bus, std::uint64_t tick) {
    if (last_tick_ && tick < *last_tick_) return Result::invalid_time;
    if (tick < ready_at_) return Result::busy;
    // SA0 -> panel CN3.24 -> LCD CN4.4 (RS); R/W is tied low.
    const bool two_lines = function_ && (*function_ & 8) != 0;
    const auto valid_address = [two_lines](unsigned address) {
        return two_lines ? address <= 0x27 || (address >= 0x40 && address <= 0x67)
                         : address <= 0x4f;
    };
    if (data_register && (!function_ || !valid_address(address_)))
        return Result::requires_data;
    if (high_nibble_ && pending_data_ != data_register)
        return data_register ? Result::requires_data : Result::requires_command;
    if (!eight_bit_ && !high_nibble_) {
        high_nibble_ = static_cast<std::uint8_t>(bus & 0xf0);
        pending_data_ = data_register;
        last_tick_ = tick;
        return Result::accepted;
    }
    const auto command = eight_bit_ ? bus
        : static_cast<std::uint8_t>(*high_nibble_ | (bus >> 4));
    const bool function_set = !data_register && (command & 0xe0) == 0x20;
    const bool display_control = !data_register && (command & 0xf8) == 0x08;
    const bool set_address = !data_register && (command & 0x80) != 0
        && valid_address(command & 0x7f);
    const bool entry_mode = !data_register && (command & 0xfd) == 0x04;
    if (!data_register && !function_set && !display_control && !set_address && !entry_mode)
        return Result::requires_command;
    // Include nominal address-counter update time after a data write.
    const auto duration = data_register ? function_ticks + 16 : function_ticks;
    if (tick > std::numeric_limits<std::uint64_t>::max() - duration)
        return Result::invalid_time;
    if (data_register) {
        ddram_[address_] = command;
        const auto last = two_lines ? 0x67 : 0x4f;
        if (increment_) {
            if (address_ == last) address_ = 0;
            else if (two_lines && address_ == 0x27) address_ = 0x40;
            else ++address_;
        } else {
            if (address_ == 0) address_ = static_cast<std::uint8_t>(last);
            else if (two_lines && address_ == 0x40) address_ = 0x27;
            else --address_;
        }
    } else if (function_set) {
        function_ = command;
        eight_bit_ = (command & 0x10) != 0;
    } else if (display_control) {
        display_control_ = static_cast<std::uint8_t>(command & 7);
    } else if (set_address) {
        address_ = static_cast<std::uint8_t>(command & 0x7f);
    } else {
        increment_ = (command & 2) != 0;
    }
    high_nibble_.reset();
    last_tick_ = tick;
    ready_at_ = tick + duration;
    return Result::accepted;
}
}
