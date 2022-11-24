#pragma once

#include <type_traits>
#include <cstdint>
#include <utility>
#include <string_view>
#include <limits>

constexpr std::size_t min(std::size_t a, std::size_t b) {
    return a < b ? a : b;
}

template <class Enum, std::size_t MAXN = 512>
	requires std::is_enum_v<Enum>
struct EnumeratorTraits {
    using UnderlyingType = std::underlying_type_t<Enum>;
    
    static constexpr std::size_t size() noexcept {
        constexpr auto high = min( static_cast<std::size_t>(std::numeric_limits<UnderlyingType>::max()), MAXN);
        constexpr auto low  = min(-static_cast<std::size_t>(std::numeric_limits<UnderlyingType>::min()), MAXN);
        constexpr auto highHalved = high / 2;
        constexpr auto lowHalved  = low  / 2;
        
        std::size_t zeroIsValid = isValid_<(Enum)0>();
        std::size_t sz = zeroIsValid;
        if constexpr (highHalved != 0) sz += size_< 1, 1>(std::make_index_sequence<highHalved>{});
        if constexpr (lowHalved  != 0) sz += size_<-1, 1>(std::make_index_sequence< lowHalved>{});
        if constexpr (high != 0) sz += size_< 1, highHalved + 1>(std::make_index_sequence<high - highHalved>{});
        if constexpr (low  != 0) sz += size_<-1,  lowHalved + 1>(std::make_index_sequence< low -  lowHalved>{});
        return sz;
    }
//    static constexpr Enum at(std::size_t i) noexcept;
//    static constexpr std::string_view nameAt(std::size_t i) noexcept;

private:
    template <char sign, std::size_t offset, std::size_t... ints>
    static constexpr std::size_t size_(std::integer_sequence<std::size_t, ints...>) noexcept {
        // works fine when compiled with clang without flags only if sizeof...(ints) is no more than 256
        return (isValid_<(Enum)(sign * (ints + offset))>() + ...);
    }

    template <Enum V>
    static constexpr short isValid_() {
        constexpr const std::string_view name(__PRETTY_FUNCTION__);
        constexpr auto i = name.find("V = ");
        if constexpr (name[i + 4] == '(') {
            return 0;
        }
        return 1;
    }
};
