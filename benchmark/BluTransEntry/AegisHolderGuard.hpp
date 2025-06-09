#ifndef CMANAGERBASEPTRGUARD_HPP
#define CMANAGERBASEPTRGUARD_HPP
#include <cstdint>
#include <atomic>
#include "AegisPtrBaseHolder.hpp"
namespace BluBooster::Concurrent::AegisPtr::Internal {
    template <typename T,size_t MAX_THREADS = 16>
    struct AegisHolderGuard {
        AegisHolderGuard() = delete;
        AegisHolderGuard(AegisPtrBaseHolder<T,MAX_THREADS>& ptr,size_t tid) : ref(ptr), tid(tid) {
        }
        ~AegisHolderGuard() {
            std::atomic_thread_fence(std::memory_order_acquire);
            ref.m_base.flags.Unset(tid);
            ref.~AegisPtrBaseHolder();
        }
        T* operator->() const {
            if(!hasProtected){
                std::atomic_thread_fence(std::memory_order_release);
                ref.m_base.flags.Set(tid);
                hasProtected = true;
                return ref.m_base.ptr;
            }
            return ref.m_base.ptr;
        }
        bool isUsing () const {
            return !ref.m_base.flags.CanDestroy(ref.m_base.flags);
        }
        AegisPtrBaseHolder<T,MAX_THREADS>& ref;
        size_t tid;
        mutable bool hasProtected = false;
    };
}

#endif // CMANAGERBASEPTRGUARD_HPP
