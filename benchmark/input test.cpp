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

constexpr int THREAD_COUNT = 64;

struct AegisData {
	int v;
	AegisData(int val) : v(val) {}
};

struct HazData : folly::hazptr_obj_base<HazData> {
	int v;
	HazData(int val) : v(val) {}
};

static void BM_SingleThreadAegisPtr(benchmark::State& state) {
	using AegisHolder = AegisPtrBaseHolder<AegisData, THREAD_COUNT>;
	AegisHolder holder(new AegisData(123));

	for (auto _ : state) {
		AegisHolderGuard<AegisData, THREAD_COUNT> guard(holder, 0); // tid = 0
		AegisData* ptr = guard.use();
		benchmark::DoNotOptimize(ptr);
	}
}

static void BM_AegisPtr_MT(benchmark::State& state) {
	static AegisPtrBaseHolder<AegisData, THREAD_COUNT> holder(new AegisData(123));
	int tid = state.thread_index();

	for (auto _ : state) {
		AegisHolderGuard<AegisData, THREAD_COUNT> guard(holder, tid);
		benchmark::DoNotOptimize(guard.use());
		if (tid == 0)
			holder.try_delete();
	}
}

static void BM_SingleThreadHazptr(benchmark::State& state) {
	std::atomic<HazData*> ptr(new HazData(123));

	for (auto _ : state) {
		folly::hazptr_holder<> h = folly::make_hazard_pointer();
		HazData* p = h.protect(ptr);
		if (p) {
			benchmark::DoNotOptimize(p);
			ptr.store(nullptr, std::memory_order_release);
			p->retire();
		}
	}
	folly::hazptr_cleanup(); // GC ผ๖วเ

}

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

BENCHMARK(BM_SingleThreadAegisPtr);
BENCHMARK(BM_SingleThreadHazptr);
BENCHMARK(BM_AegisPtr_MT)->Threads(1)->Threads(2)->Threads(4)->Threads(8)->Threads(16)->Threads(32)->Threads(64);
BENCHMARK(BM_Hazptr_MT)->Threads(1)->Threads(2)->Threads(4)->Threads(8)->Threads(16)->Threads(32)->Threads(64);
BENCHMARK_MAIN();