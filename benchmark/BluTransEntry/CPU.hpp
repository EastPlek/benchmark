#ifndef CPU_HPP
#define CPU_HPP
//========================================================================================
// TITLE     : BluBooster - CPU Module
// CREATED   : 2025-06-05
// AUTHOR    : EastPlek
//========================================================================================
// CPU Module Overview (Simplified Directory Tree)
// ---------------------------------------------------------------------------------------
// BluBooster.hpp
// CPU/
// └── CPU.hpp
//     ├── CPU_Features.hpp
//     ├── CPU_Sync.hpp
//     ├── CPU_Bits.hpp
//     ├── CPU_Cache.hpp (SSE2)
//     ├── CPU_Info.hpp
//     ├── CPU_Time.hpp
//     ├── CPU_SIMD.hpp (AVX2,AVX512,SSE4.2)
//     ├── CPU_BMI.hpp (BMI1, BMI2)
//     ├── CPU_FMA.hpp
//     ├── CPU_GatherScatter.hpp
//     └── CPU_Matrix.hpp (AMX)  
//========================================================================================
#if !defined(_M_X64) && !defined(__x86_64__) && !defined(__aarch64__)
    #pragma message("CPU Module Supports only x64 and ARM64.")
    #error "Unsupported CPU."
#endif
#include "CPU_Features.hpp"
#include "CPU_Sync.hpp"
#include "CPU_Bits.hpp"

#endif // CPU_HPP
