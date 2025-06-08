#ifndef MACROS_HPP
#define MACROS_HPP

#if defined (__GNUC__) || defined(__clang__)
#define BLUBOOSTER_UNROLL _Pragma("GCC Unroll 8")
#elif defined(_MSC_VER)
#define BLUBOOSTER_UNROLL __pragma(loop(hint_parallel(8)))
#else
#define BLUBOOSTER_UNROLL
#endif

#if defined(__GNUC__) || defined(__clang__)
#define BLUBOOSTER_FORCE_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define BLUBOOSTER_FORCE_INLINE __forceinline
#endif

#endif // MACROS_HPP
