// SPDX-License-Identifier: AGPL-3.0-only
#include "core/devices/pio_programming.hpp"
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

TEST_CASE("PIO vectors are independent and survive hardware reset") {
    emu::PioProgramming pio;
    using Result = emu::PioProgramming::Result;
    REQUIRE_FALSE(pio.port(0).vector.has_value());
    REQUIRE_FALSE(pio.port(1).vector.has_value());
    REQUIRE(pio.write_control(0, 0xf8) == Result::accepted);
    REQUIRE(pio.write_control(1, 0xfa) == Result::accepted);
    REQUIRE(pio.port(0).vector == 0xf8);
    REQUIRE(pio.port(1).vector == 0xfa);
    REQUIRE(pio.write_control(0, 0x97) == Result::requires_interrupts);
    pio.reset();
    REQUIRE(pio.port(0).vector == 0xf8);
    REQUIRE(pio.port(1).vector == 0xfa);
    for (unsigned index = 0; index < 2; ++index) {
        REQUIRE(pio.port(index).interrupt_control == 0);
        REQUIRE(pio.port(index).mask == 0xff);
        REQUIRE_FALSE(pio.port(index).expects_mask);
    }
    REQUIRE(pio.write_control(0, 0xe2) == Result::accepted);
    REQUIRE(pio.port(0).vector == 0xe2);
    REQUIRE_THROWS_AS(pio.write_control(2, 0), std::out_of_range);
    REQUIRE_THROWS_AS(pio.port(2), std::out_of_range);
}

TEST_CASE("PIO pending masks take precedence over all command encodings") {
    using Result = emu::PioProgramming::Result;
    // Even vector, mode command, interrupt command and enable command shapes.
    for (const auto mask : {0, 0xf8, 0xcf, 0x87, 0x83, 0xff}) {
        emu::PioProgramming pio;
        REQUIRE(pio.write_control(0, 0xe0) == Result::accepted);
        REQUIRE(pio.write_control(0, 0x77) == Result::accepted);
        REQUIRE(pio.port(0).expects_mask);
        REQUIRE(pio.write_control(1, 0xfa) == Result::accepted);
        REQUIRE(pio.port(0).expects_mask);
        REQUIRE(pio.write_control(0, static_cast<std::uint8_t>(mask)) == Result::accepted);
        REQUIRE(pio.port(0).mask == mask);
        REQUIRE_FALSE(pio.port(0).expects_mask);
        REQUIRE(pio.port(0).vector == 0xe0);
        REQUIRE(pio.port(1).vector == 0xfa);
        REQUIRE(pio.port(1).mask == 0xff);
        REQUIRE(pio.write_control(0, 0xe2) == Result::accepted);
        REQUIRE(pio.port(0).vector == 0xe2);
    }
}

TEST_CASE("PIO interrupt enable changes preserve control conditions") {
    emu::PioProgramming pio;
    using Result = emu::PioProgramming::Result;
    REQUIRE(pio.write_control(0, 0x67) == Result::accepted);
    REQUIRE_FALSE(pio.port(0).expects_mask);
    REQUIRE(pio.write_control(0, 0x83) == Result::requires_interrupts);
    REQUIRE(pio.port(0).interrupt_control == 0xe7);
    // Bits 6-4 are ignored by an enable-only command.
    REQUIRE(pio.write_control(0, 0x73) == Result::accepted);
    REQUIRE(pio.port(0).interrupt_control == 0x67);
    REQUIRE(pio.write_control(1, 0x97) == Result::requires_interrupts);
    REQUIRE(pio.port(1).expects_mask);
    // The frontend reports the boundary even before the requested mask arrives.
    REQUIRE(pio.write_control(1, 0xff) == Result::requires_interrupts);
    REQUIRE_FALSE(pio.port(1).expects_mask);
}

TEST_CASE("PIO modes and unknown controls cannot silently continue a trace") {
    using Result = emu::PioProgramming::Result;
    for (unsigned index = 0; index < 2; ++index) {
        for (const auto mode : {0x0f, 0x4f, 0x8f, 0xcf, 0xff}) {
            emu::PioProgramming pio;
            REQUIRE(pio.write_control(index, static_cast<std::uint8_t>(mode)) == Result::requires_io);
            REQUIRE_FALSE(pio.port(index).expects_mask);
        }
        for (const auto control : {0x01, 0x05, 0x09, 0x0b, 0x0d}) {
            emu::PioProgramming pio;
            REQUIRE(pio.write_control(index, static_cast<std::uint8_t>(control)) == Result::unsupported_control);
            REQUIRE_FALSE(pio.port(index).vector.has_value());
            REQUIRE(pio.port(index).interrupt_control == 0);
        }
    }
}
