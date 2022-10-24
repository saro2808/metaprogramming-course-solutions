#pragma once

#include <concepts>

namespace type_lists
{

template<class TL>
concept TypeSequence =
    requires {
        typename TL::Head;
        typename TL::Tail;
    };

struct Nil {};

template<class TL>
concept Empty = std::derived_from<TL, Nil>;

template<class TL>
concept TypeList = Empty<TL> || TypeSequence<TL>;

// TopSort
template< TypeSequence TL >
struct Minimal {
	using Prev = Minimal<typename TL::Tail>;
	constexpr static bool isLess = std::is_base_of_v<typename Prev::Value::Class, typename TL::Head::Class>;
	using Value = std::conditional_t<isLess, typename TL::Head, typename Prev::Value>;
	constexpr static size_t Index = isLess ? 0 : Prev::Index + 1;
};

template< TypeSequence TL >
	requires Empty<typename TL::Tail>
struct Minimal<TL> {
	using Value = typename TL::Head;
	constexpr static size_t Index = 0;
};


template< TypeSequence TL, size_t IndexOfMinimum >
struct WithoutMinimal {
	using Head = TL::Head;
	using Tail = WithoutMinimal<typename TL::Tail, IndexOfMinimum - 1>;
};

template< TypeSequence TL >
	requires (!Empty<typename TL::Tail>)
struct WithoutMinimal<TL, 0> {
	using Head = typename TL::Tail::Head;
	using Tail = typename TL::Tail::Tail;
};

template< TypeSequence TL >
	requires Empty<typename TL::Tail>
struct WithoutMinimal<TL, 0> : Nil {};

template< TypeList TL >
struct TopSort {
	using Minimum = Minimal<TL>;
	using Head = typename Minimum::Value;
	using Tail = TopSort<WithoutMinimal<TL, Minimum::Index>>;
};

template< Empty E >
struct TopSort<E> : Nil {};

// VaArgsToTypeList
template<class... Ts>
struct VaArgsToTypeList : Nil {};

template<class T, class... Ts>
struct VaArgsToTypeList<T, Ts...> {
	using Head = T;
	using Tail = VaArgsToTypeList<Ts...>;
};

} // namespace type_lists

