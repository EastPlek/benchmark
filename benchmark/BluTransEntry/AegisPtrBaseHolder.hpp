#ifndef CMANAGERBASEPTR_HPP
#define CMANAGERBASEPTR_HPP
#include <array>
#include <atomic>
#include <cstdint>
#include "CPU.hpp"
#include "SharedObjects.hpp"
#include "Macros.hpp"

namespace BluBooster::Concurrent::AegisPtr::Internal{
    struct alignas(64) AegisBits {
        std::atomic<uint8_t> bit;
        char padding[63];
    };
    template <size_t MAX_THREADS>
    struct alignas(64) AegisPtrBaseHolderFlags {

        AegisPtrBaseHolderFlags():  bits {}{

        }
        AegisPtrBaseHolderFlags(const AegisPtrBaseHolderFlags&) = delete;
        AegisPtrBaseHolderFlags& operator=(const AegisPtrBaseHolderFlags&) = delete;

        AegisPtrBaseHolderFlags(AegisPtrBaseHolderFlags&& other) {
            for (size_t i = 0; i < bits.size(); ++i)
                bits[i].bit.store(other.bits[i].bit.load(std::memory_order_relaxed));
        }
        AegisPtrBaseHolderFlags& operator= (AegisPtrBaseHolderFlags&& other) {
            for (size_t i = 0; i < bits.size(); ++i)
                bits[i].bit.store(other.bits[i].bit.load(std::memory_order_relaxed));
            return *this;
        }
        using BitType = AegisBits;
        std::array<BitType,MAX_THREADS> bits;
        BLUBOOSTER_FORCE_INLINE void Set(size_t tid) {
            bits[tid].bit.store(1,std::memory_order_release);
        }
        BLUBOOSTER_FORCE_INLINE void Unset(size_t tid) {
            bits[tid].bit.store(0, std::memory_order_release);
        }
        BLUBOOSTER_FORCE_INLINE bool CanDestroy() const {
            for(int i = 0; i < bits.size(); ++i)
                if(bits[i].bit.load(std::memory_order_acquire) != 0) return false;
            return true;
        }
    };

    
    template <typename T,size_t MAX_THREADS>
    struct alignas(64) AegisPtrBaseHolderSlot{
        T* ptr{nullptr};
        AegisPtrBaseHolderFlags<MAX_THREADS>flags;
    };

    template <typename T, size_t N>
    struct AegisPtrBaseHolder;

    template <typename U>
    struct is_aegis_ptr_base_holder : std::false_type{};

    template <typename U,size_t N>
    struct is_aegis_ptr_base_holder<AegisPtrBaseHolder<U,N>> : std::true_type{};

    template <typename U>
    inline constexpr bool is_aegis_ptr_base_holder_v = is_aegis_ptr_base_holder<std::remove_cv_t<U>>::value;


    template <typename T,size_t MAX_THREADS = 16>
    struct AegisPtrBaseHolder{
        static_assert(!is_aegis_ptr_base_holder_v<T>,"T must not be itself AegisPtrBaseHolder.");
        static_assert(MAX_THREADS % 8 == 0,"Threads Must be divided by 8. ");
        AegisPtrBaseHolder() {
            m_base.ptr = nullptr;
        }
        AegisPtrBaseHolder(const T& _rawData) {
            m_base.ptr = new T (_rawData);
        }
        AegisPtrBaseHolder(T&& _mvRawData) {
            m_base.ptr = new T (std::move(_mvRawData));
        }
        AegisPtrBaseHolder(T* _rawPtr,bool disposable = false) {
            m_base.ptr = _rawPtr;
            isDisposable.store(disposable, std::memory_order_release);
        }
        AegisPtrBaseHolder(const AegisPtrBaseHolder<T, MAX_THREADS>& other) = delete;
        AegisPtrBaseHolder<T,MAX_THREADS>& operator= (const AegisPtrBaseHolder<T,MAX_THREADS>& other) = delete;
        AegisPtrBaseHolder(AegisPtrBaseHolder&& other) noexcept {
            assert(m_base.ptr == nullptr); // 이동 전 객체는 비어있어야
            m_base.ptr = std::exchange(other.m_base.ptr, nullptr); // 소유권 이전
            isDisposable = other.isDisposable.load(std::memory_order_acquire);
            m_base.flags = std::move(other.m_base.flags);
        }
        AegisPtrBaseHolder& operator=(AegisPtrBaseHolder<T,MAX_THREADS>&& other) = delete;
        ~AegisPtrBaseHolder() {
            if (isDisposable.load(std::memory_order_acquire) && !isDeleted.load(std::memory_order_acquire)
                && m_base.flags.CanDestroy() && m_base.ptr) {
                delete m_base.ptr;
                m_base.ptr = nullptr;
            }
        }

        bool try_delete() {
            if (!isDeleted.load(std::memory_order_acquire) && m_base.flags.CanDestroy() && m_base.ptr) {
                isDeleted.store(true, std::memory_order_release);
                delete m_base.ptr;
                m_base.ptr = nullptr;
                return true;
            }
            return false;
        }
        AegisPtrBaseHolderSlot<T, MAX_THREADS> m_base{};
        std::atomic<bool> isDeleted{ false };
        std::atomic<bool> isDisposable{ false };
    };

}


#endif // CMANAGERBASEPTR_HPP
