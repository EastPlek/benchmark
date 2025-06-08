#ifndef CPU_BRANCHFINDER_HPP
#define CPU_BRANCHFINDER_HPP

#include <array>
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__)
#include <cpuid.h>
#include <x86intrin.h>
#include <cstdint>
#endif

//========================================================================================
// TITLE     : BluBooster - CPU Features Finder Macro
// CREATED   : 2025-06-06
// AUTHOR    : EastPlek
//========================================================================================
// Description
// ---------------------------------------------------------------------------------------
// Automatically finds CPU Insturction sets in runtime.
//========================================================================================

namespace BluBooster::CPU {

    #if defined(_M_X64) || defined(__x86_64__)
    inline constexpr bool isX64 = true;
        #if defined(BLUBOOSTER_HAS_AVX2)
            #pragma message("AVX2 Enabled.")
            inline constexpr bool hasAVX2  = true;
        #else
            inline constexpr bool hasAVX2 = false;
        #endif

        #if defined(BLUBOOSTER_HAS_AVX512F)
            #pragma message("AVX512 Enabled.")
            inline constexpr bool hasAVX512 = true;
        #else
            inline constexpr bool hasAVX512 = false;
        #endif

        #if defined(BLUBOOSTER_HAS_SSE42)
            #pragma message("SSE4.2 Enabled.")
            inline constexpr bool hasSSE42 = true;
        #else
            inline constexpr bool hasSSE42 = false;
        #endif

        #if defined(BLUBOOSTER_HAS_BMI2)
            #pragma message("BMI Enabled.")
            inline constexpr bool hasBMI = true;
        #else
            inline constexpr bool hasBMI = false;
        #endif

        #if defined(__AMX_TILE__) && defined(__AMX_INT8__) && defined(__AMX_BF16__)
            #pragma message("AMX Enabled.")
            inline constexpr bool hasAMX = true;
        #else
            inline constexpr bool hasAMX = false;
    #endif

    #else
    inline constexpr bool isX64 = false;
    #endif
    #if defined(__aarch64__)
    inline constexpr bool isARM = true;
    #else
    inline constexpr bool isARM = false;
    #endif
    enum class RuntimeFeatureLevel {
        NONE,
    #if defined(_M_X64) || defined(__x86_64__)
        SSE42,
        POPCNT, // SSE4.2 + POPCNT
        BMI2,
        AVX2,
        AVX512,
        AMX,
    #elif defined (__aarch64__)
        NEON,
        ARMV8,
        A64_POPCNT,
        A64_BITOPS
    #endif
        END
    };
    struct RuntimeCPUFeatures {
    #if defined(_M_X64) || defined(__x86_64__)
        bool hasAVX512F = false;
        bool hasAVX2 = false;
        bool hasSSE42 = false;
        bool hasBMI2 = false;
        bool hasAMX = false;
        bool hasPOPCNT = false;

        bool usableAVX512F = false;
        bool usableAMX = false;
    #elif defined (__aarch64__)
        bool hasNEON = false;
        bool hasPOPCNT = false;
        bool hasBITOPS = false;
        bool hasASIMD = false;
        bool hasSVE = false;
        bool hasDotP = false;
        bool hasFP16 = false;

        bool usableSVE = false;
    #endif
    };
    inline RuntimeCPUFeatures RuntimeCPUFeatureCheckX64() {
        RuntimeCPUFeatures features{};
        std::array<int,4> info{};
        // Detect SSE4.2 Feature
        #if defined(_MSC_VER)
            __cpuid(info.data(), 1);
        #else
            __cpuid(1,info[0],info[1],info[2],info[3]);
        #endif
        features.hasSSE42 = (info[2] & (1 << 20)) != 0;

        // Detect AVX2 and BMI Feature
        #if defined(_MSC_VER)
            __cpuidex(info.data(),7,0);
            uint64_t xcr0 = _xgetbv(0);
        #else
            __cpuid_count(7,0,info[0],info[1],info[2],info[3]);
            uint32_t eax, edx;
            __asm__ volatile (
                "xgetbv"
                : "=a"(eax), "=d"(edx)
                : "c"(0)
                );
            uint64_t xcr0 = (static_cast<uint64_t>(edx) << 32) | eax;
        #endif
        // BMI2
        features.hasBMI2 = (info[1] & (1 << 8)) != 0;
        features.hasAVX2 = (info[1] & (1 << 5)) != 0;
        features.hasAVX512F = (info[1] & (1 << 16)) != 0;
        features.usableAVX512F = features.hasAVX512F && (xcr0 & 0xE6) == 0xE6;
        features.hasPOPCNT = (info[2] & (1 << 23)) != 0;

        features.hasAMX = (info[3] & (1 << 24)) != 0;
        features.usableAMX = features.hasAMX && ((xcr0 >> 17) & 0x3) == 0x3;
        return features;
    }
    inline RuntimeCPUFeatures RuntimeCPUFeatureCheckARM() {
        // TODO : Implement Runtime ARM Check.
        RuntimeCPUFeatures features{};
        return features;
    }
    inline RuntimeCPUFeatures RuntimeCPUFeatureCheck() {
        if constexpr(isX64)
            return RuntimeCPUFeatureCheckX64();
        else if constexpr(isARM)
            return RuntimeCPUFeatureCheckARM();
    }
    inline RuntimeFeatureLevel RuntimeCPUFeatureLevelX64(const RuntimeCPUFeatures& features) {
        if (features.hasAMX) return RuntimeFeatureLevel::AMX;
        if(features.hasAVX512F) return RuntimeFeatureLevel::AVX512;
        if(features.hasAVX2) return RuntimeFeatureLevel::AVX2;
        if(features.hasBMI2) return RuntimeFeatureLevel::BMI2;
        if(features.hasPOPCNT) return RuntimeFeatureLevel::POPCNT;
        if(features.hasSSE42) return RuntimeFeatureLevel::SSE42;
        return RuntimeFeatureLevel::NONE;
    }
    inline RuntimeFeatureLevel RuntimeCPUFeatureLevelARM(const RuntimeCPUFeatures& features) {
        //
        return RuntimeFeatureLevel::NONE;
    }
    inline RuntimeFeatureLevel RuntimeCPUFeatureLevel(const RuntimeCPUFeatures& features) {
        if constexpr (isX64)
            return RuntimeCPUFeatureLevelX64(features);
        else if constexpr(isARM)
            return RuntimeCPUFeatureLevelARM(features);
    }
}

#endif // CPU_BRANCHFINDER_HPP
