Task 1. Infinite Laziness
========================

## Preparation

At the seminar, we played around with tie lists. Recall that there are several ways to define them. In this task, we will develop the “old” method using modern technologies.

Recall how instantiation works for classes: instantiation of declarations, instantiation of definitions, instantiation of their members.

Recall what constraints and concepts are, what a concept can be from a philosophical point of view, how constraints interact with explicit specializations and overloads.

Remember that you can extract various entities from the parent class into the child class using `using BaseClassName::EntityName;` (works for functions, fields, and *aliases*).

Remember that you can specialize class templates and overload functions using *constraints*. However, keep in mind that compilers' ability to see implications between sets of constraints is still quite limited, and sometimes you have to fight to get the desired result.

## Task

### Working with infinite lists of types

Consider the following concept.
```c++

template<class TL>
concept TypeSequence =
    requires {
        typename TL::Head;
        typename TL::Tail;
    };
```

It is claimed that it describes *infinite type sequences*. TL::Tail is also implicitly required to satisfy the TypeSequence concept, but it is most likely impossible to model this in C++20.

But how can a type sequence be infinite if the compiler's memory is finite?

The answer is the same as in functional languages: lazy evaluation. The compiler lazily instantiates class template members, including aliases such as Tail, which means they can even refer to the current template, but with different arguments.

Example:
```c++
template<class T>
struct FunnyStarrySequence {
    using Head = T;
    using Tail = FunnyStarrySequence<T*>;
};
```
Instances of this template describe infinite lists of the form `(T, T*, T**, T***, T****, ...)`. The entire list is not stored in the compiler's memory; only the scheme for obtaining the next elements is stored. How many times you take the next element is how many times it will be calculated.

But it is inconvenient to work with infinite sequences of types alone, so let's consider the following code.
```c++
struct Nil {};

template<class TL>
concept Empty = std::derived_from<TL, Nil>;

template<class TL>
concept TypeList = Empty<TL> || TypeSequence<TL>;
```

We introduced the concept of `Empty`, which describes an empty list as an arbitrary descendant of `Nil`, as well as `TypeList`, which describes a *potentially infinite list* as either empty or a sequence. Note that the implicit requirement for `TL::Tail` needs to be changed: it must satisfy the more general concept of `TypeList`, rather than specifically `TypeSequence`. A more correct approach would be to define `TypeList` independently of TypeSequence and treat them as independent entities. However, the chosen approach allows the compiler to guess that any sequence is also a list, which can be useful in practice when overloading functions.

So, your task is to implement a set of interesting operations for working with infinite lists, figuring out how to implement specific types of tuples in the process.

### “New” type lists

From now on, we will call them type tuples to better distinguish them from potentially infinite lists. In this task, we will also need tuples in some places, but we will not write very complex algorithms for them. Recall that they are defined as
```c++
template<class... Ts>
struct TTuple {};
```
Note that all different tuples can be combined into a single family using the `TypeTuple` concept. Take a look at its implementation in the template and think about what else you can do using this technique.

### Part 1: Operations

The following functions should be located in the `type_lists` namespace.

 * `Cons<T, TL>` — a list that starts with `T`, followed by elements of the list `TL`.
 * `FromTuple<TT>`/`ToTuple<TL>` — functions for converting between **finite** lists and tuples. They will help you with debugging.
 * `Repeat<T>` -- an infinite list of `T`.
 * `Take<N, TL>` -- the first `N` elements of a potentially infinite list `TL`.
 * `Drop<N, TL>` -- everything except the first `N` elements of the list `TL`.
 * `Replicate<N, T>` -- a list of `N` elements equal to `T`.
 * `Map<F, TL>` -- a list of the results of applying `F` to the elements of `TL`.
 * `Filter<P, TL>` — a list of only those elements of `TL` that satisfy `P<_>::Value`. The relative order of the elements must not change.
 * `Iterate<F, T>` — a list in which each subsequent element is the result of applying the metafunction `F` to the previous one, and the first element is `T`.
 * `Cycle<TL>` -- an infinite list in which the finite list `TL` is repeated over and over again.
 * `Inits<TL>` -- a list of all prefixes of `TL` in ascending order of length.
 * `Tails<TL>` -- a list of all suffixes of `TL` in ascending order of the length of their complement to the entire list.
 * `Scanl<OP, T, TL>` -- a sequence in which the first element is `T`, and each subsequent element is obtained by applying `OP<_, _>::Type` to the current and next elements of `TL`.
 * `Foldl<OP, T, TL>` -- a type obtained as `OP<... OP<OP<T, TL[0]>, TL[1]> ... >`. If the sequence is infinite, the value is undefined.
 * `Zip2<L, R>` — a list of pairs of consecutive elements from lists `L` and `R`, respectively.
 * `Zip<TL...>` — a list of tuples with one element of a fixed number from each list.

 Bonus level (+1 point):

* `GroupBy<EQ, TL>` -- a list of lists of **consecutive** elements `TL` that are **sequentially** “equal,” i.e., each subsequent element must be equal to the current one (this allows, for example, searching for ascending subsequences). Equality is understood in the sense of `EQ<T, S>::Value == true`. For example, group the letters in the word “Mississippi” by equality -- `[“M”,“i”,“ss”,“i”,“ss”,“i”,‘pp’,“i”]`. If all elements of an *infinite* sequence `TL` are equal, the behavior is undefined (think about what prevents us from defining it). Here, `EQ` is a metapredicate that satisfies the axioms of equality.

### Part 2: Numeric sequences

Place the contents of this section in the global namespace.

Let's introduce a new class:
```c++
template<auto V>
struct ValueTag{ static constexpr auto Value = V; };
```
With its help, we can store values inside types, which allows us to work with infinite lists of values at compile time. Calculate the following types:

* `Nats` -- natural numbers `(0, 1, 2, 3, ...)`
 * `Fib` -- Fibonacci numbers `(0, 1, 1, 2, ...)`
 * `Primes` -- prime numbers `(2, 3, 5, 7, ...)`

Use already implemented functions whenever possible. Place their definitions in the `fun_value_sequences.hpp` file. 

## Examples and tests

In general, comparing infinite sequences for equality boils down to solving the halting problem, so for debugging, proceed as in the [tests](https://github.com/Mrkol/metaprogramming-course/blob/master/tests/task1/main.cpp): cut off a piece of the infinite list, convert it to a tuple, and compare them using `std::is_same`. If there are other lists inside the list, the cutting and conversion must be done recursively using `Map` and the curried version of `Take`.

## Formalities

**Deadline:** 04:00 on October 28, 2022.

**Points:** 4 + 1.

Push the code to the `task1` branch and make a pull request to `master`. Don't forget to add a reviewer to the reviewers.

A mandatory requirement is to apply constraints to template arguments as much as possible and, if appropriate, to extract sets of constraints into concepts.

## Background

Once, at a seminar on taillists, while showing a reference to Data.List in Haskell and scrolling through functions for infinite lists, I said, “This is not right; we don't know how to do this.” It turns out we do! After several days of experimentation, it was discovered that infinite lists can indeed be implemented, and it looks quite good. Of course, the usefulness of this “invention” is questionable, but it's perfect for training.
