// SPDX-License-Identifier: AGPL-3.0-only
#include "core/scheduler.hpp"
#include "core/main_memory.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <stdexcept>
#include <vector>

TEST_CASE("Same-tick events use phase order then insertion order") {
    emu::Scheduler scheduler;
    scheduler.schedule(5, emu::Phase::scanner_cpu, 50);
    scheduler.schedule(5, emu::Phase::main_cpu, 40);
    scheduler.schedule(5, emu::Phase::device, 30);
    scheduler.schedule(5, emu::Phase::bus, 20);
    scheduler.schedule(5, emu::Phase::reset, 10);
    scheduler.schedule(5, emu::Phase::device, 31);
    scheduler.schedule(6, emu::Phase::reset, 60);
    std::vector<std::uint32_t> trace;
    scheduler.run_until(5, [&](const emu::Event& e) { trace.push_back(e.tag); });
    REQUIRE(trace == std::vector<std::uint32_t>{10, 20, 30, 31, 40, 50});
    REQUIRE(scheduler.now() == 5);
    REQUIRE(scheduler.pending() == 1);
}

namespace {
std::vector<emu::Event> replay(bool chunked) {
    emu::Scheduler scheduler;
    scheduler.schedule(0, emu::Phase::reset, 1);
    scheduler.schedule(5, emu::Phase::device, 2);
    scheduler.schedule(10, emu::Phase::bus, 3);
    scheduler.schedule(10, emu::Phase::main_cpu, 4);
    std::vector<emu::Event> trace;
    const auto handler = [&](const emu::Event& e) {
        trace.push_back(e);
        if (e.tag == 2) {
            scheduler.schedule(5, emu::Phase::device, 5);
            scheduler.schedule(10, emu::Phase::reset, 6);
        }
    };
    if (chunked) for (const auto deadline : {0, 3, 5, 5, 9, 10, 20}) scheduler.run_until(deadline, handler);
    else scheduler.run_until(20, handler);
    REQUIRE(scheduler.now() == 20);
    REQUIRE(scheduler.pending() == 0);
    return trace;
}
}
TEST_CASE("Chunked replay including handler-enqueued events matches one-shot replay") {
    const auto trace = replay(false);
    REQUIRE(trace == replay(true));
    std::vector<std::uint32_t> tags;
    for (const auto& e : trace) tags.push_back(e.tag);
    REQUIRE(tags == std::vector<std::uint32_t>{1, 2, 5, 6, 3, 4});
}
TEST_CASE("Past events and phase inversion are rejected across run boundaries") {
    emu::Scheduler scheduler;
    scheduler.schedule(5, emu::Phase::main_cpu, 1);
    scheduler.run_until(5, [](const emu::Event&) {});
    REQUIRE_THROWS_AS(scheduler.schedule(4, emu::Phase::scanner_cpu, 0), std::invalid_argument);
    REQUIRE_THROWS_AS(scheduler.schedule(5, emu::Phase::reset, 0), std::invalid_argument);
    REQUIRE_THROWS_AS(scheduler.run_until(4, [](const emu::Event&) {}), std::invalid_argument);
    REQUIRE_THROWS_AS(scheduler.schedule(6, static_cast<emu::Phase>(255), 0), std::invalid_argument);
    REQUIRE(scheduler.now() == 5);
    REQUIRE(scheduler.pending() == 0);
    scheduler.schedule(5, emu::Phase::scanner_cpu, 2);
    scheduler.run_until(6, [](const emu::Event&) {});
    REQUIRE_NOTHROW(scheduler.schedule(6, emu::Phase::reset, 3));
}
TEST_CASE("Handler cannot schedule an earlier same-tick phase or dispatch recursively") {
    emu::Scheduler scheduler;
    scheduler.schedule(1, emu::Phase::device, 1);
    scheduler.run_until(1, [&](const emu::Event&) {
        REQUIRE_THROWS_AS(scheduler.schedule(1, emu::Phase::bus, 2), std::invalid_argument);
        REQUIRE_THROWS_AS(scheduler.run_until(1, [](const emu::Event&) {}), std::logic_error);
        scheduler.schedule(2, emu::Phase::reset, 3);
    });
    REQUIRE(scheduler.pending() == 1);
}
TEST_CASE("Throwing and empty handlers leave an explicit recoverable queue state") {
    emu::Scheduler scheduler;
    scheduler.schedule(2, emu::Phase::device, 1);
    scheduler.schedule(2, emu::Phase::scanner_cpu, 2);
    REQUIRE_THROWS_AS(scheduler.run_until(10, {}), std::invalid_argument);
    REQUIRE(scheduler.now() == 0);
    REQUIRE(scheduler.pending() == 2);
    REQUIRE_THROWS_AS(scheduler.run_until(10, [](const emu::Event&) { throw std::runtime_error("device failed"); }), std::runtime_error);
    REQUIRE(scheduler.now() == 2);
    REQUIRE(scheduler.pending() == 1);
    REQUIRE_THROWS_AS(scheduler.schedule(2, emu::Phase::reset, 3), std::invalid_argument);
    std::vector<std::uint32_t> trace;
    scheduler.run_until(10, [&](const emu::Event& e) { trace.push_back(e.tag); });
    REQUIRE(trace == std::vector<std::uint32_t>{2});
    REQUIRE(scheduler.now() == 10);
}
TEST_CASE("Full-range integer deadlines neither wrap nor lose precision") {
    emu::Scheduler scheduler;
    constexpr auto end = std::numeric_limits<emu::Tick>::max();
    scheduler.schedule(end, emu::Phase::device, 1);
    scheduler.run_until(end - 1, [](const emu::Event&) { FAIL("event dispatched early"); });
    REQUIRE(scheduler.pending() == 1);
    scheduler.run_until(end, [&](const emu::Event& e) {
        REQUIRE(e.tick == end);
        REQUIRE(scheduler.now() == end);
    });
    REQUIRE(scheduler.pending() == 0);
}
TEST_CASE("Synthetic reset bus and CPU trace observes DMA ownership") {
    // Contract example, not a DMA peripheral or running Z80 integration.
    emu::Scheduler scheduler;
    emu::MainMemory memory;
    bool dma_owns_bus = false;
    scheduler.schedule(5, emu::Phase::main_cpu, 4);
    scheduler.schedule(5, emu::Phase::bus, 2);
    scheduler.schedule(5, emu::Phase::reset, 1);
    scheduler.schedule(5, emu::Phase::device, 3);
    scheduler.schedule(6, emu::Phase::bus, 5);
    scheduler.schedule(6, emu::Phase::main_cpu, 4);
    unsigned cpu_writes = 0;
    scheduler.run_until(6, [&](const emu::Event& e) {
        switch (e.tag) {
        case 1: memory.reset_paging(); break;
        case 2: dma_owns_bus = true; break;
        case 3: memory.write(emu::MainMemory::Bus::dma, 0x2000, 0x5a); break;
        case 4:
            if (!dma_owns_bus) { memory.write(emu::MainMemory::Bus::cpu, 0x2000, 0x42); ++cpu_writes; }
            else REQUIRE(memory.read(emu::MainMemory::Bus::cpu, 0x2000) == 0x5a);
            break;
        case 5: dma_owns_bus = false; break;
        }
    });
    REQUIRE(cpu_writes == 1);
    REQUIRE(memory.read(emu::MainMemory::Bus::dma, 0x2000) == 0x42);
}
