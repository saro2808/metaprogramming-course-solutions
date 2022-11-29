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
    static constexpr std::string_view nameOf_() noexcept {
        constexpr const std::string_view name(__PRETTY_FUNCTION__);
        constexpr auto i = name.find("V = ");
        if constexpr (name[i + 4] == '(')
            return std::string_view("");
        else {
            constexpr auto j = name.find(';', i);
            constexpr auto k = (j == std::string_view::npos) ? name.find(']', i) : j;
            constexpr auto res = name.substr(i+4, k-i-4);
            constexpr auto l = res.find("::");
            if constexpr (l != std::string_view::npos)
                return res.substr(l+2, j-l-2);
            return res;
        }
    }

    template <class Enum, std::size_t MAXN>
    static constexpr std::size_t high_() noexcept {
        return min( static_cast<std::size_t>(std::numeric_limits<std::underlying_type_t<Enum>>::max()), MAXN);
    }

    template <class Enum, std::size_t MAXN>
    static constexpr std::size_t low_() noexcept {
        return min(-static_cast<std::size_t>(std::numeric_limits<std::underlying_type_t<Enum>>::min()), MAXN);
    }

    template<class Enum, std::size_t MAXN>
    inline constexpr auto high_v = high_<Enum, MAXN>();

    template<class Enum, std::size_t MAXN>
    inline constexpr auto  low_v =  low_<Enum, MAXN>();

    template<class Enum, std::size_t MAXN, char sign, int offset, std::size_t max>
    static constexpr auto fill(auto& arr, std::size_t start) noexcept {
        std::size_t i = start;
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            ([&]() {
                constexpr auto currentValue = static_cast<Enum>(sign*(static_cast<int>(I)+1+offset));
                arr[i].first = nameOf_<Enum, currentValue>();
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
        constexpr auto  low =  low_v<Enum, MAXN>;
        constexpr auto high = high_v<Enum, MAXN>;

        constexpr auto zeroValue = static_cast<Enum>(0);
        constexpr auto zeroNameValue = nameOf_<Enum, zeroValue>();
        
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
    
    template<class Enum, std::size_t MAXN>
    inline constexpr auto enumData = nameValueArr<Enum, MAXN>();

    template<class Enum, std::size_t MAXN>
    inline constexpr auto sizes = enumData<Enum, MAXN>.first;

    template<class Enum, std::size_t MAXN>
    inline constexpr auto nameValues = enumData<Enum, MAXN>.second;

    template<class Enum>
    inline constexpr auto zeroData = std::make_pair(nameOf_<Enum, static_cast<Enum>(0)>(), static_cast<Enum>(0));

} // namespace detail

template <class Enum, std::size_t MAXN = 512>
	requires std::is_enum_v<Enum>
struct EnumeratorTraits {
    static constexpr std::size_t size() noexcept {
        return negativesSize + positivesSize + zeroIsValid;
    }

    static constexpr Enum at(const std::size_t i) noexcept {
        if (zeroIsValid) {
            if (i == negativesSize)
                return zeroNameValue.second;
        }
        if (i < negativesSize)
            return negativesArr[negativesSize - i].second;
        return positivesArr[i - negativesSize - zeroIsValid + 1].second;
    }

    static constexpr std::string_view nameAt(const std::size_t i) noexcept {
        if (zeroIsValid) {
            if (i == negativesSize)
                return zeroNameValue.first;
        }
        if (i < negativesSize)
            return negativesArr[negativesSize - i].first;
        return positivesArr[i - negativesSize - zeroIsValid + 1].first;
    }

    constexpr static auto negativesSize = detail::sizes<Enum, MAXN>.first;
    constexpr static auto positivesSize = detail::sizes<Enum, MAXN>.second;
    constexpr static std::size_t zeroIsValid = (detail::zeroData<Enum>.first == "" ? 0 : 1);

    constexpr static auto negativesArr = detail::nameValues<Enum, MAXN>.first;
    constexpr static auto positivesArr = detail::nameValues<Enum, MAXN>.second;
    constexpr static auto zeroNameValue = detail::zeroData<Enum>;
};

