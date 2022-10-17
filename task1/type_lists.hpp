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
struct ToTupleHelper;

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

/*template< TypeSequence TL, class... Ts >
struct ToTupleHelper<Ts..., TL> {
	using TypeListed = FromTuple<TTuple<Ts...>>;
	using Value = typename ToTupleHelper<TypeListed, TL>::Value;
};*/

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
struct Map;

template< template<class> class F, TypeSequence TL >
struct Map<F, TL> {
    using Head = F<typename TL::Head>;
    using Tail = Map<F, typename TL::Tail>;
};

template< template<class> class F, Empty E >
struct Map<F, E> : Nil {};

// Filter
template< template<class> class P, TypeList TL >
struct Filter;

template< template<class> class P, TypeSequence TL >
    requires (requires { typename P<typename TL::Head>::Value; })
struct Filter<P, TL> {
    using Head = std::conditional_t<P<typename TL::Head>::Value, typename TL::Head, Nil>;
    using Tail = Filter<P, typename TL::Tail>;
};

template< template<class> class P, Empty E >
struct Filter<P, E> : Nil {};

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
struct Inits;

template< TypeList TL >
struct Inits {
    using Head = Nil;
    using Tail = Cons<Cons<Head, typename TL::Head>, Inits<typename TL::Tail>>;
};

template< Empty E >
struct Inits<E> : Nil {};

// Tails
template< TypeList TL >
struct Tails;

template< TypeList TL >
struct Tails {
    using Head = TL;
    using Tail = Tails< typename TL::Tail>;
};

template< Empty E >
struct Tails<E> : Nil {};

// Scanl
template< template<class, class> class OP, class T, TypeList TL >
struct Scanl;

template< template<class, class> class OP, class T, TypeList TL>
struct Scanl {
    using Head = T;
    using Tail = Cons< OP<Head, typename TL::Head>, Scanl<OP, typename TL::Head, typename TL::Tail> >;
};

template< template<class, class> class OP, class T, Empty E >
struct Scanl<OP, T, E> : Nil {};

// Foldl
template< template<class, class> class OP, class T, TypeList TL >
struct FoldlHelper;

template< template<class, class> class OP, class T, TypeList TL >
struct FoldlHelper {
    using Value = typename FoldlHelper< OP, OP<T, typename TL::Head>, typename TL::Tail>::Value;
};

template< template<class, class> class OP, class T, TypeList TL >
    requires (Empty<TL> || Empty<typename TL::Tail>)
struct FoldlHelper<OP, T, TL> : Nil {};

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
struct Zip;

template< TypeList L, TypeList R, TypeList... TLs >
struct Zip<L, R, TLs...> {
    using Answer = Zip<Zip2<L, R>, TLs...>;
    using Head = Answer::Head;
    using Tail = Answer::Tail;
};

template< TypeList TL >
struct Zip<TL> : Nil {};

template< Empty E >
struct Zip<E> : Nil {};

} // namespace type_lists
