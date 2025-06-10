#include <iostream>
#include <vector>
#include <thread>
#include <cassert>
#include <atomic>
#include <chrono>
#include "BluTransEntry/BluBooster.hpp"

using namespace BluBooster::Concurrent::AegisPtr::Internal;

constexpr int THREAD_COUNT = 8192;
std::atomic<int> destroy_count{ 0 };
std::atomic<int> shared_destroy_count{ 0 };
std::atomic<int> early_gc_count{ 0 };

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

struct EarlyGCAegisData {
    int value;
    EarlyGCAegisData(int v) : value(v) {
        std::cout << "[EarlyGC CREATE] " << this << '\n';
    }
    ~EarlyGCAegisData() {
        std::cout << "[EarlyGC DELETE] " << this << '\n';
    }
};

void aegis_guard_test(AegisPtrBaseHolder<AegisData, THREAD_COUNT>& holder, int tid) {
    AegisHolderGuard<AegisData, THREAD_COUNT> guard(holder, tid);
    AegisData* p = guard.use();
    assert(p->value == tid * 100);
    // simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void aegis_guard_shared_test(AegisPtrBaseHolder<SharedAegisData, THREAD_COUNT>& shared_holder, int tid) {
    AegisHolderGuard<SharedAegisData, THREAD_COUNT> guard(shared_holder, tid);
    SharedAegisData* p = guard.use();
    assert(p->value == 999);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void aegis_early_delete_test(AegisPtrBaseHolder<EarlyGCAegisData, THREAD_COUNT>& holder, int tid) {
    AegisHolderGuard<EarlyGCAegisData, THREAD_COUNT> guard(holder, tid);
    auto* ptr = guard.use();
    early_gc_count.fetch_add(1, std::memory_order_acq_rel);
    guard.unuse();
    if (early_gc_count.load(std::memory_order_acquire) > 4) {
        if (guard.use() == nullptr) {
            std::cout << early_gc_count.load(std::memory_order_acquire) << " early gc test success" << '\n';
        }
        else {
            guard.ref.try_delete();
        }
    }
}

void run_early_delete_test() {
    EarlyGCAegisData* ptr = new EarlyGCAegisData(900);
    AegisPtrBaseHolder<EarlyGCAegisData, THREAD_COUNT> early_gc_holder(ptr);
    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i)
        threads.emplace_back(aegis_early_delete_test, std::ref(early_gc_holder), i);

    for (auto& t : threads) t.join();

}
void run_shared_access_test() {
    SharedAegisData* ptr = new SharedAegisData(999);
    AegisPtrBaseHolder<SharedAegisData, THREAD_COUNT> shared_holder(ptr);
    std::vector<std::thread> threads;

    for (int i = 0; i < THREAD_COUNT; ++i)
        threads.emplace_back(aegis_guard_shared_test, std::ref(shared_holder), i);

    for (auto& t : threads) t.join();
    std::cout << "shared access done." << '\n';


    for (int j = 0; j < (THREAD_COUNT + 63) / 64; ++j)
        assert(shared_holder.m_base.flags.bits[j].load() == 0);

    std::cout << "[PASS] Shared AegisPtr multi-thread test passed.\n";
}

void run_integrity_test() {
    std::vector<std::thread> threads;
    std::vector<AegisPtrBaseHolder<AegisData, THREAD_COUNT>> holders;

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        AegisData* data = new AegisData(i * 100);
        holders.emplace_back(data);
    }

    for (int i = 0; i < THREAD_COUNT; ++i)
        threads.emplace_back(aegis_guard_test, std::ref(holders[i]), i);

    for (auto& t : threads) t.join();


    run_early_delete_test();
    // test 4 : run shared access test

    run_shared_access_test();
    assert(shared_destroy_count.load() == 1); // shared °´Ã¼´Â ÇÑ ¹ø¸¸ ÆÄ±«µÅ¾ß ÇÔ

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
    // test 3: double free -> covered implicitly by destroy_count
    assert(destroy_count.load() == THREAD_COUNT);
    auto t2 = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    std::cout << "[PASS] AegisPtr integrity test complete. All checks passed.\n" << "Elapsed Time :" << duration << "us\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    _CrtDumpMemoryLeaks();
    return 0;
}