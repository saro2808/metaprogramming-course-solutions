#pragma once

#include <cstdint>
#include <utility>
#include <array>
#include <concepts>
#include <cstddef>
#include <tuple>

namespace type_lists {

template<class TL>
concept TypeSequence =
    requires {
        typename TL::Head;
        typename TL::Tail;
    };

struct Nil {};

template<class... Ts>
struct TTuple {};

template<class TL>
concept Empty = std::derived_from<TL, Nil>;

template<class TL>
concept TypeList = Empty<TL> || TypeSequence<TL>;

// ToTypeList
template <class... Ts>
struct ToTypeList : Nil {};

template <class T, class... Ts>
struct ToTypeList<T, Ts...> {
    using Head = T;
    using Tail = ToTypeList<Ts...>;
};

template <class T, class... Ts>
struct ToTypeList<TTuple<T, Ts...>> {
    using Head = T;
    using Tail = ToTypeList<Ts...>;
};

template<>
struct ToTypeList<TTuple<>> : Nil {};

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

template< TypeList TL >
using ToTuple = typename ToTupleHelper<TL>::Value;

// Append
template <TypeList L, TypeList R>
struct Append : Nil {};

template <TypeSequence L, TypeList R>
struct Append<L, R> {
    using Head = typename L::Head;
    using Tail = Append<typename L::Tail, R>;
};

template <Empty L, TypeSequence R>
struct Append<L, R> {
    using Head = typename R::Head;
    using Tail = typename R::Tail;
};

} // namespace type_lists

using type_lists::TypeList;
using type_lists::Empty;

template <class... Ts>
using Annotate = type_lists::TTuple<Ts...>;

namespace detail {

template <class T>
constexpr std::size_t isAnnotation = 0;

template <class... Ts>
constexpr std::size_t isAnnotation<Annotate<Ts...>> = 1;

// fields counter

template <class T, class... Args>
concept AggregateConstructibleFrom = requires(Args... args) {
    T{ args... };
};

template <std::size_t I>
struct UbiqConstructor {
    template <class Type>
    constexpr operator Type&() const noexcept;
};

template <class T, std::size_t... I>
constexpr size_t countFieldsImpl(std::index_sequence<I...>) {
    return sizeof...(I) - 1;
}

template <class T, std::size_t... I>
    requires AggregateConstructibleFrom<T, UbiqConstructor<I>...>
constexpr size_t countFieldsImpl(std::index_sequence<I...>) {
    return countFieldsImpl<T>(std::index_sequence<0, I...>{});
}

template <class T>
constexpr auto fieldsCount = countFieldsImpl<T>(std::index_sequence<>{});

// type deducer

template <class T, std::size_t N>
struct Tag {
    friend auto loophole(Tag<T, N>);    
};

template<class T, std::size_t N, class F>
struct LoopholeSet {
    friend auto loophole(Tag<T, N>) { return F{}; };
};

template <class T, std::size_t I>
struct LoopholeUbiq {
    template <class Type>
    constexpr operator Type() const noexcept {
        LoopholeSet<T, I, Type> unused{};
        return {};
    };
};

template<class T, std::size_t N>
struct LoopholeGet {
    using Type = decltype(loophole(Tag<T, N>{}));
};

template <class T, std::size_t... Is>
constexpr auto asTupleImpl(std::index_sequence<Is...>) {
    constexpr T t{ LoopholeUbiq<T, Is>{}... };
    return type_lists::TTuple<
        typename LoopholeGet<T, Is>::Type...
    >{};
}

template <class T>
using typesTuple = decltype(asTupleImpl<T>(std::make_index_sequence<fieldsCount<T>>{}));

template <int N, typename... Ts>
struct GetNth {
    using Type = type_lists::Nil;
};

template <int N, typename T, typename... Ts>
struct GetNth<N, type_lists::TTuple<T, Ts...>> {
    using Type = typename GetNth<N - 1, type_lists::TTuple<Ts...>>::Type;
};

template <typename T, typename... Ts>
struct GetNth<0, type_lists::TTuple<T, Ts...>> {
    using Type = T;
};

template <class T>
constexpr auto nonAnnotIndices() {
    std::array<std::size_t, fieldsCount<T>> nonAnnots;
    nonAnnots.fill(0);
    std::size_t nonAnnotIdx = 0;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        ([&]() {
            if constexpr (!isAnnotation<typename GetNth<I, typesTuple<T>>::Type>)
                nonAnnots[nonAnnotIdx++] = I;
        }(), ...);
    }(std::make_index_sequence<fieldsCount<T>>{});
    return nonAnnots;
}

template <class T>
constexpr auto typesTupleWithoutAnnots = nonAnnotIndices<T>();

template <class T>
constexpr std::size_t countNonAnnotFields() {
    if constexpr (fieldsCount<T> == 0)
        return 0;
    std::size_t sz = 0;
    for (int i = 0; i < fieldsCount<T>; ++i)
        if (typesTupleWithoutAnnots<T>[i] == 0)
            ++sz;
    return sz + 1;
}

template <class T, std::size_t I>
using FieldType = typename GetNth<I, typesTuple<T>>::Type;

template <class T, std::size_t I>
using NonAnnotFieldType = typename GetNth<typesTupleWithoutAnnots<T>[I], typesTuple<T>>::Type;

template <class T, std::size_t I>
using AnnotListAt = type_lists::ToTypeList<FieldType<T, I>>;

template <class T, std::size_t Begin, std::size_t End>
struct AnnotList : type_lists::Nil {};

template <class T, std::size_t Begin, std::size_t End>
    requires (Begin < End)
struct AnnotList<T, Begin, End> {
    using Value = type_lists::Append<
        AnnotListAt<T, Begin>,
        AnnotList<T, Begin + 1, End>
    >;
    using Head = typename Value::Head;
    using Tail = typename Value::Tail;
};

template <class T, std::size_t I>
using Annotations = AnnotList
                      < T
                      , I == 0 ? 0 : typesTupleWithoutAnnots<T>[I-1] + 1
                      , typesTupleWithoutAnnots<T>[I]
                      >;

template <class T, std::size_t I, TypeList Annotations, class Annotation>
struct HasAnnotClass {
    static constexpr bool Value = std::is_same_v<
                    typename Annotations::Head,
                    Annotation
                  > || HasAnnotClass<T, I, typename Annotations::Tail, Annotation>::Value;
};

template <class T, std::size_t I, Empty E, class Annotation>
struct HasAnnotClass<T, I, E, Annotation> {
    static constexpr bool Value = false;
};

template <template <class...> class Template, class T>
constexpr bool isInstanceOfTemplate = false;

template <template <class...> class Template, class... Ts>
constexpr bool isInstanceOfTemplate<Template, Template<Ts...>> = true;

template <class T, std::size_t I, TypeList Annotations, template <class...> class AnnotationTemplate>
struct HasAnnotTemplate {
    static constexpr bool Value = isInstanceOfTemplate<
                    AnnotationTemplate,
                    typename Annotations::Head 
                  > || HasAnnotTemplate<T, I, typename Annotations::Tail, AnnotationTemplate>::Value;
};

template <class T, std::size_t I, Empty E, template <class...> class AnnotationTemplate>
struct HasAnnotTemplate<T, I, E, AnnotationTemplate> {
    static constexpr bool Value = false;
};

template <class T, std::size_t I, TypeList Annotations, template <class...> class AnnotationTemplate>
struct FindAnnotation {
    using FoundAnnotation = std::conditional_t<
            isInstanceOfTemplate<AnnotationTemplate, typename Annotations::Head>,
            typename Annotations::Head,
            typename FindAnnotation<T, I, typename Annotations::Tail, AnnotationTemplate>::FoundAnnotation
        >;
};

}; // namespace detail

template <class T, std::size_t I>
struct FieldDescriptor {
    using Type = detail::NonAnnotFieldType<T, I>;
    using Annotations = type_lists::ToTuple<detail::Annotations<T, I>>;

    template <template <class...> class AnnotationTemplate>
    static constexpr bool has_annotation_template = detail::HasAnnotTemplate<T, I, detail::Annotations<T, I>, AnnotationTemplate>::Value;
    
    template <class Annotation>
    static constexpr bool has_annotation_class = detail::HasAnnotClass<T, I, detail::Annotations<T, I>, Annotation>::Value;

    template <template <class...> class AnnotationTemplate>
    using FindAnnotation = typename detail::FindAnnotation<T, I, detail::Annotations<T, I>, AnnotationTemplate>::FoundAnnotation;
};

template <class T>
struct Describe {
    static constexpr std::size_t num_fields = detail::countNonAnnotFields<T>();
    
    template <std::size_t I>
    using Field = FieldDescriptor<T, I>;
};
