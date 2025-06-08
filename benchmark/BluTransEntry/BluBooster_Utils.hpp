#ifndef BLUBOOSTER_UTILS_HPP
#define BLUBOOSTER_UTILS_HPP

#include <type_traits>
#include <bit>
#include <cstdint>
#include <cstring>
namespace BluBooster::Utils {
    namespace Internal {
        template <typename To, typename From>
        [[nodiscard]] constexpr To bitwise_cast_constexpr(const From& from) noexcept {
            return std::bit_cast<To>(from);
        }
        template <typename To, typename From>
        [[nodiscard]] constexpr To constexpr_memcopy(const From& from) noexcept {
            To res;
            constexpr size_t copy_size = (sizeof(To) < sizeof(From)) ? sizeof(To) : sizeof(From);
            uint8_t* resScan = reinterpret_cast<uint8_t*>(&res);
            const uint8_t* fromScan = reinterpret_cast<uint8_t*>(&from);
            for(size_t i = 0; i < copy_size; ++i)
                resScan[i] = fromScan[i];
            return res;
        }
        template <typename To, typename From>
        [[nodiscard]] To bitwise_cast_fallback(const From& from) noexcept {
            constexpr size_t copy_size = (sizeof(To) < sizeof(From)) ? sizeof(To) : sizeof(From);
            To result;
            std::memcpy(&result,&from,copy_size);
            return result;
        }
    }

    template <typename To,typename From>
    [[nodiscard]] constexpr To bitwise_cast(From& from) noexcept{
        if constexpr (std::is_constant_evaluated()){
            if constexpr (sizeof(To) == sizeof(From) && std::is_trivially_copyable_v<From>
                      && std::is_trivially_copyable_v<To>)
                    return Internal::bitwise_cast_constexpr<To,From>(from);
            else
                return Internal::constexpr_memcopy<To,From>(from);
        }
        return Internal::bitwise_cast_fallback<To,From>(from);
    }
}
#endif // BLUBOOSTER_UTILS_HPP
