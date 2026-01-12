Task 3. A very clever pointer
========================

## Preparation

Refresh your memory on [the use of concepts and constraints](https://en.cppreference.com/w/cpp/language/constraints) and [object concepts of the standard library](https://en.cppreference.com/w/cpp/concepts).

Recall the ways to implement type erasure.

Recall how “boxes” for values are written in C++ from the point of view of the external interface (e.g., `std::optional`, `std::unique_ptr`). What operators need to be supported? What constructors need to be written?

## Task

Implement a template class Spy that wraps an arbitrary object and logs accesses to it.

The wrapped object must be stored *by value*. Two years ago, it was rightly noted that it is incorrect to call such entities pointers, so we will call them smart non-pointers.

If `s` is of type `Spy<T>`, the expression `s->member` should result in an access to the non-static member `member` of the wrapped object.

### Loggers

The `setLogger` method sets a logger. After each expression containing references to the wrapped object via `operator ->` is evaluated, the logger should be called if it is set. The logger takes the number of references to the object during the evaluation of the expression as an argument.

Accesses via `operator *` are not logged.

If the wrapped type `T` is not copyable, then `Spy<T>` must support move-only loggers.

If a wrapped object is accessed and the logger is changed in a single expression, the behavior is undefined.

Using `std::function` and `std::any` is prohibited in this task.

### Saving concepts

Let's say that a smart pointer `W` preserves concept `C` if, for any type `T`

1) `T` satisfies `C` &rArr; `W<T>` satisfies `C`,
2) `T` models `C` &rArr; `W<T>` models `C`.

Your `Spy` should preserve the basic object concepts: `std::movable`, `std::copyable`, `std::semiregular`, `std::regular`. To do this, you can impose additional restrictions on the template argument of the `setLogger` method.

Comparison operators should compare wrapper objects, ignoring the logger. When copying (moving), both the logger and the wrapper object should be copied (moved).

If a single expression references the wrapped object and moves a non-pointer, the behavior is undefined. Copying creates a new object, references to which must be considered separately.

### Bonus: allocators and small buffer optimization (+2)

Support custom allocators and small buffer optimization for loggers. Note that we are writing in C++20, and instead of placement new, we should use the `std::allocator_traits<Alloc>::construct` method, which in turn will call `std::construct_at`, and instead of directly calling the destructor, we should use the `std::allocator_traits<Alloc>::destroy` method, which in turn will call `std::destroy_at`. Furthermore, since we do not know the type of the logger at the time of creating `Spy`, we will have to use an allocator for the type `std::byte`. However, the functions `construct` and `destroy` accept a template parameter for the type to be constructed, so the same allocator can allocate memory for and construct any loggers.

Check out the named set of requirements [AllocatorAwareContainer](https://en.cppreference.com/w/cpp/named_req/AllocatorAwareContainer). Read an interesting [article](https://www.foonathan.net/2015/10/allocatorawarecontainer-propagation-pitfalls/) about the funny traits `propagate_on_container_copy_assignment` and `propagate_on_container_move_assignment`. Don't forget to reallocate memory in case of a move assignment of `Spy` with a false `propagate_on_container_move_assignment` and different allocators.

### Bonus 2: generalized virtual call table (no points, no tests, for madmen)

If you have nothing else to do, come up with (or steal) a design and implement a generalized mechanism for virtual call tables. As a result, the Spy class should be able to “remove” the deleted move function at the user's request through the “move-only” policy and weigh a few bytes less. It should also be possible to move the virtual call table to static storage (in which case virtual calls will work in 2 indirections, but Spy itself will weigh even less).

## Example

```c++
struct Holder {
    int x = 0;
    bool isPositive() const {
        return x > 0;
    }
};

Spy s{Holder{}};
static_assert(std::semiregular<decltype(s)>);

s.setLogger([](auto n) { std::cout << n << std::endl; });

s->isPositive() && s->x--; // prints 1
s->x++ + s->x++; // prints 2
s->isPositive() && s->x--; // prints 2

s.setLogger([dummy = std::unique_ptr<int>()](auto n) {}); // compilation error

// -----------------------------------

struct MoveOnly {
    MoveOnly() = default;

    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator =(MoveOnly&&) = default;

    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator =(const MoveOnly&) = delete;

    ~MoveOnly() noexcept = default;
};

Spy t{MoveOnly{}};

s.setLogger([dummy = std::unique_ptr<int>()](auto n) {}); // ok

static_assert(std::movable<decltype(t)>);
static_assert(!std::copyable<decltype(t)>);
```

## Questions to ponder

1. What problems would we encounter if we wanted to add a constant version of the `->` operator?

2. How would you modify the `Spy` class to support access to the wrapped object from multiple threads?

3. Can we neatly handle all cases of `noexcept` annotations on deleted logger methods?

4. We would like the counter to be preserved when moving within an expression, as if the calls were made to the same object:

   `(a->doSmth(), (b = std::move(a))->doSmth()); // 2`

   Think about how this can be implemented without dynamic memory allocation. Why would this solution work? If you want, implement it.

## Formalities

**Deadline:** 04:00 14.11.2020.

**Points:** 3 (+2).

Push the code to the `task3` branch and make a pull request to `master`.

Use the full power of the standard library, except for `std::function` and `std::any`.

Answering the questions for reflection is optional.

## Tests

The tests for this task are collected with address and UB sanitizers. Be careful when doing the bonus level!