#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include "BluTransEntry/CPU.hpp"
#include "BluTransEntry/SharedObjects.hpp"
#include "BluTransEntry/Macros.hpp"

namespace BluBooster::Concurrent::AegisPtr::Internal {
    struct alignas(64) AegisGhostFlag {
        std::atomic<uint64_t> last_tid;
    };
    template <typename T>
    struct alignas(64) AegisPtrBaseGhostHolderSlot {
        T* ptr{ nullptr };
        AegisGhostFlag flag;
    };

    template <typename T>
    struct AegisPtrBaseGhostHolder;

    template <typename U>
    struct is_aegis_ptr_base_ghost_holder : std::false_type {};

    template <typename U>
    struct is_aegis_ptr_base_ghost_holder<AegisPtrBaseGhostHolder<U>> : std::true_type {};

    template <typename U>
    inline constexpr bool is_aegis_ptr_base_ghost_holder_v = is_aegis_ptr_base_ghost_holder<std::remove_cv_t<U>>::value;


    template <typename T>
    struct alignas(64) AegisPtrBaseGhostHolder {
        static_assert(!is_aegis_ptr_base_ghost_holder_v<T>, "T must not be itself AegisPtrBaseGhostHolder.");
        AegisPtrBaseGhostHolder() {
            m_base.ptr = nullptr;
        }
        AegisPtrBaseGhostHolder(const T& _rawData) {
            m_base.ptr = new T(_rawData);
        }
        AegisPtrBaseGhostHolder(T&& _mvRawData) {
            m_base.ptr = new T(std::move(_mvRawData));
        }
        AegisPtrBaseGhostHolder(T* _rawPtr) {
            m_base.ptr = _rawPtr;
        }
        AegisPtrBaseGhostHolder(const AegisPtrBaseGhostHolder<T>& other) = delete;
        AegisPtrBaseGhostHolder<T, MAX_THREADS>& operator= (const AegisPtrBaseGhostHolder<T>& other) = delete;
        AegisPtrBaseGhostHolder(AegisPtrBaseGhostHolder&& other) noexcept {
            assert(m_base.ptr == nullptr); // 이동 전 객체는 비어있어야
            m_base.ptr = std::exchange(other.m_base.ptr, nullptr); // 소유권 이전
            isDisposable = other.isDisposable.load(std::memory_order_acquire);
            m_base.flags = std::move(other.m_base.flags);
        }
        AegisPtrBaseGhostHolder& operator=(AegisPtrBaseGhostHolder<T, MAX_THREADS>&& other) = delete;
        ~AegisPtrBaseHolder() {
            if (isDisposable.load(std::memory_order_acquire) && !isDeleted.load(std::memory_order_acquire)
                && m_base.flags.CanDestroy() && m_base.ptr) {
                delete m_base.ptr;
                m_base.ptr = nullptr;
            }
        }

        void hold() {
            isHolding.store(true, std::memory_order_release);
        }
        void unhold() {
            isHolding.store(false, std::memo)
        }
        AegisPtrBaseHolderSlot<T, MAX_THREADS> m_base{};
        std::atomic<bool> isHolding{ false };
    };
}
