#ifndef CMANAGERBASEPTR_HPP
#define CMANAGERBASEPTR_HPP
#include <array>
#include <atomic>
#include <cstdint>
#include "CPU.hpp"
#include "SharedObjects.hpp"
#include "Macros.hpp"

namespace BluBooster::Concurrent::AegisPtr::Internal{

    struct  AegisBits {
        using BitType = std::atomic<uint64_t>;
        BitType bit{ 0 };
        //char padding[64 - sizeof(bit)];
    };
    struct AegisPtrBaseHolderFlags {

        AegisPtrBaseHolderFlags():  bits {}{
        }
        ~AegisPtrBaseHolderFlags() {
        }
        AegisPtrBaseHolderFlags(const AegisPtrBaseHolderFlags&) = delete;
        AegisPtrBaseHolderFlags& operator=(const AegisPtrBaseHolderFlags&) = delete;

        AegisPtrBaseHolderFlags(AegisPtrBaseHolderFlags&& other) {
            bits.bit.store(other.bits.bit.load(std::memory_order_relaxed));
        }
        AegisPtrBaseHolderFlags& operator= (AegisPtrBaseHolderFlags&& other) {
            bits.bit.store(other.bits.bit.load(std::memory_order_relaxed));
            return *this;
        }
        AegisBits bits;
        BLUBOOSTER_FORCE_INLINE void Set() {
                bits.bit.fetch_add(1, std::memory_order_relaxed);
        }
        BLUBOOSTER_FORCE_INLINE void Unset() {
            bits.bit.fetch_sub(1, std::memory_order_relaxed);
        }
        BLUBOOSTER_FORCE_INLINE bool CanDestroy()  {
            return bits.bit.load(std::memory_order_acquire) == 0;
        }
    };

    
    template <typename T>
    struct AegisPtrBaseHolderSlot{
        T* ptr{nullptr};
        AegisPtrBaseHolderFlags flags;
    };


    template <typename T>
    struct AegisPtrBaseHolder{
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
        AegisPtrBaseHolder(const AegisPtrBaseHolder<T>& other) = delete;
        AegisPtrBaseHolder<T>& operator= (const AegisPtrBaseHolder<T>& other) = delete;
        AegisPtrBaseHolder(AegisPtrBaseHolder&& other) noexcept {
            assert(m_base.ptr == nullptr); // 이동 전 객체는 비어있어야
            m_base.ptr = std::exchange(other.m_base.ptr, nullptr); // 소유권 이전
            isDisposable = other.isDisposable.load(std::memory_order_acquire);
            m_base.flags = std::move(other.m_base.flags);
        }
        AegisPtrBaseHolder& operator=(AegisPtrBaseHolder<T>&& other) = delete;
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
        AegisPtrBaseHolderSlot<T> m_base{};
        std::atomic<bool> isDeleted{ false };
        std::atomic<bool> isDisposable{ false };
    };

}


#endif // CMANAGERBASEPTR_HPP
