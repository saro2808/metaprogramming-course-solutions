Task 0. Slice-n-dice
========================

## Preparation

Recall what templates and explicit specializations (full and partial) are.

Recall that the `std::span` template from the standard library is used as a non-owning reference to several contiguous objects in memory. It serves as the “greatest common denominator” between different types of [contiguous containers](https://en.cppreference.com/w/cpp/named_req/ContiguousContainer): by using it as a function argument, you allow the user to pass to your function both `std::vector` from an arbitrary allocator and `std::array` of any size, as well as various wrappers such as [`std::flat_map`](https://en.cppreference.com/w/cpp/header/flat_map), and even containers from other libraries, of which there are many in practice.

Remember the existence of cast operators and user deduction guides.

Note that `std::span` has not one but two template arguments: the element type and the size. The size can be specified as `std::dynamic_extent`, which means “the size is not known at compile time, but will be known at runtime.”

## Task

### Slice

In this task, you need to write a more general version of `std::span`: `Slice`, which supports an additional template parameter stride — the size of the step when moving to the next element. For example,
```c++
    std::array<int, 10> data;
    std::iota(data.begin(), data.end(), 0);
    slice<int, 5, 2> even_elements{data};
```
Here, the contents of `even_elements` should be `0, 2, 4, 6, 8`. Your implementation should also support the runtime value of both parameters or either one of them when substituting into the `std::dynamic_extent` and `dynamic_stride` templates, respectively.

### Slice Interface

Requirements for the `Slice` template -- repeat the interface [`std::span`](https://en.cppreference.com/w/cpp/container/span), with the following exceptions:
* `size_bytes`
* `as_bytes`
* `as_writable_bytes`
* `subspan`
These methods imply `stride == 1`, so we will ignore them for now. Also ignore the “Helper templates” section for now. Finally, `std::span` constructors are quite a delicate science, so it is recommended not to copy them one-to-one from cppreference, but to write your own from scratch so that the tests pass.
Next, we will need a number of additions related to the `stride` parameter:
* `Skip` methods that allow you to change `stride`
* `DropFirst` methods that discard the first few elements
* `DropLast` methods that discard the last few elements
* Modification of the constructor from the iterator and size, which also accepts `stride`
Note that we do not require a `Subslice` method similar to `subspan`. Attempting to write such a method, taking into account all combinations of runtime and compile-time parameters, leads to a combinatorial explosion, so instead we provide the more elementary methods `First`, `Last`, `DropFirst`, `DropLast`, and `Stride`. For each of them, create two overloads

Note that `const Slice<T>` and `Slice<const T>` are not the same thing! The former prohibits changing the current variable of type `Slice`, while the latter prohibits changing the data pointed to by the current variable.

We also require that `Slice` with static parameters be implicitly cast to a similar slice with dynamic parameters (both simultaneously and only one separately). Casting `Slice<T>` to `Slice<const T>` must also be implicit.

Another mandatory requirement is that the size of each `Slice` specialization must be as small as possible. That is, you cannot store data about `extent` and `stride` at runtime if they are known at compile time. Think about how to achieve this without solving the whole problem four times over. Maybe inheritance will help? Don't be afraid to use the full power of C++ and even completely change the solution template. The main thing is to pass the tests.

Do not clutter the global namespace with auxiliary templates; place them in a separate namespace.

Finally, do not forget to familiarize yourself with the [tests](https://github.com/Mrkol/metaprogramming-course/blob/master/tests/task0).

## Formalities

**Deadline:** 04:00 on October 14, 2022.

**Points:** 2.

The `Slice` template must be available in the global namespace when the `slice.hpp` header file is included. Please note that creating additional files and classes is not prohibited. `cpp` files in the folder will be automatically compiled and linked to the tests, although they are unlikely to be useful in this task.

Push the code to the `task0` branch and do a pull request to `master`.