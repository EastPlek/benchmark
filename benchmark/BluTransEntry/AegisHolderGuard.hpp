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
            unuse();
        }
        T* use() const {
            if(!hasProtected && !isGuarding){
                ref.m_base.flags.Set(tid);
                hasProtected = true;
                isGuarding = true;
            }
            return ref.m_base.ptr;
        }
        void unuse() {
            if(isGuarding){
                ref.m_base.flags.Unset(tid);
                isGuarding = false;
            }
        }
        void setDisposable(bool disposable) {
            ref.isDisposable.store(disposable);
        }
        bool isUsing () const {
            return !ref.m_base.flags.CanDestroy(ref.m_base.flags);
        }
        AegisPtrBaseHolder<T,MAX_THREADS>& ref;
        size_t tid;
        mutable bool hasProtected = false;
        mutable bool isGuarding = false;
    };



}

#endif // CMANAGERBASEPTRGUARD_HPP
