// SPDX-License-Identifier: AGPL-3.0-only
#include "core/scheduler.hpp"
#include <limits>
#include <stdexcept>
#include <tuple>
namespace emu {
bool Scheduler::Later::operator()(const Event& a, const Event& b) const noexcept {
    return std::tie(a.tick, a.phase, a.sequence) > std::tie(b.tick, b.phase, b.sequence);
}
void Scheduler::schedule(Tick tick, Phase phase, std::uint32_t tag) {
    if (phase > Phase::scanner_cpu) throw std::invalid_argument("unknown event phase");
    if (tick < now_ || (tick == now_ && phase_floor_ && phase < *phase_floor_))
        throw std::invalid_argument("event precedes dispatched time/phase");
    if (next_sequence_ == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("event sequence exhausted");
    events_.push({tick, phase, tag, next_sequence_});
    ++next_sequence_;
}
void Scheduler::run_until(Tick deadline, const Handler& handler) {
    if (running_) throw std::logic_error("recursive scheduler dispatch");
    if (!handler) throw std::invalid_argument("empty event handler");
    if (deadline < now_) throw std::invalid_argument("deadline precedes current time");
    running_ = true;
    try {
        while (!events_.empty() && events_.top().tick <= deadline) {
            const auto event = events_.top();
            events_.pop();
            now_ = event.tick;
            phase_floor_ = event.phase;
            handler(event);
        }
        if (deadline > now_) phase_floor_.reset();
        now_ = deadline;
        running_ = false;
    } catch (...) {
        running_ = false;
        throw;
    }
}
}
