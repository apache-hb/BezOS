#include <benchmark/benchmark.h>

#include <absl/container/btree_map.h>
#include <random>
#include "std/container/btree.hpp"

[[noreturn]]
void km::BugCheck(stdx::StringView, std::source_location) noexcept [[clang::nonblocking]] {
    assert(false && "BugCheck called in benchmark");
    __builtin_trap();
}

static void BM_AbseilBtreeInsert(benchmark::State& state) {
    absl::btree_map<int, int> btree;
    std::unique_ptr<int[]> keys(new int[state.range(0)]);
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, INT_MAX);
    for (int64_t i = 0; i < state.range(0); i++) {
        keys[i] = dist(mt);
    }

    for (auto _ : state) {
        for (int64_t i = 0; i < state.range(0); i++) {
            btree.insert({keys[i], i});
        }

        btree.clear();
    }
}

BENCHMARK(BM_AbseilBtreeInsert)
    ->Range(32, 8 << 10);

static void BM_BezosBtreeInsert(benchmark::State& state) {
    sm::BTreeMap<int, int> btree;
    std::unique_ptr<int[]> keys(new int[state.range(0)]);
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, INT_MAX);
    for (int64_t i = 0; i < state.range(0); i++) {
        keys[i] = dist(mt);
    }

    for (auto _ : state) {
        for (int64_t i = 0; i < state.range(0); i++) {
            btree.insert(keys[i], i);
        }

        btree.clear();
    }
}

BENCHMARK(BM_BezosBtreeInsert)
    ->Range(32, 8 << 10);
