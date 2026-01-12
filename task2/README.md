Task 2. Sounds of nature
========================

## Preparation

With C++ 20, the type of non-type template parameters can be a class. Previously, only integral types, pointers, references, and enums could be passed in templates. That is, you could write:

```c++
struct Data {
    int a;
    char b;
};

template <Data input>
class StaticDataHolder {};

StaticDataHolder<{5, ‘c’}> holder;
```

Of course, there are [a bunch of restrictions](https://en.cppreference.com/w/cpp/language/template_parameters#Non-type_template_parameter). This feature has been supported in GCC since version 9. It will come in handy for us.

Recall how [user-defined operators](https://en.cppreference.com/w/cpp/language/user_literal) are defined for string literals.

Recall how `dynamic_cast` works. What is the difference between `dynamic_cast<T*>(ptr)` and `dynamic_cast<T&>(value)`?

Recall how to use [variadic templates](https://en.cppreference.com/w/cpp/language/parameter_pack).

**Make sure you are using GCC 10.**

## Task

### Mapping polymorphic types to objects

Let's say we have a polymorphic hierarchy of classes inheriting from `Base`. Each class in this hierarchy is associated with a certain value of type `Target`. Given a reference `Base& object`, we want to find the value corresponding to the actual type of `object`.

We propose creating a generalized mechanism that allows us to specify such mappings.

You need to implement the `Mapping` and `PolymorphicMapper` classes, as well as the static method `PolymorphicMapper::map` in the `string_mapper.hpp` file.

When calling `map(object)`, the mapper should search among `Mappings...` for the first `Mapping<From, target>` such that the actual type of `object` is From or a descendant of `From`, and return `target`. If no such mapping is found, `std::nullopt` is returned.

The code should **not** compile if, when instantiating `PolymorphicMapper<Base, Target, Mappings...>::map`, at least one type from the `Mappings...` pack is not `Mapping<SomeClass, Target some_value>`, where `SomeClass` inherits from `Base`. Most likely, you will succeed naturally, but it is better to provide a reasonable error message for the user.

It is guaranteed that when instantiating, mappings are passed sorted in order of inheritance: descendants come before superclasses. Otherwise, the behavior is undefined.

If you don't understand anything, read the next section and then the example.

### Bonus level for the experienced (+1)

If you don't want to suffer, skip this section.

Make your mapper work even if the mappings are passed in a completely random order. See the tests in the `unsorted.hpp` file.

**At runtime, the mapper must work in O(n) dynamic_cast calls.**

### Strings at compile time

1. Implement the template class `FixedString` with one non-type template parameter `size_t max_length`, a constructor with two arguments `const char* string, size_t length`, and an implicit cast operator to `std::string_view`. The class should store the first `length` characters of `string` and return them when cast to `string_view`. If `length > max_length`, the behavior is undefined.

2. Implement the operator `“”_cstr`, which returns a FixedString<256> containing the literal. If the length of the literal is greater than 256, the behavior is undefined.

3. Make `FixedString` and `“”_cstr` usable at compile time.
4. Make objects of type `FixedString` passable as non-type template parameters.

Pay attention to the assertions at the very beginning of the main test file. They should definitely pass, but they are not enough for the class to be used as a type of non-typical template parameters. See the link above.

## Example

Let's imagine that we have a hierarchy of classes representing animals. We receive a pointer to some animal and need to understand what sound it makes. To do this, we want to write the following code.

```cpp
class Animal {
public:
    virtual ~Animal() = default;
};

class Cat : public Animal {};
class Cow : public Animal {};
class Dog : public Animal {};

using MyMapper = PolymorphicMapper<
    Animal, FixedString<256>,
    Mapping<Cat, "Meow"_cstr>,
    Mapping<Dog, "Bark"_cstr>,
    Mapping<Cow, "Moo"_cstr>
>;


std::unique_ptr<Animal> some_animal{new Dog()};
std::string_view dog_sound = *MyMapper::map(*some_animal);
ASSERT_EQ(dog_sound, "Bark");
```

Also check out [the tests](https://github.com/Mrkol/metaprogramming-course/blob/master/tests/task2/main.cpp).

## Formalities

**Deadline:** 04:00 on October 28, 2022.

**Points:** 2+1.

Push the code to the `task2` branch and make a pull request to `master`.

Use the full power of the standard library, including type traits and the concepts library, if necessary.

Don't be afraid to change the solution template — it is deliberately minimalistic.

## Background

The author encountered a similar task when it was necessary to convert C++ exceptions to Java exceptions and throw them via JNI. That is, there is a fairly large hierarchy of C++ exceptions, the exception is caught by a reference to the base class object, and then you need to get the corresponding Java class name.

Prior to C++ 20, this could be implemented using the [GNU extension of the operator “”](https://habr.com/ru/post/243581/).