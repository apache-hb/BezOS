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
    ->Range(32, 8 << 14);

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
    ->Range(32, 8 << 14);


static void BM_AbseilBtreeFind(benchmark::State& state) {
    absl::btree_map<int, int> btree;
    std::unique_ptr<int[]> keys(new int[state.range(0)]);
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, INT_MAX);
    for (int64_t i = 0; i < state.range(0); i++) {
        keys[i] = dist(mt);
        btree.insert({keys[i], i});
    }

    for (auto _ : state) {
        for (int64_t i = 0; i < state.range(0); i++) {
            auto it = btree.find(keys[i]);
            benchmark::DoNotOptimize(it);
        }
    }
}

BENCHMARK(BM_AbseilBtreeFind)
    ->Range(32, 8 << 14);

static void BM_BezosBtreeFind(benchmark::State& state) {
    sm::BTreeMap<int, int> btree;
    std::unique_ptr<int[]> keys(new int[state.range(0)]);
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, INT_MAX);
    for (int64_t i = 0; i < state.range(0); i++) {
        keys[i] = dist(mt);
        btree.insert(keys[i], i);
    }

    for (auto _ : state) {
        for (int64_t i = 0; i < state.range(0); i++) {
            auto it = btree.find(keys[i]);
            benchmark::DoNotOptimize(it);
        }
    }
}

BENCHMARK(BM_BezosBtreeFind)
    ->Range(32, 8 << 14);

static void BM_AbseilBtreeErase(benchmark::State& state) {
    absl::btree_map<int, int> btree;
    std::unique_ptr<int[]> keys(new int[state.range(0)]);
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, INT_MAX);
    for (int64_t i = 0; i < state.range(0); i++) {
        keys[i] = dist(mt);
    }

    std::shuffle(keys.get(), keys.get() + state.range(0), mt);

    for (auto _ : state) {
        state.PauseTiming();
        for (int64_t i = 0; i < state.range(0); i++) {
            btree.insert({keys[i], i});
        }
        state.ResumeTiming();

        for (int64_t i = 0; i < state.range(0); i++) {
            btree.erase(keys[i]);
        }
    }
}

BENCHMARK(BM_AbseilBtreeErase)
    ->Range(32, 8 << 14);

static void BM_BezosBtreeErase(benchmark::State& state) {
    sm::BTreeMap<int, int> btree;
    std::unique_ptr<int[]> keys(new int[state.range(0)]);
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, INT_MAX);
    for (int64_t i = 0; i < state.range(0); i++) {
        keys[i] = dist(mt);
    }

    std::shuffle(keys.get(), keys.get() + state.range(0), mt);

    for (auto _ : state) {
        state.PauseTiming();
        for (int64_t i = 0; i < state.range(0); i++) {
            btree.insert(keys[i], i);
        }
        state.ResumeTiming();

        for (int64_t i = 0; i < state.range(0); i++) {
            btree.remove(keys[i]);
        }
    }
}

BENCHMARK(BM_BezosBtreeErase)
    ->Range(32, 8 << 14);

static void BM_AbseilBtreeIterate(benchmark::State& state) {
    absl::btree_map<int, int> btree;
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, INT_MAX);
    for (int64_t i = 0; i < state.range(0); i++) {
        btree.insert({dist(mt), i});
    }

    for (auto _ : state) {
        for (const auto& [key, value] : btree) {
            benchmark::DoNotOptimize(auto(key));
            benchmark::DoNotOptimize(auto(value));
        }
    }
}

BENCHMARK(BM_AbseilBtreeIterate)
    ->Range(32, 8 << 14);

static void BM_BezosBtreeIterate(benchmark::State& state) {
    sm::BTreeMap<int, int> btree;
    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<int> dist(0, INT_MAX);
    for (int64_t i = 0; i < state.range(0); i++) {
        btree.insert(dist(mt), i);
    }

    for (auto _ : state) {
        for (const auto& [key, value] : btree) {
            benchmark::DoNotOptimize(auto(key));
            benchmark::DoNotOptimize(auto(value));
        }
    }
}

BENCHMARK(BM_BezosBtreeIterate)
    ->Range(32, 8 << 14);
