// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <optional>

namespace emu {
// IC47 Q0-Q7, in order. Names are wire labels, not implemented peripherals.
enum class ScannerStrobe { wl0, wl1, adc_read, adc_start, scan_read, wl2, metronome, bvd };

struct ScannerIoSelection {
    bool pio = false;
    bool ctc = false;
    std::optional<ScannerStrobe> strobe;
    unsigned register_index = 0;

    [[nodiscard]] constexpr bool overlaps() const noexcept {
        return (pio && ctc) || (strobe.has_value() && (pio || ctc));
    }
};

// Valid ordinary I/O read/write cycle only (/IORQ and /RD or /WR asserted).
// Excludes interrupt acknowledge, idle and memory cycles; models no timing.
[[nodiscard]] constexpr ScannerIoSelection decode_scanner_io(std::uint16_t port) noexcept {
    ScannerIoSelection selected;
    selected.pio = (port & 0x20) == 0; // IC46 /CE <- SA5
    selected.ctc = (port & 0x10) == 0; // IC45 /CE <- SA4
    selected.register_index = port & 3u;
    if ((port & 0x40) == 0) { // IC47 /G2A <- SA6
        selected.strobe = static_cast<ScannerStrobe>((port >> 1) & 7u);
    }
    return selected;
}
}
