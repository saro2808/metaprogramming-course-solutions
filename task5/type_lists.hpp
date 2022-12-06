#pragma once

namespace type_lists {

template<class TL>
concept TypeSequence =
    requires {
        typename TL::Head;
        typename TL::Tail;
    };

struct Nil {};

template<class... Ts>
struct TTuple {};

template<class TL>
concept Empty = std::derived_from<TL, Nil>;

template<class TL>
concept TypeList = Empty<TL> || TypeSequence<TL>;

// ToTypeList
template <class... Ts>
struct ToTypeList : Nil {};

template <class T, class... Ts>
struct ToTypeList<T, Ts...> {
    using Head = T;
    using Tail = ToTypeList<Ts...>;
};

template <class T, class... Ts>
struct ToTypeList<TTuple<T, Ts...>> {
    using Head = T;
    using Tail = ToTypeList<Ts...>;
};

template<>
struct ToTypeList<TTuple<>> : Nil {};

// ToTuple
template< class... Ts >
struct ToTupleHelper {
	using Value = TTuple<Ts...>;
};

template< TypeSequence TL, class... Ts >
struct ToTupleHelper<TL, Ts...> {
	using Value = typename ToTupleHelper<typename TL::Tail, Ts..., typename TL::Head>::Value;
};

template< Empty E, class... Ts >
struct ToTupleHelper<E, Ts...> {
	using Value = typename ToTupleHelper<Ts...>::Value;
};

template< TypeList TL >
using ToTuple = typename ToTupleHelper<TL>::Value;

// Append
template <TypeList L, TypeList R>
struct Append : Nil {};

template <TypeSequence L, TypeList R>
struct Append<L, R> {
    using Head = typename L::Head;
    using Tail = Append<typename L::Tail, R>;
};

template <Empty L, TypeSequence R>
struct Append<L, R> {
    using Head = typename R::Head;
    using Tail = typename R::Tail;
};

// GetNth
template <int N, typename... Ts>
struct GetNth {
    using Type = Nil;
};

template <int N, typename T, typename... Ts>
struct GetNth<N, TTuple<T, Ts...>> {
    using Type = typename GetNth<N - 1, TTuple<Ts...>>::Type;
};

template <typename T, typename... Ts>
struct GetNth<0, TTuple<T, Ts...>> {
    using Type = T;
};

} // namespace type_lists
