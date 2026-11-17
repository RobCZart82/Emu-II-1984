// SPDX-License-Identifier: AGPL-3.0-only
#include "core/devices/lcd_function_programming.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>

TEST_CASE("LCD function programming changes bus width only on completed transfers") {
    emu::LcdFunctionProgramming lcd;
    using Result = emu::LcdFunctionProgramming::Result;
    REQUIRE(lcd.eight_bit());
    REQUIRE_FALSE(lcd.function().has_value());
    REQUIRE(lcd.write(false, 0x3b, 100) == Result::accepted);
    REQUIRE(lcd.function() == 0x3b);
    REQUIRE(lcd.ready_at() == 248);
    REQUIRE(lcd.write(false, 0x2b, 247) == Result::busy);
    REQUIRE(lcd.eight_bit());
    REQUIRE(lcd.function() == 0x3b);
    REQUIRE(lcd.write(false, 0x2b, 248) == Result::accepted);
    REQUIRE_FALSE(lcd.eight_bit());
    REQUIRE(lcd.ready_at() == 396);
    REQUIRE(lcd.write(false, 0x2b, 396) == Result::accepted);
    REQUIRE(lcd.pending_nibble() == 0x20);
    REQUIRE(lcd.function() == 0x2b);
    REQUIRE(lcd.ready_at() == 396);
    REQUIRE(lcd.write(false, 0x8b, 397) == Result::accepted);
    REQUIRE(lcd.function() == 0x28);
    REQUIRE_FALSE(lcd.pending_nibble().has_value());
    REQUIRE(lcd.ready_at() == 545);
    REQUIRE(lcd.write(false, 0x3b, 545) == Result::accepted);
    REQUIRE_FALSE(lcd.eight_bit());
    REQUIRE(lcd.write(false, 0x0b, 546) == Result::accepted);
    REQUIRE(lcd.function() == 0x30);
    REQUIRE(lcd.eight_bit());
}

TEST_CASE("LCD data and other commands remain explicit boundaries without losing partial bytes") {
    emu::LcdFunctionProgramming lcd;
    using Result = emu::LcdFunctionProgramming::Result;
    // SA0 selects data separately from the complete 8-bit bus value.
    REQUIRE(lcd.write(true, 0x3b, 0) == Result::requires_data);
    REQUIRE_FALSE(lcd.function().has_value());
    REQUIRE(lcd.write(false, 0x01, 0) == Result::requires_command);
    REQUIRE(lcd.write(false, 0x2b, 0) == Result::accepted);
    REQUIRE(lcd.write(false, 0x00, 148) == Result::accepted);
    REQUIRE(lcd.pending_nibble().has_value());
    REQUIRE(lcd.pending_nibble() == 0);
    REQUIRE(lcd.write(true, 0x10, 149) == Result::requires_data);
    REQUIRE(lcd.pending_nibble() == 0);
    REQUIRE(lcd.write(false, 0x10, 149) == Result::requires_command);
    REQUIRE(lcd.pending_nibble() == 0);
    REQUIRE(lcd.function() == 0x2b);
    REQUIRE(lcd.ready_at() == 148);
    lcd.reset();
    REQUIRE(lcd.eight_bit());
    REQUIRE_FALSE(lcd.function().has_value());
    REQUIRE_FALSE(lcd.pending_nibble().has_value());
    REQUIRE(lcd.ready_at() == 0);
}

TEST_CASE("LCD time checks preserve state at reverse and overflow boundaries") {
    using Result = emu::LcdFunctionProgramming::Result;
    constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
    emu::LcdFunctionProgramming lcd;
    REQUIRE(lcd.write(false, 0x3b, maximum - 147) == Result::invalid_time);
    REQUIRE_FALSE(lcd.function().has_value());
    REQUIRE(lcd.ready_at() == 0);
    REQUIRE(lcd.write(false, 0x3b, maximum - 148) == Result::accepted);
    REQUIRE(lcd.ready_at() == maximum);
    REQUIRE(lcd.write(false, 0x2b, maximum) == Result::invalid_time);
    REQUIRE(lcd.eight_bit());
    REQUIRE(lcd.function() == 0x3b);
    lcd.reset();
    REQUIRE(lcd.write(false, 0x2b, 0) == Result::accepted);
    REQUIRE(lcd.write(false, 0x2b, 200) == Result::accepted);
    REQUIRE(lcd.write(false, 0x8b, 199) == Result::invalid_time);
    REQUIRE(lcd.pending_nibble() == 0x20);
    REQUIRE(lcd.write(false, 0x8b, maximum) == Result::invalid_time);
    REQUIRE(lcd.pending_nibble() == 0x20);
    REQUIRE(lcd.write(false, 0x8b, 201) == Result::accepted);
    REQUIRE(lcd.function() == 0x28);
    emu::LcdFunctionProgramming other;
    REQUIRE(other.eight_bit());
    REQUIRE_FALSE(other.function().has_value());
    REQUIRE(other.write(false, 0x3b, 0) == Result::accepted);
    REQUIRE(lcd.function() == 0x28);
    REQUIRE(other.write(false, 0x3f, 148) == Result::accepted);
    REQUIRE(other.function() == 0x3f);
}

TEST_CASE("LCD display control retains D C B without changing function programming") {
    using Result = emu::LcdFunctionProgramming::Result;
    for (unsigned bits = 0; bits < 8; ++bits) {
        emu::LcdFunctionProgramming lcd;
        REQUIRE_FALSE(lcd.display_control().has_value());
        REQUIRE(lcd.write(false, 0x38, 0) == Result::accepted);
        REQUIRE(lcd.write(false, static_cast<std::uint8_t>(8 | bits), 148) == Result::accepted);
        REQUIRE(lcd.display_control() == bits);
        REQUIRE(lcd.function() == 0x38);
        REQUIRE(lcd.eight_bit());
        REQUIRE(lcd.ready_at() == 296);
        REQUIRE(lcd.write(false, 8, 295) == Result::busy);
        REQUIRE(lcd.display_control() == bits);
        lcd.reset();
        REQUIRE_FALSE(lcd.display_control().has_value());
        REQUIRE(lcd.write(false, 0x28, 0) == Result::accepted);
        REQUIRE(lcd.write(false, 0, 148) == Result::accepted);
        REQUIRE_FALSE(lcd.display_control().has_value());
        REQUIRE(lcd.write(false, static_cast<std::uint8_t>((8 | bits) << 4), 149) == Result::accepted);
        REQUIRE(lcd.display_control() == bits);
        REQUIRE(lcd.function() == 0x28);
        REQUIRE_FALSE(lcd.eight_bit());
        REQUIRE_FALSE(lcd.pending_nibble().has_value());
    }
}
