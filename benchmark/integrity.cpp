#include <iostream>
#include <vector>
#include <thread>
#include <cassert>
#include <atomic>
#include <chrono>
#include "BluTransEntry/BluBooster.hpp"

using namespace BluBooster::Concurrent::AegisPtr::Internal;

constexpr int THREAD_COUNT = 8;
std::atomic<int> destroy_count{ 0 };

struct AegisData {
    int value;
    AegisData(int v) : value(v) {}
    ~AegisData() {
        std::cout << "[DESTROY] " << this << '\n';
        destroy_count.fetch_add(1, std::memory_order_relaxed);
    }
};

void aegis_guard_test(AegisPtrBaseHolder<AegisData, 16>& holder, int tid) {
    AegisHolderGuard<AegisData, 16> guard(holder, tid);
    AegisData* p = guard.operator->();
    assert(p->value == tid * 100);
    // simulate some work
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void run_integrity_test() {
    std::vector<std::thread> threads;
    std::vector<AegisPtrBaseHolder<AegisData, 16>> holders;

    for (int i = 0; i < THREAD_COUNT; ++i)
        holders.emplace_back(i * 100);

    for (int i = 0; i < THREAD_COUNT; ++i)
        threads.emplace_back(aegis_guard_test, std::ref(holders[i]), i);

    for (auto& t : threads) t.join();

    // test 3: use-after-free (should crash in debug mode)
#ifdef _DEBUG
    try {
        AegisData* dangling = nullptr;
        {
            AegisPtrBaseHolder<AegisData, 16> temp(999);
            AegisHolderGuard<AegisData, 16> g(temp, 0);
            dangling = g.operator->();
        }
        std::cout << "[USE-AFTER-FREE TEST] " << dangling->value << '\n'; // likely crash in debug
    }
    catch (...) {
        std::cout << "[PASS] use-after-free triggered safely (caught)\n";
    }
#endif

    // test 4: double free -> covered implicitly by destroy_count
    assert(destroy_count.load() == THREAD_COUNT);

    //// test 5: check flag reset
    //for (int i = 0; i < THREAD_COUNT; ++i) {
    //    const auto& flags = holders[i].m_base.flags; // implement getter if needed
    //    for (int j = 0; j < 16; ++j)
    //    {
    //        uint64_t bits = flags[i].load(std::memory_order::acquire);
    //        assert(flags[i] == 0); // all flags cleared
    //    }
    //}

    std::cout << "[PASS] AegisPtr integrity test complete. All checks passed.\n";
}
int main() {
    run_integrity_test();
    _CrtDumpMemoryLeaks();
    return 0;
}