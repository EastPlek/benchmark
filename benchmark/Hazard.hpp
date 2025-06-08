// FastHazardPtr.hpp
// High-performance, portable Hazard Pointer implementation (C++17 compatible)

#pragma once
#include <atomic>
#include <cstddef>
#include <functional>
#include <thread>
#include <vector>
#include <algorithm>
#include <memory>
#include <immintrin.h> // For SIMD if available

// Usage:
//   FastHazardPtr<MyData>::protect(ptr);
//   // ... use ptr ...
//   FastHazardPtr<MyData>::unprotect();
//   FastHazardPtr<MyData>::retire(ptr, [](MyData* p){ delete p; });

template <typename T, std::size_t MaxThreads = 128, std::size_t RetireThreshold = 64>
class FastHazardPtr {
public:
    using Deleter = std::function<void(T*)>;

    // Initialize the global hazard slots once
    static void init() {
        static std::once_flag flag;
        std::call_once(flag, [] {
            hazardSlots().resize(MaxThreads);
            for (auto& slot : hazardSlots()) {
                slot.store(nullptr, std::memory_order_relaxed);
            }
            });
    }

    // Get a unique slot index for this thread
    static std::size_t slot_index() {
        static thread_local std::size_t idx = registry().fetch_add(1, std::memory_order_relaxed);
        return (idx < MaxThreads) ? idx : (idx % MaxThreads);
    }

    // Protect pointer in this thread's slot
    static void protect(T* p) {
        init();
        hazardSlots()[slot_index()].store(p, std::memory_order_release);
    }

    // Clear protection for this thread
    static void unprotect() {
        hazardSlots()[slot_index()].store(nullptr, std::memory_order_release);
    }

    // Retire pointer with custom deleter; triggers scan when threshold reached
    static void retire(T* p, Deleter d) {
        auto& buf = retireBuffer();
        buf.push_back(Retired{ p, d });
        if (buf.size() >= RetireThreshold) {
            scan_and_reclaim();
        }
    }

private:
    struct Retired { T* ptr; Deleter del; };

    // Global array of hazard pointers
    static std::vector<std::atomic<T*>>& hazardSlots() {
        static std::vector<std::atomic<T*>> slots;
        return slots;
    }

    // Thread-local retired pointer buffer
    static std::vector<Retired>& retireBuffer() {
        thread_local std::vector<Retired> buf;
        return buf;
    }

    // Registry for assigning slot indices
    static std::atomic<std::size_t>& registry() {
        static std::atomic<std::size_t> reg{ 0 };
        return reg;
    }

    // Scan hazards and reclaim safe retired pointers
    static void scan_and_reclaim() {
        init();
        std::vector<T*> hazards;
        hazards.reserve(MaxThreads);
        for (auto& slot : hazardSlots()) {
            T* hp = slot.load(std::memory_order_acquire);
            if (hp) hazards.push_back(hp);
        }
        std::sort(hazards.begin(), hazards.end());

        auto& buf = retireBuffer();
        std::size_t dst = 0;
        for (std::size_t i = 0, n = buf.size(); i < n; ++i) {
            T* p = buf[i].ptr;
            if (!std::binary_search(hazards.begin(), hazards.end(), p)) {
                buf[i].del(p);
            }
            else {
                buf[dst++] = buf[i];
            }
        }
        buf.resize(dst);
    }
};
