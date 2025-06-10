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


BluBooster::Concurrent::AegisPtr::Internal::AegisPtrBaseHolderFlags<16> flags;

static void BM_Set(benchmark::State& state) {
	AegisPtrBaseHolder<AegisData, 16> holder;
	int tid = state.thread_index();
	for (auto _ : state)
		holder.m_base.flags.Set(tid); // 고정된 tid로 테스트
}
BENCHMARK(BM_Set)->Threads(2)->Threads(4)->Threads(8)->Threads(16);

static void BM_Unset(benchmark::State& state) {
	AegisPtrBaseHolder<AegisData, 16> holder;
	int tid = state.thread_index();
	for (auto _ : state)
		holder.m_base.flags.Unset(tid);
}
BENCHMARK(BM_Unset)->Threads(2)->Threads(4)->Threads(8)->Threads(16);

static void BM_GuardGeneration(benchmark::State& state) {
	AegisPtrBaseHolder<AegisData, 16> holder;
	int tid = state.thread_index();
	for (auto _ : state)
		AegisHolderGuard<AegisData, 16>(holder, tid);
}
BENCHMARK(BM_GuardGeneration)->Threads(2)->Threads(4)->Threads(8)->Threads(16);

static void BM_AegisPtr_MT(benchmark::State& state) {
	static AegisPtrBaseHolder<AegisData, THREAD_COUNT> holder(new AegisData(123));
	int tid = state.thread_index();

	for (auto _ : state) {
		AegisHolderGuard<AegisData, THREAD_COUNT> guard(holder, tid);
		benchmark::DoNotOptimize(guard.use());
	}
}
BENCHMARK(BM_AegisPtr_MT)->Threads(2)->Threads(4)->Threads(8)->Threads(16);

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
BENCHMARK(BM_Hazptr_MT)->Threads(2)->Threads(4)->Threads(8)->Threads(16);
BENCHMARK_MAIN();