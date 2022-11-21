#pragma once

#include <type_traits>
#include <cstdint>
#include <utility>
#include <string>
#include <limits>
#include <algorithm>

template <class Enum, std::size_t MAXN = 512>
	requires std::is_enum_v<Enum>
struct EnumeratorTraits {
    using UnderlyingType = std::underlying_type_t<Enum>;
    
    static constexpr std::size_t size() noexcept {
        constexpr std::size_t high = std::min(static_cast<std::size_t>( std::numeric_limits<UnderlyingType>::max()), MAXN);
        constexpr std::size_t low  = std::min(static_cast<std::size_t>(-std::numeric_limits<UnderlyingType>::min()), MAXN);
        std::size_t zeroIsValid = isValid_<(Enum)0>();
        std::size_t sz = zeroIsValid;
        if constexpr (high != 0) sz += size_< 1>(std::make_index_sequence<high + 1>{}) - zeroIsValid;
        if constexpr (low  != 0) sz += size_<-1>(std::make_index_sequence< low + 1>{}) - zeroIsValid;
        return sz;
    }
//    static constexpr Enum at(std::size_t i) noexcept;
//    static constexpr std::string_view nameAt(std::size_t i) noexcept;

private:
    template <char sign, std::size_t... ints>
    static constexpr std::size_t size_(std::integer_sequence<std::size_t, ints...>) noexcept {
        return (isValid_<(Enum)(sign * ints)>() + ...);
    }

    template <Enum V>
    static constexpr short isValid_() {
        const std::string_view name = __PRETTY_FUNCTION__;
        const std::string_view pattern = "with Enum V = ";
        std::size_t nameLen = name.length();
        std::size_t patternLen = pattern.length();
        std::size_t i = 0;
        do ++i;
        while (i + patternLen <= nameLen && name.substr(i, patternLen) != pattern);
        if (name[i + patternLen] == '(') {
            return 0;
        }
        return 1;
    }
};
