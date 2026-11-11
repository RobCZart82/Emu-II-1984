// SPDX-License-Identifier: AGPL-3.0-only
#include "core/devices/ctc_programming.hpp"

namespace emu {
CtcProgramming::Result CtcProgramming::write(unsigned index, std::uint8_t value) {
    auto& state = channels_.at(index);
    if (state.expects_constant) {
        state.time_constant = value == 0 ? 256 : value;
        state.expects_constant = false;
        return Result::requires_timing;
    }
    if ((value & 1) == 0) {
        if (index != 0) return Result::unsupported_vector_channel;
        vector_ = static_cast<std::uint8_t>(value & 0xf8);
        return Result::accepted;
    }
    state.last_control = value;
    state.expects_constant = (value & 4) != 0;
    // Continue through reset requests; execution needs the timing model.
    return (value & 2) != 0 ? Result::accepted : Result::requires_timing;
}
void CtcProgramming::reset() noexcept {
    channels_ = {};
    vector_.reset();
}
}
