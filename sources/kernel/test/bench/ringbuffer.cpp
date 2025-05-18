#include <format>

#include <benchmark/benchmark.h>
#include <random>
#include <thread>
#include "std/ringbuffer.hpp"
#include "util/format.hpp"

struct DataObject {
    int data[48]{};
};

/// @brief Baseline benchmark
static void BM_CopyObject(benchmark::State& state) {
    DataObject src;
    for (auto _ : state) {
        DataObject dst = src;
        benchmark::DoNotOptimize(dst);
    }
}

BENCHMARK(BM_CopyObject);

sm::AtomicRingQueue<DataObject> gQueue;
std::jthread gConsumer;

static void BM_QueueObjectSetup(const benchmark::State& state) {
    uint32_t capacity = state.range(0);
    if (OsStatus status = sm::AtomicRingQueue<DataObject>::create(capacity, &gQueue)) {
        throw std::runtime_error(std::format("Failed to create queue of size '{}' ({})", capacity, std::string_view(km::format(OsStatusId(status)))));
    }

    gConsumer = std::jthread([](std::stop_token stop) {
        DataObject sink;
        while (!stop.stop_requested()) {
            gQueue.tryPop(sink);
        }
    });
}

static void BM_QueueObjectTeardown(const benchmark::State&) {
    gConsumer.request_stop();
    gConsumer.join();
}

static void BM_QueueObject(benchmark::State& state) {
    std::mt19937 mt{0x1234};
    DataObject src;
    std::uniform_int_distribution<> dist(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    for (auto& c : src.data) {
        c = dist(mt);
    }

    for (auto _ : state) {
        gQueue.tryPush(src);
    }
}

BENCHMARK(BM_QueueObject)
    ->Setup(BM_QueueObjectSetup)
    ->Teardown(BM_QueueObjectTeardown)
    ->Range(32, 8 << 10)
    ->ThreadRange(1, 64);
