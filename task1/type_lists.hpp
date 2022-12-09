#pragma once

#include <concepts>

#include "type_tuples.hpp"

using type_tuples::TypeTuple;
using type_tuples::TTuple;

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

// Cons
template<class T, class U>
struct Cons {
	using Head = T;
	using Tail = U;
};

// FromTuple
template< TypeTuple TT >
struct FromTuple : Nil {};

template< class T, class... Ts >
struct FromTuple<TTuple<T, Ts...>> {
    using Head = T;
    using Tail = FromTuple<TTuple<Ts...>>;
};

// ToTuple
namespace detail {

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

template<class... Ts>
struct ToTypeList : Nil {};

template<class T, class... Ts>
struct ToTypeList<T, Ts...> {
    using Head = T;
    using Tail = ToTypeList<Ts...>;
};

} // namespace detail

template< TypeList TL >
using ToTuple = typename detail::ToTupleHelper<TL>::Value;

// Repeat
template<class T>
struct Repeat {
    using Head = T;
    using Tail = Repeat<T>;
};

// Take
template< int N, TypeList TL >
struct Take : Nil {};

template< int N, TypeSequence TL >
    requires (N > 0)
struct Take<N, TL> {
    using Head = typename TL::Head;
    using Tail = Take<N - 1, typename TL::Tail>;
};

// Drop
template< int N, TypeList TL >
struct Drop : Nil {};

template< int N, TypeList TL >
    requires (N > 0 && !Empty<typename TL::Tail>)
struct Drop<N, TL> : Drop<N - 1, typename TL::Tail> {};

template< TypeSequence TL >
struct Drop<0, TL> {
    using Head = typename TL::Head;
    using Tail = typename TL::Tail;
};

// Replicate
template< int N, class T >
struct Replicate : Nil {};

template< int N, class T >
    requires (N > 0)
struct Replicate<N, T> {
    using Head = T;
    using Tail = Replicate<N - 1, T>;
};

// Map
template< template<class> class F, TypeList TL >
struct Map : Nil {};

template< template<class> class F, TypeSequence TL >
struct Map<F, TL> {
    using Head = F<typename TL::Head>;
    using Tail = Map<F, typename TL::Tail>;
};

// Filter
template< template<class> class P, TypeList TL >
struct Filter : Nil {};

template< template<class> class P, TypeSequence TL >
    requires (requires { P<typename TL::Head>::Value; } && P<typename TL::Head>::Value)
struct Filter<P, TL> {
    using Head = typename TL::Head;
    using Tail = Filter<P, typename TL::Tail>;
};

template< template<class> class P, TypeSequence TL >
    requires (requires { P<typename TL::Head>::Value; } && !P<typename TL::Head>::Value)
struct Filter<P, TL> {
	using Answer = Filter<P, typename TL::Tail>;
    using Head = typename Answer::Head;
    using Tail = typename Answer::Tail;
};

// Iterate
template< template<class> class F, class T >
struct Iterate {
    using Head = T;
    using Tail = Iterate<F, F<T>>;
};

// Cycle
template< TypeList TL >
struct Cycle {
	using Head = typename TL::Head;
	using Tail = Cycle<FromTuple<ToTuple<detail::ToTypeList<typename TL::Tail, Cons<typename TL::Head, Nil>>>>>;
};

template< template<class, class> class OP, class T, TypeList TL >
struct Scanl;

// Inits, Tails
namespace detail {

template< class, class >
struct AppendHelper {
    using Value = TTuple<>;
};

template< class T, class... Ts >
struct AppendHelper<TTuple<Ts...>, T> {
    using Value = TTuple<Ts..., T>;
};

template< class L, class R >
using Append = typename AppendHelper<L, R>::Value;

template< TypeList L, class >
using Cut = std::conditional_t<Empty<L>, Cons<Nil, Nil>, typename L::Tail>;

} // namespace detail

template< TypeList TL >
using Inits = Map<FromTuple, Scanl<detail::Append, TTuple<>, TL>>;

template< TypeList TL >
using Tails = Scanl<detail::Cut, TL, TL>;

// Scanl
template< template<class, class> class OP, class T, TypeList TL >
struct Scanl : Nil {};

template< template<class, class> class OP, class T, TypeSequence TL >
struct Scanl<OP, T, TL> {
	using Head = T;
    using Tail = Scanl<OP, OP<Head, typename TL::Head>, typename TL::Tail>;
};

template< template<class, class> class OP, class T, Empty E >
struct Scanl<OP, T, E> {
	using Head = T;
	using Tail = Nil;
};

// Foldl
namespace detail {

template< template<class, class> class OP, class T, TypeList TL >
struct FoldlHelper;

template< template<class, class> class OP, class T, TypeList TL >
struct FoldlHelper {
    using Value = typename FoldlHelper< OP, OP<T, typename TL::Head>, typename TL::Tail>::Value;
};

template< template<class, class> class OP, class T, TypeList TL >
    requires (Empty<TL> || Empty<typename TL::Tail>)
struct FoldlHelper<OP, T, TL> {
	using Value = T;
};

} // namesace detail

template< template<class, class> class OP, class T, TypeList TL >
using Foldl = typename detail::FoldlHelper<OP, T, TL>::Value;

// Zip2
template< TypeList L, TypeList R >
struct Zip2;

template< TypeList L, TypeList R >
struct Zip2 {
    using Head = TTuple<typename L::Head, typename R::Head>;
    using Tail = Zip2<typename L::Tail, typename R::Tail>;
};

template< TypeList L, TypeList R >
    requires (Empty<L> || Empty<R>)
struct Zip2<L, R> : Nil {};

// Zip
namespace detail {

template<TypeSequence TL>
using Head = typename TL::Head;

template<TypeSequence TL>
using Tail = typename TL::Tail;

// TLL stands for TypeLists' list
template<TypeList TLL>
struct ZipHelper {
    using Head = ToTuple<Map<Head, TLL>>;
    using Tail = ZipHelper<Map<Tail, TLL>>;
};

template<Empty E>
struct ZipHelper<E> : Nil {};

} // namespace detail

template<TypeList... TLs>
using Zip = detail::ZipHelper<detail::ToTypeList<TLs...>>;

// GroupBy
template< template<class, class> class EQ, TypeList TL >
struct GroupBy;

template< template<class, class> class EQ, TypeSequence TL >
	requires EQ<typename TL::Head, typename TL::Tail::Head>::Value
struct GroupBy<EQ, TL> {
	using Prev = GroupBy<EQ, typename TL::Tail>;
	using Head = Cons<typename TL::Head, typename Prev::Head>;
	using Tail = typename Prev::Tail;
};

template< template<class, class> class EQ, TypeSequence TL >
struct GroupBy<EQ, TL> {
	using Head = Cons<typename TL::Head, Nil>;
	using Tail = GroupBy<EQ, typename TL::Tail>;
};

template< template<class, class> class EQ, Empty E >
struct GroupBy<EQ, E> : Nil {};

} // namespace type_lists

