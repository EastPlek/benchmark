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

#include <cds/init.h>
#include <cds/gc/hp.h>
#include <cds/threading/model.h>
#include <chrono>
#include <iostream>
#include "BluTransEntry/BluBooster.hpp"

struct HazData : public folly::hazptr_obj_base<HazData> {
    int value;
    HazData(int v) : value(v) {}
    ~HazData() { /*std::cout << "[folly] deleted by thread\n"; */ }
};

void folly_worker(std::atomic<HazData*>* ptr, int tid) {
    folly::hazptr_holder<> h = folly::make_hazard_pointer();
    HazData* p = h.protect(*ptr);
    assert(p);
    int v = p->value;
    // Retire (thread-local)
    ptr->store(nullptr, std::memory_order_release);
    p->retire();
    // Protect 해제(소멸자 호출 기다림)
}

void folly_mt_test(int thread_count) {
    std::vector<std::thread> threads;
    std::vector<std::atomic<HazData*>> data(thread_count);
    for (int i = 0; i < thread_count; ++i)
        data[i].store(new HazData(i * 100));
    for (int i = 0; i < thread_count; ++i)
        threads.emplace_back(folly_worker, &data[i], i);
    for (auto& t : threads) t.join();
    folly::hazptr_cleanup();

}

using BluBooster::Concurrent::AegisPtr::Internal::AegisPtrBaseHolder;
using BluBooster::Concurrent::AegisPtr::Internal::AegisHolderGuard;

struct AegisData {
    int value;
    AegisData(int v) : value(v) {}
    ~AegisData() { /*std::cout << "[Aegis] deleted by thread\n";*/ }
};

void aegis_worker(AegisPtrBaseHolder<AegisData, 16>&holder, int tid) {
    AegisHolderGuard<AegisData, 16> guard(holder, tid);
    AegisData* p = guard.operator->();
    assert(p);
    int v = p->value;
    // guard 해제 → 이후 holder가 삭제될 때 delete
}

void aegis_mt_test(int thread_count) {
    std::vector<std::thread> threads;
    std::vector <AegisPtrBaseHolder<AegisData, 16>>holders;
    for (int i = 0; i < thread_count; ++i)
    {
        holders.push_back(AegisPtrBaseHolder<AegisData, 16>(i * 100));
    }
    for (int i = 0; i < thread_count; ++i)
        threads.emplace_back(aegis_worker, std::ref(holders[i]), i);
    for (auto& t : threads) t.join();
}

int main() {
    long long folly_duration{};
    long long aegis_duration{};
    uint64_t test_count{ 1'000};
    for(int i = 0; i < test_count; ++i) {
        const int thread_count = 8;
        if (i % 2 == 0) {
            auto t1 = std::chrono::high_resolution_clock::now();
            folly_mt_test(thread_count);
            auto t2 = std::chrono::high_resolution_clock::now();
            std::cout << "folly::hazptr: " << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << " us\n";
            folly_duration += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

            auto t3 = std::chrono::high_resolution_clock::now();
            aegis_mt_test(thread_count);
            auto t4 = std::chrono::high_resolution_clock::now();
            std::cout << "AegisPtr: " << std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count() << " us\n";
            aegis_duration += std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
        }
        else {
            auto t1 = std::chrono::high_resolution_clock::now();
            aegis_mt_test(thread_count);
            auto t2 = std::chrono::high_resolution_clock::now();
            std::cout << "AegisPtr: " << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << " us\n";
            aegis_duration += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

            auto t3 = std::chrono::high_resolution_clock::now();
            folly_mt_test(thread_count);
            auto t4 = std::chrono::high_resolution_clock::now();
            std::cout << "folly::hazptr: " << std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count() << " us\n";
            folly_duration += std::chrono::duration_cast<std::chrono::microseconds>(t4 - t3).count();
        }
        std::cout << "------------------------------------------------" << '\n';
        folly::hazptr_cleanup();

    }
    std::cout << "folly::hazptr Average: " << folly_duration / test_count << " us\n";
    std::cout << "AegisPtr Average: " << aegis_duration / test_count << " us\n";
    _CrtDumpMemoryLeaks();
    return 0;

}