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

#include <thread>
#include <chrono>
#include <cassert>
#include <iostream>
#include "BluTransEntry/BluBooster.hpp"

#include <benchmark/benchmark.h>

using BluBooster::Concurrent::AegisPtr::Internal::AegisPtrBaseHolder;
using BluBooster::Concurrent::AegisPtr::Internal::AegisHolderGuard;

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


static void BM_AegisPtr_MT(benchmark::State& state) {
	static AegisPtrBaseHolder<AegisData, THREAD_COUNT> holder(new AegisData(123),true);
	int tid = state.thread_index();

	for (auto _ : state) {
		AegisHolderGuard<AegisData, THREAD_COUNT> guard(holder, tid);
		benchmark::DoNotOptimize(guard.use());
	}
}
BENCHMARK(BM_AegisPtr_MT)->Threads(2)->Threads(4)->Threads(8)->Threads(12);

static void BM_Hazptr_MT(benchmark::State& state) {
	static std::atomic<HazData*> ptr(new HazData(123));
	int tid = state.thread_index();

	for (auto _ : state) {
		folly::hazptr_holder<> h = folly::make_hazard_pointer();
		HazData* p = h.protect(ptr);
		if (p) benchmark::DoNotOptimize(p);
	}

	if (tid == 0) folly::hazptr_cleanup();
}
BENCHMARK(BM_Hazptr_MT)->Threads(2)->Threads(4)->Threads(8)->Threads(12);
BENCHMARK_MAIN();