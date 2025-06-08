#ifndef SYSTEMALLOCATOR_HPP
#define SYSTEMALLOCATOR_HPP

#include <stdint.h>
#include <stdexcept>
#if defined(BLUBOOSTER_OS_WINDOWS)
#include <windows.h>
#include <cstdlib>
#elif defined(BLUBOOSTER_OS_LINUX)
#endif
namespace BluBooster::Memory::PlatformPolicy {
    void* SystemAllignedAllocate(size_t size,size_t align) {
    #if defined(BLUBOOSTER_OS_WINDOWS)
        return _aligned_malloc(size,align);
    #elif defined(BLUBOOSTER_OS_LINUX)
        void* p = nullptr;
        if(posix_memalign(&p, align, size) != 0)
            return nullptr;
        return p;
    #endif
    }
    void SystemAllignedFree(void* p) {
    #if defined(BLUBOOSTER_OS_WINDOWS)
        _aligned_free(p);
    #elif defined(BLUBOOSTER_OS_LINUX)
        std::free(p);
    #endif
    }
}
namespace BluBooster::Memory {
    class SystemAllocator {
    public:
        template <typename T,size_t Align = alignof(T),typename... Args>
        static T* Alloc(Args&&... args){
            static_assert(Align >= alignof(T),
                          "requested alignment is less than natural alignment of T.");
            void* p = nullptr;
            if(Align <= 16)
                p = std::malloc(sizeof(T));
            else
                p = BluBooster::Memory::PlatformPolicy::SystemAllignedAllocate(sizeof(T),Align);
            if(!p)
                return nullptr;
            return new (p) T(std::forward<Args>(args)...);
        }
        template<typename T,size_t Align = alignof(T)>
        static void Free(T* ptr) {
            static_assert(Align >= alignof(T),
                "requested alignment is less than natural alignment of T.");
            if constexpr(!std::is_trivially_destructible_v<T>)
                std::destroy_at(ptr);
            if constexpr (Align <= 16)
                std::free(ptr);
            else
                BluBooster::Memory::PlatformPolicy::SystemAllignedFree(ptr);
            ptr = nullptr;
        }
        template<typename T, size_t Align = alignof(T)>
        static T* AllocArray(size_t size,std::initializer_list<T> = {}) {
            static_assert(Align >= alignof(T),
                          "requrest alignment is less than natural alignment of T.");
            {

            }
        }
    };
}

#endif // SYSTEMALLOCATOR_HPP
