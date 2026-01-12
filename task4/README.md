Task 4. Enumerating enumerators
========================

## Preparation

Ensure you are using the latest stable version of GCC or Clang. Older versions will likely work as well, but I have not tested them.

Recall what [scoped and unscoped enumerations](https://en.cppreference.com/w/cpp/language/enum) are and their internal types.

Experiment with the contents of the built-in variable [\_\_PRETTY_FUNCTION\_\_](https://gcc.gnu.org/onlinedocs/gcc/Function-Names.html) in template functions. Try making some enum one of the non-typical template parameters. What will this macro output if you cast an arbitrary number to an enum and put it in the template argument?

Note that all `std::string_view` methods are declared `constexpr`.

## Task

Implement the template class `EnumeratorTraits` with template parameters and public methods as in the solution template.

Let `Enum` be a scoped or unscoped enumeration. We will call the value of the enumerator `e` of type `Enum` `static_cast<std::underlying_type_t<Enum>>(e)`.

- `size()` returns the number of enumerators `Enum`,
- `at(i)` returns the i-th enumerator in ascending order of their values,
- `nameAt(i)` returns the name of the i-th enumerator in ascending order of their values.

**It is guaranteed that**

- for unscoped enumerations, the internal type is explicitly specified (find out why it won't work otherwise),
- the value of any enumerator modulo does not exceed `MAXN`,
- the values of all enumerators are distinct,
- the argument of the `nameAt` and `at` functions lies in the range from `0` to `size() - 1` inclusive.

Otherwise, the behavior is undefined.

**Asymptotic requirements**

- The functions `size`, `at`, and `nameAt` must run in constant time relative to `MAXN` and the number of enumerators.
- The memory allocated during instantiation may depend linearly on `MAXN`. This refers to the data that you yourself store in fields or variables. We are not interested in what happens inside the compiler.
- There are no requirements for compilation time during instantiation.

> Lyrical digression
>
> Generally speaking, if you require linear memory consumption at the compilation stage, you cannot use the standard recursive approach to working with parameter packs.
>
> The compiler uses the list of template arguments as a key when caching template instances. With the usual biting off of several parameters from the head of the pack and recursive instantiation from the tail, the total number of template arguments in all instances is the sum of an arithmetic progression — depends quadratically on the length of the original pack.
>
> However, recursion is not necessary to solve this problem.

For `MAXN <= 512`, your code should compile with the latest stable versions of Clang and GCC without specifying flags that control the depth of template recursion and constexpr functions, as well as the number of steps when calculating constexpr functions.

## Example

```cpp
enum Fruit : unsigned short {
    APPLE, MELON
};

enum class Shape {
    SQUARE, CIRCLE = 5, LINE, POINT = -2
};

static_assert(EnumeratorTraits<Shape>::size() == 4);
static_assert(EnumeratorTraits<Fruit>::size() == 2);

static_assert(EnumeratorTraits<Fruit>::nameAt(0) == "APPLE");
static_assert(EnumeratorTraits<Fruit>::nameAt(1) == "MELON");

static_assert(EnumeratorTraits<Shape>::nameAt(0) == "POINT");
static_assert(EnumeratorTraits<Shape>::at(0) == Shape::POINT);
static_assert(EnumeratorTraits<Shape>::at(1) == Shape::SQUARE);
static_assert(EnumeratorTraits<Shape>::at(3) == Shape::LINE);
```

## Formalities

**Deadline:** 04:00 on December 3, 2022.

**Points:** 3.

The `EnumeratorTraits` class must be accessible in the global namespace when connecting the `EnumeratorTraits.hpp` header file. This header file should be located in the `task4` folder at the root of the repository.

Push the code to the `task4` branch and make a pull request to `master`.

Use the full power of the standard library, including type traits and the concepts library, if necessary.