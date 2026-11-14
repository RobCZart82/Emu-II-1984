// SPDX-License-Identifier: AGPL-3.0-only
#include "core/scanner_io.hpp"
#include <catch2/catch_test_macros.hpp>
#include <array>

TEST_CASE("Scanner I/O selections follow the schematic across all 16-bit ports") {
    // Truth table for SA6..SA4, independent of the per-bit implementation.
    constexpr std::array<bool, 8> pio{true, true, false, false, true, true, false, false};
    constexpr std::array<bool, 8> ctc{true, false, true, false, true, false, true, false};
    constexpr std::array<bool, 8> strobe{true, true, true, true, false, false, false, false};
    constexpr std::array<bool, 8> overlap{true, true, true, false, true, false, false, false};
    for (unsigned port = 0; port < 65536; ++port) {
        const auto selection = emu::decode_scanner_io(static_cast<std::uint16_t>(port));
        const auto region = (port / 16) % 8;
        REQUIRE(selection.pio == pio[region]);
        REQUIRE(selection.ctc == ctc[region]);
        REQUIRE(selection.strobe.has_value() == strobe[region]);
        REQUIRE(selection.overlaps() == overlap[region]);
        REQUIRE(selection.register_index == port % 4);
        if (selection.strobe) {
            REQUIRE(static_cast<unsigned>(*selection.strobe) == (port % 16) / 2);
        }
    }
}

TEST_CASE("Scanner ordinary device windows and BE strobe remain distinct") {
    constexpr std::array<unsigned, 8> mirrors{0, 4, 8, 12, 0x80, 0x84, 0x88, 0x8c};
    for (unsigned index = 0; index < 4; ++index) {
        for (const auto mirror : mirrors) {
            const auto pio = emu::decode_scanner_io(static_cast<std::uint16_t>(0x50 | index | mirror));
            const auto ctc = emu::decode_scanner_io(static_cast<std::uint16_t>(0x60 | index | mirror));
            REQUIRE(pio.pio);
            REQUIRE_FALSE(pio.ctc);
            REQUIRE_FALSE(pio.strobe.has_value());
            REQUIRE(pio.register_index == index);
            REQUIRE(ctc.ctc);
            REQUIRE_FALSE(ctc.pio);
            REQUIRE_FALSE(ctc.strobe.has_value());
            REQUIRE(ctc.register_index == index);
        }
    }
    for (const auto port : {0x003e, 0x003f, 0x00be, 0x00bf, 0x3bbe, 0xffbf}) {
        const auto selected = emu::decode_scanner_io(static_cast<std::uint16_t>(port));
        REQUIRE(selected.strobe == emu::ScannerStrobe::bvd);
        REQUIRE_FALSE(selected.pio);
        REQUIRE_FALSE(selected.ctc);
        REQUIRE_FALSE(selected.overlaps());
    }
    REQUIRE_FALSE(emu::decode_scanner_io(0xfffe).strobe.has_value());
}
