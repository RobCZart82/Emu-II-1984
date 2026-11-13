// SPDX-License-Identifier: AGPL-3.0-only
#include "core/devices/pio_programming.hpp"

namespace emu {
PioProgramming::Result PioProgramming::write_control(unsigned index, std::uint8_t value) {
    auto& state = ports_.at(index);
    if (state.expects_direction) {
        state.direction = value;
        state.expects_direction = false;
        return Result::requires_io;
    }
    if (state.expects_mask) {
        state.mask = value;
        state.expects_mask = false;
    } else if ((value & 1) == 0) {
        state.vector = value;
    } else {
        switch (value & 0x0f) {
        case 0x0f:
            state.mode = static_cast<std::uint8_t>(value >> 6);
            state.direction.reset();
            state.expects_direction = *state.mode == 3;
            // Pin drive and handshake remain outside the programming front end.
            return Result::requires_io;
        case 0x07:
            state.interrupt_control = value;
            state.expects_mask = (value & 0x10) != 0;
            break;
        case 0x03:
            state.interrupt_control = static_cast<std::uint8_t>(
                (state.interrupt_control & 0x7f) | (value & 0x80));
            break;
        default:
            return Result::unsupported_control;
        }
    }
    // Conservative boundary even when mask programming delays actual enable.
    return (state.interrupt_control & 0x80) != 0
        ? Result::requires_interrupts : Result::accepted;
}
void PioProgramming::reset() noexcept {
    for (auto& state : ports_) {
        const auto vector = state.vector;
        state = {};
        state.vector = vector;
    }
}
}
