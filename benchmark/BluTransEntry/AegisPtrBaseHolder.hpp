#ifndef CMANAGERBASEPTR_HPP
#define CMANAGERBASEPTR_HPP
#include <array>
#include <atomic>
#include <cstdint>
#include "CPU.hpp"
#include "SharedObjects.hpp"
#include "Macros.hpp"

namespace BluBooster::Concurrent::AegisPtr::Internal{
    template <size_t MAX_THREADS>
    struct alignas(64) AegisPtrBaseHolderFlags {

        AegisPtrBaseHolderFlags():  bits {}{

        }
        AegisPtrBaseHolderFlags(const AegisPtrBaseHolderFlags&) = delete;
        AegisPtrBaseHolderFlags& operator=(const AegisPtrBaseHolderFlags&) = delete;

        AegisPtrBaseHolderFlags(AegisPtrBaseHolderFlags&& other) {
            for (size_t i = 0; i < bits.size(); ++i)
                bits[i].store(other.bits[i].load(std::memory_order_relaxed));
        }
        AegisPtrBaseHolderFlags& operator= (AegisPtrBaseHolderFlags&& other) {
            for (size_t i = 0; i < bits.size(); ++i)
                bits[i].store(other.bits[i].load(std::memory_order_relaxed));
            return *this;
        }
        #if defined(BLUBOOSTER_AEGISPTR_SAFE)
            using BitType = std::atomic<uint64_t>;
        #else
            using BitType = uint64_t;
        #endif
        std::array<BitType,(MAX_THREADS + 63)/ 64> bits;
        BLUBOOSTER_FORCE_INLINE void Set(size_t tid) {
            #if defined (BLUBOOSTER_AEGISPTR_SAFE)
            bits[tid / 64].fetch_or(1ULL << (tid % 64),std::memory_order_seq_cst);
            #else
            bits[tid / 64] |= (1ULL << (tid % 64));
            BluBooster::CPU::Sync::write_mbarrier();
            #endif
        }
        BLUBOOSTER_FORCE_INLINE void Unset(size_t tid) {
        #if defined (BLUBOOSTER_AEGISPTR_SAFE)
            bits[tid / 64].fetch_and(~(1ULL << (tid % 64)),std::memory_order_seq_cst);
        #else
            BluBooster::CPU::Sync::read_mbarrier();
            bits[tid / 64] &= ~(1ULL << (tid % 64));
        #endif
        }
        BLUBOOSTER_FORCE_INLINE bool CanDestroy() const {
            size_t bits_size = bits.size();
            size_t i = 0;
            #if defined(BLUBOOSTER_AEGISPTR_SAFE)
            std::array<uint64_t, (MAX_THREADS + 63) / 64> rawCopy;
            for (size_t i = 0; i < rawCopy.size(); ++i)
                rawCopy[i] = bits[i].load(std::memory_order_relaxed);
            auto* rawBits = rawCopy.data();
            #else
            auto* rawBits = reinterpret_cast<const uint64_t*>(&bits[0]);
            #endif
            if constexpr(BluBooster::CPU::hasAVX512) {
                if (BluBooster::cpuFeatures.hasAVX512F) {
                    while(i + 8 <= bits_size){
                        __m512i chunk = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(&rawBits[i]));
                        __mmask64 mask = _mm512_test_epi8_mask(chunk,_mm512_setzero_si512());
                        if(mask != 0) return false;
                        i += 8;
                    }
                }
            }
            if constexpr(BluBooster::CPU::hasAVX2) {
                if(BluBooster::cpuFeatures.hasAVX2){
                    while(i + 4 <= bits_size) {
                        __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&rawBits[i]));
                        __m256i cnt = _mm256_cmpeq_epi8(chunk,_mm256_setzero_si256());
                        int mask = _mm256_movemask_epi8(cnt);
                        if(mask != -1) return false;
                        i += 4;
                    }
                }
            }
            if constexpr(BluBooster::CPU::hasSSE42) {
                if(BluBooster::cpuFeatures.hasSSE42){
                    while(i + 2 <= bits_size) {
                        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&rawBits[i]));
                        __m128i cmp = _mm_cmpeq_epi8(chunk,_mm_setzero_si128());
                        int mask = _mm_movemask_epi8(cmp);
                        if(mask != 0xFFFF) return false;
                        i += 2;
                    }
                }
            }
            for(; i < bits.size(); ++i)
                if(rawBits[i] != 0) return false;
            return true;
        }
    };
    template <typename T,size_t MAX_THREADS>
    struct alignas(64) AegisPtrBaseHolderSlot{
        T* ptr{nullptr};
        AegisPtrBaseHolderFlags<MAX_THREADS> flags;
    };

    template <typename T, size_t N>
    struct AegisPtrBaseHolder;

    template <typename U>
    struct is_aegis_ptr_base_holder : std::false_type{};

    template <typename U,size_t N>
    struct is_aegis_ptr_base_holder<AegisPtrBaseHolder<U,N>> : std::true_type{};

    template <typename U>
    inline constexpr bool is_aegis_ptr_base_holder_v = is_aegis_ptr_base_holder<std::remove_cv_t<U>>::value;
    template <typename T,size_t MAX_THREADS = 16>
    struct AegisPtrBaseHolder{
        static_assert(!is_aegis_ptr_base_holder_v<T>,"T must not be itself AegisPtrBaseHolder.");
        static_assert(MAX_THREADS % 8 == 0,"Threads Must be divided by 8. ");
        AegisPtrBaseHolder() {
            m_base.ptr = nullptr;
        }
        AegisPtrBaseHolder(const T& _rawData) {
            m_base.ptr = BluBooster::Alloc<T>(_rawData);
        }
        AegisPtrBaseHolder(T&& _mvRawData) {
            m_base.ptr = BluBooster::Alloc<T>(std::move(_mvRawData));
        }
        AegisPtrBaseHolder(const AegisPtrBaseHolder<T, MAX_THREADS>& other) = delete;
        AegisPtrBaseHolder<T,MAX_THREADS>& operator= (const AegisPtrBaseHolder<T,MAX_THREADS>& other) = delete;
        AegisPtrBaseHolder(AegisPtrBaseHolder&& other) noexcept {
            assert(m_base.ptr == nullptr); // 이동 전 객체는 비어있어야
            m_base.ptr = std::exchange(other.m_base.ptr, nullptr); // 소유권 이전
            m_base.flags = std::move(other.m_base.flags);
        }
        AegisPtrBaseHolder& operator=(AegisPtrBaseHolder<T,MAX_THREADS>&& other) = delete;
        
        ~AegisPtrBaseHolder() {
            if(m_base.ptr && m_base.flags.CanDestroy())
            {
                BluBooster::Memory::SystemAllocator::Free(m_base.ptr);
                m_base.ptr = nullptr;
            }
        }
        AegisPtrBaseHolderSlot<T,MAX_THREADS> m_base{};
    };
}

#endif // CMANAGERBASEPTR_HPP
