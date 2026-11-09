// SPDX-License-Identifier: AGPL-3.0-only
#include "core/formats.hpp"
#include <string>
namespace emu {
FormatHint format_hint(std::string_view filename, std::uint64_t bytes) {
    const auto dot = filename.find_last_of('.');
    if (dot == std::string_view::npos) return FormatHint::unknown;
    std::string ext(filename.substr(dot));
    for (auto& c : ext) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
    if ((ext == ".img" || ext == ".emuiifd") && bytes == 573440) return FormatHint::raw_disk;
    if (ext == ".eii" && bytes == 485887) return FormatHint::bank;
    if (ext == ".e2o" && bytes == 72704) return FormatHint::operating_system;
    // HFE lengths vary. Header, track table and encoding validation are M2 work.
    if (ext == ".hfe" && bytes >= 512) return FormatHint::hfe_container;
    return FormatHint::unknown;
}
}
