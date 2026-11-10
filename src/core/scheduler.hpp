// SPDX-License-Identifier: AGPL-3.0-only
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <queue>
#include <vector>

namespace emu {
using Tick = std::uint64_t;
// Project ordering policy, not yet a measured Emulator II timing model.
enum class Phase : std::uint8_t { reset, bus, device, main_cpu, scanner_cpu };
struct Event {
    Tick tick;
    Phase phase;
    std::uint32_t tag;
    std::uint64_t sequence;
    bool operator==(const Event&) const = default;
};

// Offline research scheduler. Queue/handler allocation is not audio-thread safe.
class Scheduler {
public:
    using Handler = std::function<void(const Event&)>;
    Scheduler() = default;
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = delete;
    Scheduler& operator=(Scheduler&&) = delete;
    void schedule(Tick tick, Phase phase, std::uint32_t tag);
    // Inclusive deadline. Handlers may enqueue events, but may not recurse.
    // A throwing handler consumes its event; remaining events stay queued.
    void run_until(Tick deadline, const Handler& handler);
    [[nodiscard]] Tick now() const noexcept { return now_; }
    [[nodiscard]] std::size_t pending() const noexcept { return events_.size(); }
private:
    struct Later {
        bool operator()(const Event& a, const Event& b) const noexcept;
    };
    std::priority_queue<Event, std::vector<Event>, Later> events_;
    Tick now_ = 0;
    std::uint64_t next_sequence_ = 0;
    std::optional<Phase> phase_floor_;
    bool running_ = false;
};
}
