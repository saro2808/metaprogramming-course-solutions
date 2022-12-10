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

template <class Enum, Enum V>
static constexpr std::string_view nameOf() noexcept {
    constexpr const std::string_view name(__PRETTY_FUNCTION__);
    constexpr auto i = name.find("V = ");
    if constexpr (name[i + 4] == '(')
        return std::string_view("");
    constexpr auto j = name.find(';', i);
    constexpr auto k = (j == std::string_view::npos) ? name.find(']', i) : j;
    constexpr auto res = name.substr(i+4, k-i-4);
    constexpr auto l = res.find("::");
    if constexpr (l != std::string_view::npos)
        return res.substr(l+2, j-l-2);
    return res;
}

template <class Enum, std::size_t MAXN>
static constexpr std::size_t highLimit() noexcept {
    return std::min( static_cast<std::size_t>(std::numeric_limits<std::underlying_type_t<Enum>>::max()), MAXN);
}

template <class Enum, std::size_t MAXN>
static constexpr std::size_t lowLimit() noexcept {
    return std::min(-static_cast<std::size_t>(std::numeric_limits<std::underlying_type_t<Enum>>::min()), MAXN);
}

template<class Enum, std::size_t MAXN>
inline constexpr auto highV = highLimit<Enum, MAXN>();

template<class Enum, std::size_t MAXN>
inline constexpr auto  lowV =  lowLimit<Enum, MAXN>();

template<class Enum, std::size_t MAXN, char sign, int offset, std::size_t max>
static constexpr auto fill(auto& arr, std::size_t start) noexcept {
    std::size_t i = start;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        ([&]() {
            constexpr auto currentValue = static_cast<Enum>(sign*(static_cast<int>(I)+1+offset));
            arr[i].first = nameOf<Enum, currentValue>();
            if (arr[i].first == std::string_view("")) {
                arr[i].second = static_cast<Enum>(MAXN + 1); // invalid enumerator
            } else {
                arr[i].second = currentValue;
                i += sign;
            }
        }(), ...);
    }(std::make_index_sequence<max>{});
    return sign * (i - start);
}

template<class Enum, std::size_t MAXN>
static constexpr auto describeEnum() noexcept {
    constexpr auto  low =  lowV<Enum, MAXN>;
    constexpr auto high = highV<Enum, MAXN>;

    std::array<NameValue<Enum>, high + low + 1> nameValueArr;
    
    constexpr auto zeroValue = static_cast<Enum>(0);
    constexpr auto zeroName = nameOf<Enum, zeroValue>();
    constexpr std::size_t zeroIsValid = (zeroName == "") ? 0 : 1;
    if constexpr (zeroIsValid) {
        nameValueArr[low].first = zeroName;
        nameValueArr[low].second = zeroValue;
    }
    auto smallNegativesSize = fill<Enum, MAXN, -1, 0,  low/2>(nameValueArr, low - 1);
    auto smallPositivesSize = fill<Enum, MAXN,  1, 0, high/2>(nameValueArr, low + zeroIsValid);
    auto negativesSize = smallNegativesSize + fill<Enum, MAXN, -1,  low/2,  low -  low/2>(nameValueArr, low - 1 - smallNegativesSize);
    auto positivesSize = smallPositivesSize + fill<Enum, MAXN,  1, high/2, high - high/2>(nameValueArr, low + zeroIsValid + smallPositivesSize) + zeroIsValid;
        
    return std::make_pair(
            std::make_pair(low - negativesSize, negativesSize + positivesSize),
            nameValueArr
    );
}

template<class Enum, std::size_t MAXN>
inline constexpr auto enumData = describeEnum<Enum, MAXN>();

template<class Enum, std::size_t MAXN>
inline constexpr auto zeroIdx = enumData<Enum, MAXN>.first.first;

} // namespace detail

template <class Enum, std::size_t MAXN = 512>
	requires std::is_enum_v<Enum>
struct EnumeratorTraits {
    static constexpr std::size_t size() noexcept {
        return detail::enumData<Enum, MAXN>.first.second;
    }

    static constexpr Enum at(const std::size_t i) noexcept {
        return detail::enumData<Enum, MAXN>.second[detail::zeroIdx<Enum, MAXN> + i].second;
    }

    static constexpr std::string_view nameAt(const std::size_t i) noexcept {
        return detail::enumData<Enum, MAXN>.second[detail::zeroIdx<Enum, MAXN> + i].first;
    }
};

