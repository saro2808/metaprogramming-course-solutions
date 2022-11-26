#pragma once

#include <type_traits>
#include <cstdint>
#include <utility>
#include <string_view>
#include <array>
#include <limits>

template<class Enum>
using NameValue = std::pair<std::string_view, Enum>;

namespace detail {

    constexpr std::size_t min(std::size_t a, std::size_t b) noexcept {
        return a < b ? a : b;
    }

    template <class Enum, Enum V>
    static constexpr char isValid_() noexcept {
        constexpr const std::string_view name(__PRETTY_FUNCTION__);
        constexpr auto i = name.find("V = ");
        if constexpr (name[i + 4] == '(')
            return 0;
        return 1;
    }

    template <class Enum, Enum V>
    static constexpr std::string_view nameOf_() noexcept {
        constexpr const std::string_view name(__PRETTY_FUNCTION__);
        constexpr auto i = name.find("V = ");
        constexpr auto j = name.find(';', i);
        constexpr auto k = (j == std::string_view::npos) ? name.find(']', i) : j;
        constexpr auto res = name.substr(i+4, k-i-4);
        constexpr auto l = res.find("::");
        if constexpr (l != std::string_view::npos)
            return res.substr(l+2, j-l-2);
        return res;
    }

    template <class Enum>
    static constexpr char zeroIsValid_() noexcept {
        return isValid_<Enum, static_cast<Enum>(0)>();
    }

    template <class Enum, std::size_t MAXN>
    static constexpr std::size_t high_() noexcept {
        return min( static_cast<std::size_t>(std::numeric_limits<std::underlying_type_t<Enum>>::max()), MAXN);
    }

    template <class Enum, std::size_t MAXN>
    static constexpr std::size_t low_() noexcept {
        return min(-static_cast<std::size_t>(std::numeric_limits<std::underlying_type_t<Enum>>::min()), MAXN);
    }

    template <class Enum, char sign, std::size_t offset, std::size_t... ints>
    static constexpr std::size_t size_(std::integer_sequence<std::size_t, ints...>) noexcept {
        // works fine when compiled with clang without flags only if sizeof...(ints) doesn't exceed 256
        if constexpr (sizeof...(ints) == 0)
            return 0;
        else
            return (isValid_<Enum, static_cast<Enum>(sign * (ints + offset))>() + ...);
    }

    template<class Enum, std::size_t MAXN, char sign, std::size_t max>
    static constexpr auto signedSize_() noexcept {
        constexpr auto maxHalved = max / 2;
        constexpr auto sz0 = size_<Enum, sign,             1>(std::make_index_sequence<      maxHalved>{});
        constexpr auto sz1 = size_<Enum, sign, maxHalved + 1>(std::make_index_sequence<max - maxHalved>{});
        return std::make_pair(sz0, sz1);
    }

    template <class Enum, std::size_t MAXN>
    static constexpr std::size_t size() noexcept {
        constexpr auto positives = signedSize_<Enum, MAXN, 1, high_<Enum, MAXN>()>();
        constexpr auto negatives = signedSize_<Enum, MAXN, -1, low_<Enum, MAXN>()>();
        return zeroIsValid_<Enum>() + positives.first + positives.second
                                    + negatives.first + negatives.second;
    }

    template<class Enum, std::size_t MAXN, char sign, int offset, std::size_t max, std::size_t end>
    static constexpr void fill(std::array<NameValue<Enum>, size<Enum, MAXN>()>& arr, std::size_t start) noexcept {
        auto i = start;
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            (void)(([&]() {
                constexpr auto currentValue = static_cast<Enum>(sign*(static_cast<int>(I)+1+offset));
                if constexpr (isValid_<Enum, currentValue>()) {
                    i += sign;
                    arr[i].first = nameOf_<Enum, currentValue>();
                    arr[i].second = currentValue;
                }
            }(), i != end) && ...);
        }(std::make_index_sequence<max>{});
    }

    template<class Enum, std::size_t MAXN>
    static constexpr auto nameValueArr() noexcept {
        constexpr auto positives = signedSize_<Enum, MAXN, 1, high_<Enum, MAXN>()>();
        constexpr auto negatives = signedSize_<Enum, MAXN, -1, low_<Enum, MAXN>()>();
        constexpr auto negativesSize = negatives.first + negatives.second;
        constexpr auto zeroIsValid = zeroIsValid_<Enum>();
        constexpr auto sz = negativesSize + zeroIsValid + positives.first + positives.second;
        std::array<NameValue<Enum>, sz> nameValueArr;
        constexpr auto  low =  low_<Enum, MAXN>();
        constexpr auto high = high_<Enum, MAXN>();
        if constexpr (zeroIsValid) {
            auto negativesSizes = signedSize_<Enum, MAXN, -1, low>();
            auto zeroIdx = negativesSizes.first + negativesSizes.second;
            constexpr auto value = static_cast<Enum>(0);
            nameValueArr[zeroIdx].first = nameOf_<Enum, value>();
            nameValueArr[zeroIdx].second = value;
        }
        fill<Enum, MAXN, -1,  low/2,  low -  low/2, 0>(nameValueArr, negatives.second);
        fill<Enum, MAXN, -1, 0,  low/2, negatives.second>(nameValueArr, negativesSize);
        fill<Enum, MAXN,  1, 0, high/2, negativesSize + zeroIsValid + positives.first - 1>(nameValueArr, negativesSize + zeroIsValid - 1);
        fill<Enum, MAXN,  1, high/2, high - high/2, sz - 1>(nameValueArr, negativesSize + zeroIsValid + positives.first - 1);
        return nameValueArr;
    }

} // namespace detail

template <class Enum, std::size_t MAXN = 512>
	requires std::is_enum_v<Enum>
struct EnumeratorTraits {
    static constexpr std::size_t size() noexcept {
        return size_;
    }

    static constexpr Enum at(const std::size_t i) noexcept {
        return nameValueArr_[i].second;
    }

    static constexpr std::string_view nameAt(const std::size_t i) noexcept {
        return nameValueArr_[i].first;
    }

private:
    static constexpr std::size_t size_ = detail::size<Enum, MAXN>();
    static constexpr auto /*std::array<NameValue<Enum>, EnumeratorTraits<Enum, MAXN>::size_>*/ nameValueArr_ = detail::nameValueArr<Enum, MAXN>();
};

//template <class Enum, std::size_t MAXN>
//    requires std::is_enum_v<Enum>
//std::array<NameValue<Enum>, EnumeratorTraits<Enum, MAXN>::size_> EnumeratorTraits<Enum, MAXN>::nameValueArr_ = detail::nameValueArr<Enum, MAXN>();
