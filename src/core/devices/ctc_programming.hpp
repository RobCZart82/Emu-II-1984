// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace emu {
// Programming front end. requires_timing marks the boundary of this model.
class CtcProgramming {
public:
    enum class Result { accepted, requires_timing, unsupported_vector_channel };
    struct Channel {
        std::uint8_t last_control = 3; // Deterministic reset convention for this probe.
        std::uint16_t time_constant = 0; // Zero denotes an unprogrammed latch.
        bool expects_constant = false;
    };
    [[nodiscard]] Result write(unsigned channel, std::uint8_t value);
    [[nodiscard]] const Channel& channel(unsigned index) const { return channels_.at(index); }
    [[nodiscard]] std::optional<std::uint8_t> vector() const noexcept { return vector_; }
    void reset() noexcept;
private:
    std::array<Channel, 4> channels_{};
    std::optional<std::uint8_t> vector_;
};
}
