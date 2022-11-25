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
    
    static constexpr Enum at(const std::size_t i) noexcept {
        constexpr auto negativesSizes = signedSize_<-1, low_()>();
        constexpr auto positivesSizes = signedSize_<1, high_()>();
        constexpr auto negativesSize = negativesSizes.first + negativesSizes.second;
        constexpr auto positivesSize = positivesSizes.first + positivesSizes.second;
        constexpr auto zeroIsValid = isValid_<(Enum)0>();
        
        if (i < negativesSize) {
            // loop over negatives
            if (i < negativesSizes.second)
                return getValueAt_<-1, low_()/2, low_() - low_()/2>(i);
            return getValueAt_<-1, 0, low_()/2>(i - negativesSizes.second);
        }
        if constexpr (zeroIsValid)
            if (i == negativesSize)
                return (Enum)0;
        // loop over positives
        if (i < negativesSize + zeroIsValid + positivesSizes.first)
            return getValueAt_<1, 0, high_()/2>(i - zeroIsValid - negativesSize);
        return getValueAt_<1, high_()/2, high_() - high_()/2>(i - zeroIsValid - negativesSize - positivesSizes.first);
    }
/*
    template<typename ...>
    struct NameValueInfo {
        constexpr auto operator() {
            constexpr auto negativesSizes = signedSize_<-1, low_()>();
            constexpr auto positivesSizes = signedSize_<1, high_()>();
            constexpr auto negativesSize = negativesSizes.first + negativesSizes.second;
            constexpr auto positivesSize = positivesSizes.first + positivesSizes.second;
            constexpr auto zeroIsValid = isValid_<(Enum)0>();

        if (i < negativesSize) {
            // loop over negatives
            if (i < negativesSizes.second)
                return getValueAt_<-1, low_()/2, low_() - low_()/2>(i);
            return getValueAt_<-1, 0, low_()/2>(i - negativesSizes.second);
        }
        if constexpr (zeroIsValid)
            if (i == negativesSize)
                return (Enum)0;
        // loop over positives
        if (i < negativesSize + zeroIsValid + positivesSizes.first)
            return getValueAt_<1, 0, high_()/2>(i - zeroIsValid - negativesSize);
        return getValueAt_<1, high_()/2, high_() - high_()/2>(i - zeroIsValid - negativesSize - positivesSizes.first);
        }
    };
*/
    static constexpr std::string_view nameAt(const std::size_t i) noexcept {
        constexpr auto negativesSizes = signedSize_<-1, low_()>();
        constexpr auto positivesSizes = signedSize_<1, high_()>();
        constexpr auto negativesSize = negativesSizes.first + negativesSizes.second;
        constexpr auto positivesSize = positivesSizes.first + positivesSizes.second;
        constexpr auto zeroIsValid = isValid_<(Enum)0>();

        if (i < negativesSize) {
            // loop over negatives
            if (i < negativesSizes.second)
                return getNameAt_<-1, low_()/2, low_() - low_()/2>(i);
            return getNameAt_<-1, 0, low_()/2>(i - negativesSizes.second);
        }
        if constexpr (zeroIsValid)
            if (i == negativesSize)
                return nameOf_<(Enum)0>();
        // loop over positives
        if (i < negativesSize + zeroIsValid + positivesSizes.first)
            return getNameAt_<1, 0, high_()/2>(i - zeroIsValid - negativesSize);
        return getNameAt_<1, high_()/2, high_() - high_()/2>(i - zeroIsValid - negativesSize - positivesSizes.first);
    }

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

    template<char sign, int offset, std::size_t max>
    static constexpr Enum getValueAt_(const std::size_t i) {
        int res = 0;
        int counter = 0;
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (void)((res = [&]() {
                if constexpr (isValid_<(Enum)(sign*(static_cast<int>(I)+1+offset))>())
                    ++counter;
                if ((int)i - sign * counter == (1-sign)/2*max - (1+sign)/2)
                    return sign*(static_cast<int>(I)+1+offset);
                return 0;
            }(), (int)i - sign * counter != (1-sign)/2*max - (1+sign)/2) && ...);
        }(std::make_index_sequence<max>{});
        return (Enum)res;
    }

    template<char sign, int offset, std::size_t max>
    static constexpr std::string_view getNameAt_(const std::size_t i) {
        std::string_view res = "";
        int counter = 0;
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (void)((res = [&]() {
                if constexpr (isValid_<(Enum)(sign*(static_cast<int>(I)+1+offset))>())
                    ++counter;
                if ((int)i - sign * counter == (1-sign)/2*max - (1+sign)/2)
                    return nameOf_<(Enum)(sign*(static_cast<int>(I)+1+offset))>();
                return std::string_view("");
            }(), (int)i - sign * counter != (1-sign)/2*max - (1+sign)/2) && ...);
        }(std::make_index_sequence<max>{});
        return res;
    }

/*    template<char sign, int offset, std::size_t max>
    static constexpr std::string_view getNameAt_(const std::size_t i) {
        std::string_view res = "";
        int counter = 0;
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (void)((res = [&]() {
                if constexpr (isValid_<(Enum)(sign*(static_cast<int>(I)+1+offset))>())
                    ++counter;
                if ((int)i - sign * counter == (1-sign)/2*max - (1+sign)/2)
                    return nameOf_<(Enum)(sign*(static_cast<int>(I)+1+offset))>();
                return std::string_view("");
            }(), (int)i - sign * counter == (1-sign)/2*max - (1+sign)/2) && ...);
        }(std::make_index_sequence<max>{});
        return res;
    }*/
};
