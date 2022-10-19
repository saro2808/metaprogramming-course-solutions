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

template< class... Ts >
using ToTuple = typename ToTupleHelper<Ts...>::Value;

// Repeat
template<class T>
struct Repeat {
    using Head = T;
    using Tail = Repeat<T>;
};

// Take
template< int N, TypeList TL >
struct Take;

template< int N, TypeSequence TL >
    requires (N > 0)
struct Take<N, TL> {
    using Head = typename TL::Head;
    using Tail = Take<N - 1, typename TL::Tail>;
};

template< TypeList TL >
struct Take<0, TL> : Nil {};

template< int N, Empty E >
struct Take<N, E> : Nil {};

// Drop
template<int N, TypeList TL>
struct DropHelper;

template< int N, TypeSequence TL >
    requires (N > 0 && !Empty<typename TL::Tail>)
struct DropHelper<N, TL> {
    using Value = typename DropHelper<N - 1, typename TL::Tail>::Value;
};

template< TypeSequence TL >
struct DropHelper<0, TL> {
    using Value = TL;
};

template< int N, TypeList TL >
    requires (Empty<TL> || Empty<typename TL::Tail>)
struct DropHelper<N, TL> : Nil {
	using Value = Nil;
};

template< int N, TypeList TL >
using Drop = typename DropHelper<N, TL>::Value;

// Replicate
template< int N, class T >
struct Replicate;

template< int N, class T >
    requires (N > 0)
struct Replicate<N, T> {
    using Head = T;
    using Tail = Replicate<N - 1, T>;
};

template< class T >
struct Replicate<0, T> : Nil {};

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
    using Head = Answer::Head;
    using Tail = Answer::Tail;
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
	using Tail = Cycle<FromTuple<ToTuple<typename TL::Tail, Cons<typename TL::Head, Nil>>>>;
};

// Inits
template< TypeList TL >
struct InitsHelper : Nil {};

template< TypeSequence TL >
struct InitsHelper<TL> {
	
	template<TypeList L>
	struct AppendHead : Nil {};

	template<TypeSequence L>
	struct AppendHead<L> {
		using Head = typename TL::Head;
		using Tail = L;
	};
	
	using Head = Cons<typename TL::Head, Nil>;
	using Tail = Map<AppendHead, InitsHelper<typename TL::Tail>>;
};

template< TypeList TL >
struct Inits {
	using Head = Nil;
	using Tail = InitsHelper<TL>;
};

// Tails
template< TypeList TL >
struct Tails : Nil {};

template< TypeSequence TL >
struct Tails<TL> {
    using Head = TL;
    using Tail = Tails<typename TL::Tail>;
};

template< Empty E >
struct Tails<E> {
	using Head = E;
	using Tail = Nil;
};

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

template< template<class, class> class OP, class T, TypeList TL >
using Foldl = typename FoldlHelper<OP, T, TL>::Value;

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
template< TypeList... TLs >
struct ZipHelper;

template< TypeList L, TypeList R >
struct Zip2Helper;

template< TypeList L, TypeList R >
struct Zip2Helper {
    using Head = Cons<typename L::Head, typename R::Head>;
    using Tail = Zip2Helper<typename L::Tail, typename R::Tail>;
};

template< TypeList L, TypeList R >
    requires (Empty<L> || Empty<R>)
struct Zip2Helper<L, R> : Nil {};

template< TypeSequence TL, TypeList... TLs >
struct ZipHelper<TL, TLs...> {
	using Answer = Zip2Helper<TL, ZipHelper<TLs...>>;
	using Head = typename Answer::Head;
	using Tail = typename Answer::Tail;
};

template< TypeSequence TL >
	requires requires { !TypeTuple<typename TL::Head>; }
struct ZipHelper<TL> {
	using Head = Cons<typename TL::Head, Nil>;
	using Tail = ZipHelper<typename TL::Tail>;
};

template< TypeSequence TL >
struct ZipHelper<TL> {
        using Head = typename TL::Head;
        using Tail = ZipHelper<typename TL::Tail>;
};

template< Empty E >
struct ZipHelper<E> : Nil {
	using Head = Nil;
	using Tail = Nil;
};

template< TypeList... TLs >
using Zip = Map<ToTuple, ZipHelper<TLs...>>;

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

