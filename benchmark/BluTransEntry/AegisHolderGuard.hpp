#ifndef CMANAGERBASEPTRGUARD_HPP
#define CMANAGERBASEPTRGUARD_HPP
#include <cstdint>
#include <atomic>
#include "AegisPtrBaseHolder.hpp"
namespace BluBooster::Concurrent::AegisPtr::Internal {
    template <typename T,size_t MAX_THREADS = 16,bool AEGISPTR_FAST = false>
    struct AegisHolderGuard {
        AegisHolderGuard() = delete;
        AegisHolderGuard(AegisPtrBaseHolder<T,MAX_THREADS,AEGISPTR_FAST>& ptr,size_t tid) : ref(ptr), tid(tid) {
        }
        ~AegisHolderGuard() {
            std::atomic_thread_fence(std::memory_order_acquire);
            ref.m_base.flags.Unset(tid);
        }
        T* use() const {
            if(!hasProtected && ref.m_base.ptr){
                std::atomic_thread_fence(std::memory_order_release);
                ref.m_base.flags.Set(tid);
                hasProtected = true;
                return ref.m_base.ptr;
            }
            return ref.m_base.ptr;
        }
        void unuse() {
            std::atomic_thread_fence(std::memory_order_acquire);
            ref.m_base.flags.Unset(tid);
        }
        void setDisposable(bool disposable) {
            ref.isDisposable.store(disposable);
        }
        bool isUsing () const {
            return !ref.m_base.flags.CanDestroy(ref.m_base.flags);
        }
        AegisPtrBaseHolder<T,MAX_THREADS,AEGISPTR_FAST>& ref;
        size_t tid;
        mutable bool hasProtected = false;
    };
}

#endif // CMANAGERBASEPTRGUARD_HPP
