#pragma once

#include <concepts>

#include <type_tuples.hpp>

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
// template< class T, class... TL >
// struct Cons {
//     using type = Cons<T, typename Cons<TL...>::type>::type;
// };

// template< class T, Empty E >
// struct Cons<T, E> {
//     using type = T;
// };

// template< Empty E >
// struct Cons<E> {};
template<class T, class U>
struct Cons;

// FromTuple
template< template< class... > class TT, class... Ts >
    requires TypeTuple<TT<Ts...>>
struct FromTuple;

template< template< class, class... > class TT, class T, class... Ts >
    requires TypeTuple<TT<T, Ts...>>
struct FromTuple<TT, T, Ts...> {
    using Head = T;
    using Tail = FromTuple<TT, Ts...>;
};

template< template<class> class TT >
struct FromTuple<TT, Nil> : Nil {};

// ToTuple
template< TypeList TL >
struct ToTuple;

// Repeat
template<class T>
struct Repeat {
    using Head = T;
    using Tail = Repeat<T>;
};

// Take
template< int N, TypeList TL >
    requires (N >= 0)
struct Take;

template< int N, TypeSequence TL >
    requires (N > 0)
struct Take<N, TL> {
    using Head = Cons<typename TL::Head, typename Tail>;
    using Tail = Take<N - 1, typename TL::Tail>;
};

template< int N, TypeList TL >
    requires (N == 0 || Empty<TL>)
struct Take<N, TL> : Nil {};

// Drop
template< int N, TypeList TL >
    requires (N >= 0)
struct Drop;

template< int N, TypeSequence TL >
    requires (N > 0 && !Empty<typename TL::Tail>)
struct Drop<N, TL> {
    using Answer = Drop<N - 1, typename TL::Tail>;
    using Head = typename Answer::Head;
    using Tail = typename Answer::Tail;
};

template< TypeList TL >
struct Drop<0, TL> {
    using Head = typename TL::Head;
    using Tail = typename TL::Tail;
};

template< int N, TypeList TL >
    requires (Empty<TL> || Empty<typename TL::Tail>)
struct Drop<N, TL> : Nil {};

// Replicate
template< int N, class T >
    requires (N >= 0)
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
    requires (requires { typename P<typename TL::Head>::Value; })
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
    using Head = TL;
    using Tail = Cycle<TL>;
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
    using Tail = Cons< typename OP<Head, typename TL::Head>
                     , Scanl<OP, typename TL::Head, typename TL::Tail>
                     >;
};

template< template<class, class> class OP, class T, Empty E >
struct Scanl<OP, T, E> : Nil {};

// Foldl
template< template<class, class> class OP, class T, TypeList TL >
struct FoldlHelper;

template< template<class, class> class OP, class T, TypeList TL >
struct FoldlHelper {
    using Value = typename FoldlHelper< OP
                                      , typename OP<T, typename TL::Head>
                                      , typename TL::Tail
                                      >::Value;
};

template< template<class, class> class OP, class T, TypeList TL >
    requires (Empty<TL> || Empty<typename TL::Tail>)
struct FoldlHelper<OP, T, TL> : Nil {};

template< template<class, class> class OP, class T, TypeList TL >
using Foldl = typename FoldlHelper::Value;

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
struct Zip2 : Nil {};

// Zip
template< TypList... TLs >
struct Zip;

template< TypeList TL, TypList... TLs >
struct Zip {
};

} // namespace type_lists
