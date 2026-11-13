// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace emu {
// Early control programming only. Stop at any result other than accepted.
class PioProgramming {
public:
    enum class Result { accepted, requires_io, requires_interrupts, unsupported_control };
    struct Port {
        std::optional<std::uint8_t> vector;
        std::optional<std::uint8_t> mode;
        std::optional<std::uint8_t> direction;
        bool expects_direction = false;
        std::uint8_t interrupt_control = 0;
        std::uint8_t mask = 0xff;
        bool expects_mask = false;
    };
    [[nodiscard]] Result write_control(unsigned index, std::uint8_t value);
    [[nodiscard]] const Port& port(unsigned index) const { return ports_.at(index); }
    // Hardware reset preserves previously programmed vectors.
    void reset() noexcept;
private:
    std::array<Port, 2> ports_{};
};
}
