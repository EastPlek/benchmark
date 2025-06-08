#ifndef BLUBOOSTER_HPP
#define BLUBOOSTER_HPP
//========================================================================================
//
//        ▒▒▒▒▒▒      ▒▒             ▒▒▒▒▒▒
//       ▒▒    ▒▒    ▒▒             ▒▒    ▒▒                         ▒
//      ▒▒▒▒▒▒▒     ▒▒   ▒▒   ▒▒    ▒▒▒▒▒▒▒   ▒▒▒▒   ▒▒▒▒    ▒▒▒  ▒▒▒▒▒  ▒▒▒▒    ▒▒ ▒▒▒
//      ▒▒     ▒▒  ▒▒   ▒▒   ▒▒▒   ▒▒     ▒▒ ▒▒  ▒▒ ▒▒  ▒▒  ▒▒▒    ▒▒   ▒▒▒ ▒▒  ▒▒▒▒
//     ▒▒▒▒▒▒▒▒▒  ▒▒   ▒▒▒▒▒▒▒ ▒▒ ▒▒▒▒▒▒▒▒▒  ▒▒▒▒   ▒▒▒▒  ▒▒▒▒    ▒▒    ▒▒▒▒▒  ▒▒
//
//                                                                         EastPlek, 2025
//========================================================================================
// TITLE     : BluBooster - BluTrans-Independent Performance Booster Utility
// CREATED   : 2025-06-05
// AUTHOR    : EastPlek
//========================================================================================
// Requirements :
// ---------------------------------------------------------------------------------------
// * CPU (x86) : equivalent to or newer than Intel Haswel architecture
//             : equivalent to or newer than Sapphire Rapids architecture (AMX Support)
//
// * CPU (ARM) : equivalent to or newer than ARM-v8
//
// * Graphic Cards (CUDA) : higher or equal CUDA 12.6
//========================================================================================
// BluBooster Overview (Simplified Directory Tree)
// ---------------------------------------------------------------------------------------
// BluBooster.hpp
// ├── CPU/
// │   └── CPU.hpp
// │       ├── CPU_Features.hpp
// │       ├── CPU_Sync.hpp
// │       ├── CPU_Bits.hpp
// │       ├── CPU_Cache.hpp (SSE2)
// │       ├── CPU_Info.hpp
// │       ├── CPU_Time.hpp
// │       ├── CPU_SIMD.hpp (AVX2,AVX512,SSE4.2)
// │       ├── CPU_BMI.hpp (BMI1, BMI2)
// │       ├── CPU_FMA.hpp
// │       ├── CPU_GatherScatter.hpp
// │       └── CPU_Matrix.hpp (AMX) 
// ├── Memory/
// │   └── Memory.hpp
// │       ├── Allocator/
// │       │   ├── SystemAllocator.hpp
// │       │   ├── VirtualAllocator.hpp
// │       │   ├── NUMAAllocator.hpp
// │       │   ├── SlabAllocator.hpp
// │       │   ├── ArenaAllocator.hpp
// │       │   ├── StackAllocator.hpp
// │       │   ├── PoolAllocator.hpp
// │       │   └── CombinedAllocator.hpp
// │       ├── MemoryLogger.hpp
// │       └── Heap.hpp 
// │
// ShardObjects.hpp
// │
// Macros.hpp
// │
// BluBooster_Utils.hpp
//========================================================================================                          

// Compile OS Branch
#if defined(_WIN32) || defined(_WIN64)
#define BLUBOOSTER_OS_WINDOWS
#elif defined(__linux__)
#define BLUBOOSTER_OS_LINUX
#endif
#include "CPU.hpp"
#include "Memory.hpp"
#include "SharedObjects.hpp"
#include "BluBooster_Utils.hpp"

#include "AegisHolderGuard.hpp"
#include "AegisPtrBaseHolder.hpp"
#endif // BLUBOOSTER_HPP
