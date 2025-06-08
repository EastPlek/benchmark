#ifndef SHAREDOBJECTS_HPP
#define SHAREDOBJECTS_HPP
#include "CPU.hpp"
#include "SystemAllocator.hpp"

namespace BluBooster {
    template <typename T,size_t Align = alignof(T),typename... Args>
    static T* Alloc(Args&&... args) {
        return BluBooster::Memory::SystemAllocator::Alloc<T,Align>(std::forward<Args>(args)...);
    }
    inline static BluBooster::CPU::RuntimeCPUFeatures cpuFeatures{};
    inline static BluBooster::CPU::RuntimeFeatureLevel cpuFeatureLevel{};
}
#endif // SHAREDOBJECTS_HPP
