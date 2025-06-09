#include <iostream>
#include <vector>
#include <thread>
#include <cassert>
#include <atomic>
#include <chrono>
#include "BluTransEntry/BluBooster.hpp"

using namespace BluBooster::Concurrent::AegisPtr::Internal;

constexpr int THREAD_COUNT = 16;
std::atomic<int> destroy_count{ 0 };
std::atomic<int> shared_destroy_count{ 0 };

struct AegisData {
    int value;
    AegisData(int v) : value(v) {
        std::cout << "[CREATE] " << this << '\n';
    }
    ~AegisData() {
        std::cout << "[DESTROY] " << this << '\n';
        destroy_count.fetch_add(1);
    }
};

struct SharedAegisData {
    int value;
    SharedAegisData(int v) : value(v) {
        std::cout << "[SHARED CREATE] " << this << '\n';
    }
    ~SharedAegisData() {
        std::cout << "[SHARED DELETE] " << this << '\n';
        shared_destroy_count.fetch_add(1);
    }
};

void aegis_guard_test(AegisPtrBaseHolder<AegisData, THREAD_COUNT>& holder, int tid) {
    AegisHolderGuard<AegisData, THREAD_COUNT> guard(holder, tid);
    AegisData* p = guard.operator->();
    assert(p->value == tid * 100);
    // simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void aegis_guard_shared_test(AegisPtrBaseHolder<SharedAegisData, THREAD_COUNT>& shared_holder, int tid) {
    AegisHolderGuard<SharedAegisData, THREAD_COUNT> guard(shared_holder, tid);
    SharedAegisData* p = guard.operator->();
    assert(p->value == 999);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void run_shared_access_test() {
    AegisPtrBaseHolder<SharedAegisData, THREAD_COUNT> shared_holder(999);
    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i)
        threads.emplace_back(aegis_guard_shared_test, std::ref(shared_holder), i);

    for (auto& t : threads) t.join();

    assert(shared_destroy_count.load() == 1); // shared °´Ã¼´Â ÇÑ ¹ø¸¸ ÆÄ±«µÅ¾ß ÇÔ

    for (int j = 0; j < (THREAD_COUNT + 63) / 64; ++j)
        assert(shared_holder.m_base.flags.bits[j].load() == 0);

    std::cout << "[PASS] Shared AegisPtr multi-thread test passed.\n";
}

void run_integrity_test() {
    std::vector<std::thread> threads;
    std::vector<AegisPtrBaseHolder<AegisData, THREAD_COUNT>> holders;

    for (int i = 0; i < THREAD_COUNT; ++i)
        holders.emplace_back(i * 100);

    for (int i = 0; i < THREAD_COUNT; ++i)
        threads.emplace_back(aegis_guard_test, std::ref(holders[i]), i);

    for (auto& t : threads) t.join();

    // test 3: double free -> covered implicitly by destroy_count
    assert(destroy_count.load() == THREAD_COUNT);

    // test 4 : run shared access test

    run_shared_access_test();

    // test 5: check flag reset
    for (int i = 0; i < THREAD_COUNT; ++i) {
        const auto& flags = holders[i].m_base.flags.bits; // implement getter if needed
        for (int j = 0; j < (THREAD_COUNT + 63) / 64; ++j)
        {
            assert(flags[j].load(std::memory_order::acquire) == 0); // all flags cleared
        }
    }
}
int main() {
    auto t1 = std::chrono::system_clock::now();
    run_integrity_test();
    auto t2 = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    std::cout << "[PASS] AegisPtr integrity test complete. All checks passed.\n" << "Elapsed Time :" << duration << "us\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    _CrtDumpMemoryLeaks();
    return 0;
}