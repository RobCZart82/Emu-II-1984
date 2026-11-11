// SPDX-License-Identifier: AGPL-3.0-only
#include "core/devices/ctc_programming.hpp"
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

TEST_CASE("CTC vector uses channel zero and preserves five vector bits") {
    emu::CtcProgramming ctc;
    using Result = emu::CtcProgramming::Result;
    REQUIRE_FALSE(ctc.vector().has_value());
    REQUIRE(ctc.write(0, 0xf6) == Result::accepted);
    REQUIRE(ctc.vector() == 0xf0);
    REQUIRE(ctc.write(1, 0xe0) == Result::unsupported_vector_channel);
    REQUIRE(ctc.vector() == 0xf0);
    REQUIRE(ctc.channel(1).last_control == 3);
}
TEST_CASE("CTC constant takes precedence over vector and control decoding") {
    using Result = emu::CtcProgramming::Result;
    for (const auto value : {0, 1, 0xf0, 0xff}) {
        emu::CtcProgramming ctc;
        REQUIRE(ctc.write(0, 0x07) == Result::accepted);
        REQUIRE(ctc.channel(0).expects_constant);
        REQUIRE(ctc.write(0, static_cast<std::uint8_t>(value)) == Result::requires_timing);
        REQUIRE(ctc.channel(0).time_constant == (value == 0 ? 256 : value));
        REQUIRE_FALSE(ctc.channel(0).expects_constant);
        REQUIRE_FALSE(ctc.vector().has_value());
        REQUIRE(ctc.channel(0).last_control == 0x07);
    }
}
TEST_CASE("CTC programming isolates channels and reset clears research state") {
    emu::CtcProgramming ctc;
    using Result = emu::CtcProgramming::Result;
    REQUIRE(ctc.write(0, 0xd0) == Result::accepted);
    for (unsigned channel = 0; channel < 4; ++channel) {
        REQUIRE(ctc.write(channel, 0x07) == Result::accepted);
    }
    for (unsigned channel = 0; channel < 4; ++channel) {
        REQUIRE(ctc.write(channel, static_cast<std::uint8_t>(channel + 1)) == Result::requires_timing);
        REQUIRE(ctc.channel(channel).time_constant == channel + 1);
    }
    ctc.reset();
    REQUIRE_FALSE(ctc.vector().has_value());
    for (unsigned channel = 0; channel < 4; ++channel) {
        REQUIRE(ctc.channel(channel).last_control == 3);
        REQUIRE(ctc.channel(channel).time_constant == 0);
        REQUIRE_FALSE(ctc.channel(channel).expects_constant);
    }
    REQUIRE(ctc.write(0, 0x01) == Result::requires_timing);
    REQUIRE_THROWS_AS(ctc.write(4, 0x03), std::out_of_range);
    REQUIRE_THROWS_AS(ctc.channel(4), std::out_of_range);
}
