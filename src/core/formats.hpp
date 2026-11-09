// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstdint>
#include <string_view>
namespace emu {
// A filename/length hint only; never authorizes booting or proves validity.
enum class FormatHint { unknown, raw_disk, bank, operating_system, hfe_container };
[[nodiscard]] FormatHint format_hint(std::string_view filename, std::uint64_t bytes);
}
