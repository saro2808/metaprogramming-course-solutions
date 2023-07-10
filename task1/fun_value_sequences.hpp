#pragma once

#include <value_types.hpp>

using value_types::ValueTag;
using type_lists::Nil;

// Nats
template<int Start>
struct NatsHelper {
	using Head = ValueTag<Start>;
	using Tail = NatsHelper<Start + 1>;
};

// Fibs
template<int First, int Second>
struct FibHelper {
	using Head = ValueTag<First>;
	using Tail = FibHelper<Second, First + Second>;
};

// Primes
template<int N, int Divisor>
struct IsPrimeHelper {
	static constexpr bool Value = true;
};

template<int N, int Divisor>
	requires (N > Divisor)
struct IsPrimeHelper<N, Divisor> {
	static constexpr bool Value = (N % Divisor == 0) ? false : IsPrimeHelper<N, Divisor + 1>::Value;
};

template<int N>
constexpr bool isPrime = IsPrimeHelper<N, 2>::Value;

template<int Current>
struct PrimesHelper;

template<int Current>
struct PrimesHelper {
        using Answer = PrimesHelper<Current + 2>;
        using Head = typename Answer::Head;
        using Tail = typename Answer::Tail;
};

template<int Current>
	requires isPrime<Current>
struct PrimesHelper<Current> {
	using Head = ValueTag<Current>;
	using Tail = PrimesHelper<Current + 2>;
};

template<>
struct PrimesHelper<2> {
	using Head = ValueTag<2>;
	using Tail = PrimesHelper<3>;
};

using Nats = NatsHelper<0>;
using Fib = FibHelper<0, 1>;
using Primes = PrimesHelper<2>;
