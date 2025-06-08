#ifndef CPU_BITS_HPP
#define CPU_BITS_HPP

#include "Macros.hpp"
#include "CPU_Features.hpp"
#include "BluBooster_Utils.hpp"

#if defined(_MSC_VER)
#elif defined(__GNUC__)
#include <immintrin.h>
#endif

//========================================================================================
// TITLE     : BluBooster - CPU Bits Module
// CREATED   : 2025-06-06
// AUTHOR    : EastPlek
//========================================================================================
// Description
// ----------------------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------------------
//
//========================================================================================

namespace BluBooster::CPU::Bits {
    namespace Internal{
        constexpr int hamming_weight32(unsigned int biteset) noexcept;
        constexpr int hamming_weight64(uint64_t bitset) noexcept;
    }
    BLUBOOSTER_FORCE_INLINE constexpr int Internal::hamming_weight32(unsigned int bitset) noexcept {
        bitset -= (bitset >> 1) & 0x55555555;
        bitset = (bitset & 0x33333333) + ((bitset >> 2) & 0x33333333);
        bitset = (bitset + (bitset >> 4)) & 0x0f0f0f0f;
        return (bitset * 0x01010101) >> 24;
    }
    template<typename T>
    BLUBOOSTER_FORCE_INLINE int count32(const T& type) noexcept requires (sizeof(T) <= 4) {
    #if defined(_MSC_VER)
        return __popcnt(Utils::bitwise_cast<unsigned int>(type));
    #elif defined(__GNUC__)
        return __builtin_popcount(Utils::bitwise_cast<unsigned int>(type));
    #else
    #endif
    }
    BLUBOOSTER_FORCE_INLINE constexpr int Internal::hamming_weight64(uint64_t bitset) noexcept {
        bitset -= (bitset >> 1) & 0x5555555555555555;
        bitset = (bitset & 0x3333333333333333) + ((bitset >> 2) & 0x3333333333333333);
        bitset = (bitset + (bitset >> 4)) & 0x0f0f0f0f0f0f0f0f;
        return static_cast<int>((bitset * 0x0101010101010101) >> 56);
    }
    template<typename T>
    BLUBOOSTER_FORCE_INLINE int count64(const T& type) noexcept requires (sizeof(T) <= 8) {
        #if defined (_MSC_VER)
            return __popcnt64(Utils::bitwise_cast<unsigned __int64>(type));
        #elif defined(__GNUC__)
            return __builtin_popcountll(Utils::bitwise_cast<unsigned long long>(type));
        #endif
    }

    template <typename T>
    BLUBOOSTER_FORCE_INLINE int count_runtime(const T& type) noexcept requires (sizeof(T) <= 8) {
        if constexpr (sizeof(T) > 4)
            return bits64<T>(type);
        else
            return bits32<T>(type);
    }
    template <typename T>
    BLUBOOSTER_FORCE_INLINE constexpr int count_constexpr(const T& type) noexcept requires (sizeof(T) <= 8) {
        if constexpr (sizeof(T) > 4)
            return Internal::hamming_weight64(Utils::bitwise_cast<uint64_t>(type));
        else
            return Internal::hamming_weight32(Utils::bitwise_cast<unsigned int>(type));
    }

    template <typename T>
    BLUBOOSTER_FORCE_INLINE constexpr int count(const T& type) noexcept requires (sizeof(T) <= 8) {
        if constexpr(std::is_constant_evaluated())
            return count_constexpr(type);
        return count_runtime(type);
    }

    template <typename T>
    BLUBOOSTER_FORCE_INLINE int trailing_zero32(const T& type) noexcept requires (sizeof(T) <= 4) {
    #if defined (_MSC_VER)
        return static_cast<int>(_mm_tzcnt_u32(Utils::bitwise_cast<unsigned int>(type)));
    #elif defined (__GNUC__)
        return __builtin_ctz(Utils::bitwise_cast<unsigned int>(type));
    #endif
    }

    constexpr int trailing_zero32_constexpr(unsigned int bitset) noexcept {
        //return (bitset == 0) ? 32 : Internal::hamming_weight32((bitset & -bitset) - 1);
        return 0;
    }

    template <typename T>
    BLUBOOSTER_FORCE_INLINE int trailing_zero64(const T& type) noexcept requires (sizeof(T) <= 8) {
        #if defined (_MSC_VER)
        return static_cast<int>(_mm_tzcnt_u64(Utils::bitwise_cast<unsigned int>(type)));
        #elif defined (__GNUC__)
        return __builtin_ctzll(Utils::bitwise_cast<unsigned long long>(type));
        #endif
    }
    constexpr int trailing_zero64_constexpr(uint64_t bitset) noexcept {
        //return (bitset == 0) ? 64 : Internal::hamming_weight64((bitset & -bitset) - 1);
        return 0;
    }

    template <typename T>
    BLUBOOSTER_FORCE_INLINE int trailing_zero_runtime(const T& type) requires (sizeof(T) <= 8) {
        if constexpr (sizeof(T) > 4)
            return trailing_zero64(type);
        else
            return trailing_zero32(type);
    }

    template <typename T>
    BLUBOOSTER_FORCE_INLINE constexpr int trailing_zero_constexpr(const T& type) requires (sizeof(T) <= 8) {
        if constexpr (sizeof(T) > 4)
            return trailing_zero64_constexpr(Utils::bitwise_cast<uint64_t>(type));
        else
            return trailing_zero32_constexpr(Utils::bitwise_cast<unsigned int>(type));
    }

    template <typename T>
    BLUBOOSTER_FORCE_INLINE constexpr int trailing_zero(const T& type) requires (sizeof(T) <= 8) {
        if (std::is_constant_evaluated())
            return trailing_zero_runtime(type);
        else
            return trailing_zero_constexpr(type);
    }

    BLUBOOSTER_FORCE_INLINE constexpr uint32_t reverse_bit32(uint32_t bitset) noexcept {
        bitset = ((bitset >> 1) & 0x55555555u) | ((bitset & 0x55555555u) << 1);
        bitset = ((bitset >> 2) & 0x33333333u) | ((bitset & 0x33333333u) << 2);
        bitset = ((bitset >> 4) & 0x0f0f0f0fu) | ((bitset & 0x0f0f0f0fu) << 4);
        bitset = ((bitset >> 8) & 0x00ff00ffu) | ((bitset & 0x00ff00ffu) << 8);
        bitset = ( bitset >> 16 )              | ( bitset               << 16);
        return bitset;
    }
    BLUBOOSTER_FORCE_INLINE constexpr uint64_t reverse_bit64(uint64_t bitset) noexcept {
        bitset = ((bitset >> 1) & 0x5555555555555555ULL) | ((bitset & 0x5555555555555555ULL) << 1);
        bitset = ((bitset >> 2) & 0x3333333333333333ULL) | ((bitset & 0x3333333333333333ULL) << 2);
        bitset = ((bitset >> 4) & 0x0f0f0f0f0f0f0f0fULL) | ((bitset & 0x0f0f0f0f0f0f0f0fULL) << 4);
        bitset = ((bitset >> 8) & 0x00ff00ff00ff00ffULL) | ((bitset & 0x00ff00ff00ff00ffULL) << 8);
        bitset = ((bitset >> 16) & 0x0000ffff0000ffffULL)| ((bitset & 0x0000ffff0000ffffULL) << 16);
        bitset = ( bitset >> 32 )                        | ( bitset                          << 32);
        return bitset;
    }

    template <typename T>
    BLUBOOSTER_FORCE_INLINE int leading_zero32(const T& type) noexcept requires (sizeof(T) <= 4) {
        #if defined(_MSC_VER)
            return __lzcnt(Utils::bitwise_cast<unsigned int>(type));
        #elif defined(__GNUC__)
            return __builtin_clz(Utils::bitwise_cast<unsigned int>(type));
        #endif
    }

    BLUBOOSTER_FORCE_INLINE constexpr int leading_zero32_constexpr(uint32_t bit) noexcept {
        return trailing_zero32_constexpr(reverse_bit32(bit));
    }

    template<typename T>
    BLUBOOSTER_FORCE_INLINE int leading_zero64(const T& type) noexcept requires (sizeof(T) <= 8) {
        #if defined(_MSC_VER)
            return __lzcnt64(Utils::bitwise_cast<unsigned __int64>(type));
        #elif defined(__GNUC__)
            return __builtin_clzll(Utils::bitwise_cast<unsigned long long>(type));
        #endif
    }

    BLUBOOSTER_FORCE_INLINE constexpr int leading_zero64_constexpr(uint64_t bit) noexcept {
        return trailing_zero64_constexpr(reverse_bit64(bit));
    }

    template <typename T>
    BLUBOOSTER_FORCE_INLINE int leading_zero_runtime(const T& type)noexcept requires (sizeof(T) <= 8) {
        if constexpr (sizeof(T) > 4)
            return leading_zero64<T>(type);
        else
            return leading_zero32<T>(type);
    }

    template <typename T>
    BLUBOOSTER_FORCE_INLINE constexpr int leading_zero_constexpr(const T& type) noexcept requires (sizeof(T) <= 8) {
        if constexpr (sizeof(T) > 4)
            return leading_zero64_constexpr(Utils::bitwise_cast<uint64_t>(type));
        else
            return leading_zero32_constexpr(Utils::bitwise_cast<uint32_t>(type));
    }

    template <typename T>
    BLUBOOSTER_FORCE_INLINE constexpr int leading_zero(const T& type) noexcept requires (sizeof(T) <= 8) {
        if constexpr (std::is_constant_evaluated())
            return leading_zero_constexpr(type);
        else
            return leading_zero_runtime(type);
    }

}


#endif // CPU_BITS_HPP
