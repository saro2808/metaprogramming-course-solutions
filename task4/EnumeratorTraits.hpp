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
    static constexpr std::size_t size() noexcept {
        constexpr auto positives = signedSize_<1, high_()>();
        constexpr auto negatives = signedSize_<-1, low_()>();
        return zeroIsValid_() + positives.first + positives.second
                              + negatives.first + negatives.second;
    }
    
    static constexpr Enum at(const std::size_t i) noexcept {
        return static_cast<Enum>(nameValueAt_<int>(i));
    }

    static constexpr std::string_view nameAt(const std::size_t i) noexcept {
        return nameValueAt_<std::string_view>(i);
    }

private:
    using UnderlyingType = std::underlying_type_t<Enum>;
    
    static constexpr char zeroIsValid_() {
        return isValid_<static_cast<Enum>(0)>();
    }

    template<typename Ret>
    static constexpr Ret nameValueAt_(const std::size_t i) noexcept {
        constexpr auto negativesSizes = signedSize_<-1, low_()>();
        constexpr auto positivesSizes = signedSize_<1, high_()>();
        constexpr auto negativesSize = negativesSizes.first + negativesSizes.second;
        constexpr auto positivesSize = positivesSizes.first + positivesSizes.second;
        constexpr auto zeroIsValid = zeroIsValid_();

        if (i < negativesSize) {
            if (i < negativesSizes.second)
                return getNameValueAt_<-1, low_()/2, negativesSizes.second, low_() - low_()/2, Ret>(i);
            return getNameValueAt_<-1, 0, negativesSizes.first, low_()/2, Ret>(i - negativesSizes.second);
        }
        if constexpr (zeroIsValid) {
            if (i == negativesSize) {
                if constexpr (std::is_same_v<Ret, int>)
                    return 0;
                else
                    return nameOf_<static_cast<Enum>(0)>();
            }
        }
        if (i < negativesSize + zeroIsValid + positivesSizes.first)
            return getNameValueAt_<1, 0, positivesSizes.first, high_()/2, Ret>(i - zeroIsValid - negativesSize);
        return getNameValueAt_<1, high_()/2, positivesSizes.second, high_() - high_()/2, Ret>(i - zeroIsValid - negativesSize - positivesSizes.first);
    }

    static constexpr std::size_t high_() {
        return min( static_cast<std::size_t>(std::numeric_limits<UnderlyingType>::max()), MAXN);
    }

    static constexpr std::size_t low_() {
        return min(-static_cast<std::size_t>(std::numeric_limits<UnderlyingType>::min()), MAXN);
    }

    template<signed char sign, std::size_t max>
    static constexpr auto signedSize_() noexcept {
        constexpr auto maxHalved = max / 2;
        constexpr auto sz0 = size_<sign,             1>(std::make_index_sequence<      maxHalved>{});
        constexpr auto sz1 = size_<sign, maxHalved + 1>(std::make_index_sequence<max - maxHalved>{});
        return std::make_pair(sz0, sz1);
    }
    
    template <signed char sign, std::size_t offset, std::size_t... ints>
    static constexpr std::size_t size_(std::integer_sequence<std::size_t, ints...>) noexcept {
        // works fine when compiled with clang without flags only if sizeof...(ints) doesn't exceed 256
        if constexpr (sizeof...(ints) == 0)
            return 0;
        else
            return (isValid_<static_cast<Enum>(sign * (ints + offset))>() + ...);
    }

    template <Enum V>
    static constexpr char isValid_() {
        constexpr const std::string_view name(__PRETTY_FUNCTION__);
        constexpr auto i = name.find("V = ");
        if constexpr (name[i + 4] == '(') {
            return 0;
        }
        return 1;
    }

    template <Enum V>
    static constexpr std::string_view nameOf_() {
        constexpr const std::string_view name(__PRETTY_FUNCTION__);
        auto i = name.find("V = ");
        auto j = name.find(';', i);
        auto k = (j == std::string_view::npos) ? name.find(']', i) : j;
        auto res = name.substr(i+4, k-i-4);
        if ((i = res.find("::")) != std::string_view::npos)
            res = res.substr(i+2, j-i-2);
        return res;
    }

    template<char sign, int offset, int total, std::size_t max, class Ret>
    static constexpr Ret getNameValueAt_(const std::size_t i) {
        constexpr bool retIsInt = std::is_same_v<Ret, int>;
        Ret null;
        if constexpr (retIsInt) null = 0;
        else null = std::string_view("");
        Ret res = null;
        int counter = 0;
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (void)((res = [&]() {
                constexpr auto currentValue = sign*(static_cast<int>(I)+1+offset);
                if constexpr (isValid_<static_cast<Enum>(currentValue)>())
                    ++counter;
                if (nameValueFound_<sign, total>(i, counter)) {
                    if constexpr (retIsInt)
                        return currentValue;
                    else
                        return nameOf_<static_cast<Enum>(currentValue)>();
                }
                return null;
            }(), !nameValueFound_<sign, total>(i, counter)) && ...);
        }(std::make_index_sequence<max>{});
        return res;
    }
    
    template<char sign, int total>
    static constexpr bool nameValueFound_(const std::size_t i, const int counter) {
        return (static_cast<int>(i) - sign * counter == (1-sign)/2*total - (1+sign)/2);
    }
};
