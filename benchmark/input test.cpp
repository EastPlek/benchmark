#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define GLOG_USE_GLOG_EXPORT
#define GLOG_CUSTOM_PREFIX_SUPPORT
#define GLOG_NO_ABBREVIATED_SEVERITIES

#include <WinSock2.h>
#include <Windows.h>
#include <glog/logging.h>
#include <folly/synchronization/Hazptr.h>
#include <folly/Memory.h>

#include <memory>
#include <thread>
#include <atomic>
#include <iostream>

#include "BluTransEntry/BluBooster.hpp"
#include <benchmark/benchmark.h>

using BluBooster::Concurrent::AegisPtr::Internal::AegisPtrBaseHolder;
using BluBooster::Concurrent::AegisPtr::Internal::AegisHolderGuard;

constexpr int THREAD_COUNT = 1024;

struct AegisData {
    int v;
    AegisData(int val) : v(val) {}
};

struct HazData : folly::hazptr_obj_base<HazData> {
    int v;
    HazData(int val) : v(val) {}
};

// ---------------------------------------------------------
// AegisPtr Benchmark
// ---------------------------------------------------------
static void BM_AegisPtr_MT(benchmark::State& state) {
    const int tid = state.thread_index();

    for (auto _ : state) {
        AegisPtrBaseHolder<AegisData> holder(new AegisData(123), true);
        AegisHolderGuard<AegisData> guard(holder);
        benchmark::DoNotOptimize(guard.use());
        holder.try_delete();
    }

    benchmark::ClobberMemory();
}
BENCHMARK(BM_AegisPtr_MT)->Threads(2)->Threads(4)->Threads(8)->Threads(16)->Threads(32)->Threads(64)->Threads(128)->Threads(256)->Threads(512)->Threads(1024);

// ---------------------------------------------------------
// Hazptr Benchmark
// ---------------------------------------------------------
static void BM_Hazptr_MT(benchmark::State& state) {
    const int tid = state.thread_index();

    for (auto _ : state) {
        auto* raw = new HazData(123);
        std::atomic<HazData*> ptr(raw);

        folly::hazptr_holder<> h = folly::make_hazard_pointer();
        HazData* p = h.protect(ptr);
        if (p) {
            benchmark::DoNotOptimize(p);
            p->retire(); // GC 후보 등록
        }
    }

    // 전체 루프 끝난 뒤 cleanup (정확도 ↑)
    if (tid == 0) folly::hazptr_cleanup();

    benchmark::ClobberMemory();

}
BENCHMARK(BM_Hazptr_MT)->Threads(2)->Threads(4)->Threads(8)->Threads(16)->Threads(32)->Threads(64)->Threads(128)->Threads(256)->Threads(512)->Threads(1024);

static void BM_SharedPtr_MT(benchmark::State& state) {
    const int tid = state.thread_index();

    for (auto _ : state) {
        std::shared_ptr<AegisData> ptr = std::make_shared<AegisData>(123);
        benchmark::DoNotOptimize(ptr);
    }

    benchmark::ClobberMemory();
}
BENCHMARK(BM_SharedPtr_MT)->Threads(2)->Threads(4)->Threads(8)->Threads(16)->Threads(32)->Threads(64)->Threads(128)->Threads(256)->Threads(512)->Threads(1024);

// ---------------------------------------------------------
BENCHMARK_MAIN();