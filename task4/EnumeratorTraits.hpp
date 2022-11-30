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

static constexpr std::size_t min(std::size_t a, std::size_t b) noexcept {
    return a < b ? a : b;
}

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
    return min( static_cast<std::size_t>(std::numeric_limits<std::underlying_type_t<Enum>>::max()), MAXN);
}

template <class Enum, std::size_t MAXN>
static constexpr std::size_t lowLimit() noexcept {
    return min(-static_cast<std::size_t>(std::numeric_limits<std::underlying_type_t<Enum>>::min()), MAXN);
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
                arr[i++].second = currentValue;
            }
        }(), ...);
    }(std::make_index_sequence<max>{});
    return i - start;
}

template<class Enum, std::size_t MAXN>
static constexpr auto nameValueArr() noexcept {
    constexpr auto  low =  lowV<Enum, MAXN>;
    constexpr auto high = highV<Enum, MAXN>;

    constexpr auto zeroValue = static_cast<Enum>(0);
    constexpr auto zeroNameValue = nameOf<Enum, zeroValue>();
        
    std::array<NameValue<Enum>, high + 1> positiveArr;
    std::array<NameValue<Enum>,  low + 1> negativeArr;
        
    auto smallNegativesSize = fill<Enum, MAXN, -1, 0,  low/2>(negativeArr, 1);
    auto smallPositivesSize = fill<Enum, MAXN,  1, 0, high/2>(positiveArr, 1);
    auto NegativesSize = smallNegativesSize + fill<Enum, MAXN, -1,  low/2,  low -  low/2>(negativeArr, 1 + smallNegativesSize);
    auto PositivesSize = smallPositivesSize + fill<Enum, MAXN,  1, high/2, high - high/2>(positiveArr, 1 + smallPositivesSize);
        
    return std::make_pair(
            std::make_pair(NegativesSize, PositivesSize),
            std::make_pair(negativeArr, positiveArr)
    );
}

template<class Enum>
inline constexpr auto zeroData = std::make_pair(nameOf<Enum, static_cast<Enum>(0)>(), static_cast<Enum>(0));

template<class Enum>
inline constexpr std::size_t zeroIsValid = (zeroData<Enum>.first == "" ? 0 : 1);

template<class Enum, std::size_t MAXN>
inline constexpr auto enumData = nameValueArr<Enum, MAXN>();

template<class Enum, std::size_t MAXN>
inline constexpr auto sizes = enumData<Enum, MAXN>.first;

template<class Enum, std::size_t MAXN>
inline constexpr auto negativesSize = sizes<Enum, MAXN>.first;

template<class Enum, std::size_t MAXN>
inline constexpr auto positivesSize = sizes<Enum, MAXN>.second;

template<class Enum, std::size_t MAXN>
inline constexpr auto size = negativesSize<Enum, MAXN> + positivesSize<Enum, MAXN> + zeroIsValid<Enum>;

template<class Enum, std::size_t MAXN>
inline constexpr auto nameValues = enumData<Enum, MAXN>.second;

template<class Enum, std::size_t MAXN>
inline constexpr auto negativesArr = nameValues<Enum, MAXN>.first;

template<class Enum, std::size_t MAXN>
inline constexpr auto positivesArr = nameValues<Enum, MAXN>.second;

template<class Enum, std::size_t MAXN>
static constexpr NameValue<Enum> nameValueAt(const std::size_t i) noexcept {
    if constexpr (zeroIsValid<Enum>) {
        if (i == negativesSize<Enum, MAXN>)
            return zeroData<Enum>;
    }
    if (i < negativesSize<Enum, MAXN>)
        return negativesArr<Enum, MAXN>[negativesSize<Enum, MAXN> - i];
    return positivesArr<Enum, MAXN>[i - negativesSize<Enum, MAXN> - zeroIsValid<Enum> + 1];
}

} // namespace detail

template <class Enum, std::size_t MAXN = 512>
	requires std::is_enum_v<Enum>
struct EnumeratorTraits {
    static constexpr std::size_t size() noexcept {
        return detail::size<Enum, MAXN>;
    }

    static constexpr Enum at(const std::size_t i) noexcept {
        return detail::nameValueAt<Enum, MAXN>(i).second;
    }

    static constexpr std::string_view nameAt(const std::size_t i) noexcept {
        return detail::nameValueAt<Enum, MAXN>(i).first;
    }
};

