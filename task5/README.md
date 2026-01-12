Task 5. Annotation processing
========================

## Background

With introspection, you can generalize solutions to many problems. Boost.PFR can generate `std::hash` specialization, stream input and output operators, and comparison operators for supported structures. Other use cases: binary serialization, ORM.

Sometimes it can be useful to provide a function that uses introspection with some additional information about a specific field. For example, to specify the name of a column in a database or to exclude a field from consideration in comparison operators. In the Java ecosystem, annotations are used to pass such contextual information. We will try to implement reflection support with annotations in C++.

C++ has two built-in ways to pass contextual information: preprocessor directives and attributes. Neither of these is suitable for us, as their use would require the development of external tools that go beyond the scope of the language. Instead, we will use other fields to annotate fields:

```cpp
struct S {
    int x; // no annotation
    
    Annotate<Transient> _annot1;
    char y; // annotated with Transient
    
    Annotate<Transient> _annot2;
    Annotate<NoCompare> _annot3;
    float z; // annotated with Transient, NoCompare
    
    Annotate<Transient, NoCompare> _annot4;
    float w; // annotated with Transient, NoCompare
};
```

Annotations can have arguments. We will pass them as template parameters:

```cpp
struct MySerializableStruct {
    int x;

    Annotate<Checksum<Crc32>> _annot1;
    uint32_t checksum; // annotated with Checksum with parameter Crc32
};
```

## Preparation

Recall how various operations are performed on type lists. The most basic ones are [implemented in the standard library](https://en.cppreference.com/w/cpp/header/tuple) for `std::tuple`.

Recall the pairs about reflection in C++.

Recall how to use [template template parameters](https://en.cppreference.com/w/cpp/language/template_parameters).

## Task

Consider the template class `Annotate`:

```cpp
template <class...>
class Annotate {};
```

Let's assume that all fields of type `Annotate` describe annotations to the next field that has a type other than `Annotate`.

Implement the `Describe` template:

```cpp
template <class T>
struct Describe {
    static constexpr size_t num_fields = /* number of fields */
    
    template <size_t I>
    using Field = /* field descriptor (see below) */;
};
```

`num_fields` — the number of fields in structure `T`, excluding fields of type `Annotate`.

For each `I` from 0 to `num_fields - 1`, `Field<I>` — a type describing the `I`-th field in the order of declaration in structure `T`, excluding annotation fields, with the following members:

```cpp
struct /* unspecified (field descriptor) */ {
    using Type = /* type of field */;
    using Annotations = Annotate</* all annotations of the field */>;

    template <template <class...> class AnnotationTemplate>
    static constexpr bool has_annotation_template = /* see below */;
    
    template <class Annotation>
    static constexpr bool has_annotation_class = /* see below */;

    template <template <class...> class AnnotationTemplate>
    using FindAnnotation = /* see below */;
};
```

`has_annotation_template<AnnotationTemplate>` is true when there is an `AnnotationTemplate` with arbitrary template arguments among the field annotations.

`has_annotation_class<Annotation>` is true when there is an `Annotation` among the field annotations.

`FindAnnotation<AnnotationTemplate>` — an instance of `AnnotationTemplate` with the parameters it encounters in the list of field annotations. If `AnnotationTemplate` occurs multiple times, `FindAnnotation` — any of the matching instances. If there is no `AnnotationTemplate` among the annotations, using `FindAnnotation<AnnotationTemplate>` should result in a substitution failure.

Use any suitable technique to determine the number and types of fields. You can see the implementation in the [presentation](https://docs.google.com/presentation/d/1Ot2yhAgjUtD7fbxGqstdyzLSq_AGNoFdUNgmkQL9wFs/edit?usp=sharing) or in [Boost.PFR](https://github.com/apolukhin/magic_get). (Please do not copy and paste from Boost.PFR. At the very least, the code style there is not compatible with ours.) It is not necessary to obtain field values or references to them in this task.

### Restrictions on supported structures

Only classes that meet the following conditions can be used as template arguments for `Describe`:

- not unions,
- no user constructors or destructors,
- no virtual methods,
- no base classes,
- all fields are public, have no default values, and their types are
  - either scalar,
  - or meet these requirements.

Otherwise, the behavior is undefined.

The solution may limit the maximum number of fields, but classes containing no more than 100 fields, including annotation fields, must be supported. If the passed structure contains more fields than the solution allows, a compilation error must be thrown.

If the last field of the structure is of type `Annotate`, the behavior is undefined.

### Bonus level

Using stateful metaprogramming, implement support for an arbitrary number of fields. Bonus solutions without stateful metaprogramming will be banned at the code review stage.

## Example

```cpp
// some macro magic
// to avoid handmade names for annotation fields

#define MPC_CONCAT(a,b)  a##b
#define MPC_ANNOTATION_LABEL(a) MPC_CONCAT(_annotion_, a)
#define MPC_ANNOTATE(...) Annotate<__VA_ARGS__> MPC_ANNOTATION_LABEL(__COUNTER__);

// MPC_ANNOTATE(A, B, C) expands to
// `Annotate<A, B, C> some_ugly_unique_name`

struct Transient;
struct Crc32;
struct NoIo;
struct NoCompare;

template <class Algorithm>
struct Checksum;

struct S {
    int x; // no annotation

    MPC_ANNOTATE(Transient, NoIo)
    char y; // annotated with Transient, NoIo

    MPC_ANNOTATE(Transient)
    MPC_ANNOTATE(NoCompare)
    float z; // annotated with Transient, NoCompare

    MPC_ANNOTATE(NoIo, Checksum<Crc32>)
    uint64_t w; // annotated with NoIo, Checksum<Crc32>
};


static_assert(std::is_same_v<Describe<S>::Field<0>::Type, int>);
static_assert(std::is_same_v<Describe<S>::Field<0>::Annotations, Annotate<>>);


static_assert(Describe<S>::Field<1>::has_annotation_class<NoCompare> == false);
static_assert(Describe<S>::Field<1>::has_annotation_template<Checksum> == false);
static_assert(Describe<S>::Field<1>::has_annotation_class<NoIo> == true);

static_assert(std::is_same_v<Describe<S>::Field<2>::Annotations, Annotate<Transient, NoCompare>>);

static_assert(Describe<S>::Field<3>::has_annotation_template<Checksum> == true);
static_assert(std::is_same_v<Describe<S>::Field<3>::FindAnnotation<Checksum>, Checksum<Crc32>>);
```

## Formalities

**Deadline:** 04:00 on December 10, 2022.

**Points:** 3+1.

The `Annotate` and `Descriptor` classes must be available in the global namespace when the `reflect.hpp` header file is included. This header file must be located in the `task5` folder at the root of the repository.

Push the code to the `task5` branch and make a pull request to `master`.

Use the full power of the standard library.