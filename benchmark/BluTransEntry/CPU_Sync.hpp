#ifndef CPU_SYNC_HPP
#define CPU_SYNC_HPP

#include "CPU_Features.hpp"
#include "Macros.hpp"

//========================================================================================
// TITLE     : BluBooster - CPU Sync Module
// CREATED   : 2025-06-06
// AUTHOR    : EastPlek
//========================================================================================
// Description
// ---------------------------------------------------------------------------------------
// This Module provides basic synchronization CPU intrinsic wrapper functions.
// ---------------------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------------------
// * pause()
// input : void (none)
// output : void (none)
// Description :
//      issues a processor-level pause / yield hint.
//      x86 : pause, arm : "yield"
//      Helps reducing power consumption and contention during busy-wait loops.
//
// * write_mbarrier()
// input : void (none)
// output : void (none)
// Description :
//      Ensures that all previous store operations are globally visible
//      before any subsequent stores. Maps to SFENCE (x86) or DMB ISHST (ARM)
//
// * read_mbarrier()
// input : void
// output : void
// Description :
//      Ensures that all previous load operations are globally visible
//      before any subsequent loads. Maps to LFENCE (x86) or DMB ISHLD (ARM)
//
// * full_mbarrier()
// input : void
// output : void
// Description :
//      Acts as a full memory barrier, enforcing total ordering of
//      both loads and stores across cores. Maps to MFENCE(x86) or DMB ISH (ARM)
//
//========================================================================================

namespace BluBooster::CPU::Sync {
    BLUBOOSTER_FORCE_INLINE void pause() noexcept {
        if constexpr(isX64) {
            _mm_pause();
        }
        else if constexpr (isARM) {
        #if defined(__GNUC__) || defined (__clang__)
            __asm__ __volatile__("yield" ::: "memory");
        #endif
        }
    }
    BLUBOOSTER_FORCE_INLINE void write_mbarrier() noexcept {
        if constexpr (isX64) {
            _mm_sfence();
        } else if constexpr (isARM) {
        #if defined(__GNUC__) || defined (__clang__)
            __asm__ __volatile__("dmb ishst" ::: "memory");
         #endif
        }
    }
    BLUBOOSTER_FORCE_INLINE void read_mbarrier() noexcept {
        if constexpr (isX64) {
            _mm_lfence();
        } else if constexpr (isARM) {
        #if defined(__GNUC__) || defined (__clang__)
            __asm__ __volatile__("dmb ishld" ::: "memory");
        #endif
        }
    }
    BLUBOOSTER_FORCE_INLINE void full_mbarrier() noexcept {
        if constexpr (isX64) {
            _mm_mfence();
        } else if constexpr (isARM) {
        #if defined(__GNUC__) || defined (__clang__)
            __asm__ __volatile__("dmb ish" ::: "memory");
        #endif
        }
    }
}
#endif // CPU_SYNC_HPP
