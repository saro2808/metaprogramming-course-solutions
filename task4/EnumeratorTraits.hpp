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
        constexpr auto positives = signedSize_<1, high_()>();
        constexpr auto negatives = signedSize_<-1, low_()>();
        return isValid_<(Enum)0>() + positives.first + positives.second
                                   + negatives.first + negatives.second;
    }
    /*
    static constexpr Enum at(std::size_t i) noexcept {
        constexpr auto negativesSize = signedSize_<-1, low_()>();
        constexpr auto positivesSize = signedSize_<1, high_()>();
        constexpr auto zeroIsValid = isValid_<(Enum)0>();
        if constexpr (i < negativesSize)
    }*/

//    static constexpr std::string_view nameAt(std::size_t i) noexcept;

private:
    static constexpr std::size_t high_() {
        return min( static_cast<std::size_t>(std::numeric_limits<UnderlyingType>::max()), MAXN);
    }

    static constexpr std::size_t low_() {
        return min(-static_cast<std::size_t>(std::numeric_limits<UnderlyingType>::min()), MAXN);
    }

    template<signed char sign, std::size_t max>
    static constexpr auto signedSize_() noexcept {
        constexpr auto maxHalved = max / 2;
        constexpr auto sz0 = /*(maxHalved == 0) ? 0 :*/ size_<sign,             1>(std::make_index_sequence<      maxHalved>{});
        constexpr auto sz1 = /*(max       == 0) ? 0 :*/ size_<sign, maxHalved + 1>(std::make_index_sequence<max - maxHalved>{});
        return std::make_pair(sz0, sz1);
    }
    
    template <signed char sign, std::size_t offset, std::size_t... ints>
    static constexpr std::size_t size_(std::integer_sequence<std::size_t, ints...>) noexcept {
        // works fine when compiled with clang without flags only if sizeof...(ints) doesn't exceed 256
        if constexpr (sizeof...(ints) == 0)
            return 0;
        else
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
