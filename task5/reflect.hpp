#pragma once

#include <cstdint>
#include <utility>
#include <array>
#include <concepts>
#include <cstddef>
#include <tuple>
#include "type_lists.hpp"

using namespace type_lists;

template <class... Ts>
using Annotate = TTuple<Ts...>;

namespace detail {

template <class T>
constexpr bool isAnnotation = false;

template <class... Ts>
constexpr bool isAnnotation<Annotate<Ts...>> = true;

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

template <class T, std::size_t left, std::size_t right, std::size_t... I>
constexpr auto binarySearchImpl(std::index_sequence<I...>);

template <class T, std::size_t left, std::size_t right, bool binarySearchStarted, std::size_t... I>
constexpr size_t countFieldsImpl(std::index_sequence<I...>) {
    return binarySearchImpl<T, left, right>(std::make_index_sequence<(left+right)/2>{});
}

template <class T, std::size_t left, std::size_t right, bool binarySearchStarted, std::size_t... I>
    requires AggregateConstructibleFrom<T, UbiqConstructor<I>...>
constexpr size_t countFieldsImpl(std::index_sequence<I...>) {
    if constexpr (binarySearchStarted)
        return binarySearchImpl<T, left, right>(std::make_index_sequence<(left+right)/2>{});
    if constexpr (!AggregateConstructibleFrom<T, UbiqConstructor<0>>)
        return 0;
    return countFieldsImpl<T, right, 2 * right, false>(std::make_index_sequence<2 * right>{});
}

template <class T, std::size_t left, std::size_t right, std::size_t... I>
constexpr auto binarySearchImpl(std::index_sequence<I...>) {
    constexpr std::size_t mid = (left + right) / 2;
    if constexpr (mid == left)
        return mid;
    if constexpr (AggregateConstructibleFrom<T, UbiqConstructor<I>...>)
        return countFieldsImpl<T, mid, right, true>(std::make_index_sequence<right>{});
    else
        return countFieldsImpl<T, left,  mid, true>(std::make_index_sequence< mid >{});
}

template <class T>
constexpr auto fieldsCount = countFieldsImpl<T, 0, 1, false>(std::index_sequence<>{});

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
    return TTuple<
        typename LoopholeGet<T, Is>::Type...
    >{};
}

template <class T>
using TypesTuple = decltype(asTupleImpl<T>(std::make_index_sequence<fieldsCount<T>>{}));

// array-map of non-annotation field indices
template <class T, std::size_t max, std::size_t offset, std::size_t iterCount>
constexpr auto fillNonAnnotIndices(auto& nonAnnotsArr, std::size_t start) {
    std::size_t nonAnnotIdx = start;
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (void)(([&]() {
            if constexpr (I+offset < fieldsCount<T>) {
                if constexpr (!isAnnotation<typename GetNth<I+offset, TypesTuple<T>>::Type>)
                    nonAnnotsArr[nonAnnotIdx++] = I+offset;
            }
        }(), I+offset <= fieldsCount<T>) && ...);
    }(std::make_index_sequence<max>{});
    if constexpr (iterCount > 0)
        fillNonAnnotIndices<T, max, offset + max, iterCount - 1>(nonAnnotsArr, nonAnnotIdx);
}

template <class T>
constexpr auto nonAnnotIndices() {
    constexpr std::size_t clangFoldExprDepth = 256;
    constexpr std::size_t iterCount = (fieldsCount<T> + clangFoldExprDepth - 1) / clangFoldExprDepth;
    std::array<std::size_t, fieldsCount<T>> nonAnnots;
    nonAnnots.fill(-1);
    fillNonAnnotIndices<T, clangFoldExprDepth, 0, iterCount>(nonAnnots, 0);
    return nonAnnots;
}

template <class T>
constexpr auto nonAnnotIndexMap = nonAnnotIndices<T>();

// number of non-annotation fields
template <class T>
constexpr std::size_t countNonAnnotFields() {
    if constexpr (fieldsCount<T> == 0)
        return 0;
    std::size_t left = 0;
    std::size_t right = fieldsCount<T> - 1;
    while (true) {
        std::size_t middle = (left + right) / 2;
        if (left == middle)
            return middle + 1;
        if (nonAnnotIndexMap<T>[middle] == -1)
            right = middle;
        else
            left = middle;
    }
}

template <class T, std::size_t I>
using FieldType = typename GetNth<I, TypesTuple<T>>::Type;

template <class T, std::size_t I>
using NonAnnotFieldType = typename GetNth<nonAnnotIndexMap<T>[I], TypesTuple<T>>::Type;

// TypeList of annotations of Ith field
template <class T, std::size_t I>
using AnnotListAt = ToTypeList<FieldType<T, I>>;

// merge annotation TypeLists of the current field
template <class T, std::size_t Begin, std::size_t End>
struct AnnotList : Nil {};

template <class T, std::size_t Begin, std::size_t End>
    requires (Begin < End)
struct AnnotList<T, Begin, End> : Append<AnnotListAt<T, Begin>, AnnotList<T, Begin + 1, End>> {};

template <class T, std::size_t I>
using Annotations = AnnotList< T
                             , I == 0 ? 0 : nonAnnotIndexMap<T>[I-1] + 1
                             , nonAnnotIndexMap<T>[I] >;

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

template <class T, std::size_t I, Empty E, template <class...> class AnnotationTemplate>
struct FindAnnotation<T, I, E, AnnotationTemplate> {
    using FoundAnnotation = Nil;
};

template <class T, std::size_t I, template <class...> class AnnotationTemplate>
using FoundAnnotation = typename detail::FindAnnotation<T, I, detail::Annotations<T, I>, AnnotationTemplate>::FoundAnnotation;

}; // namespace detail

template <class T, std::size_t I>
struct FieldDescriptor {
    using Type = detail::NonAnnotFieldType<T, I>;
    using Annotations = ToTuple<detail::Annotations<T, I>>;

    template <template <class...> class AnnotationTemplate>
    static constexpr bool has_annotation_template = detail::HasAnnotTemplate<T, I, detail::Annotations<T, I>, AnnotationTemplate>::Value;
    
    template <class Annotation>
    static constexpr bool has_annotation_class = detail::HasAnnotClass<T, I, detail::Annotations<T, I>, Annotation>::Value;

    template <template <class...> class AnnotationTemplate>
        requires (!Empty<detail::FoundAnnotation<T, I, AnnotationTemplate>>)
    using FindAnnotation = detail::FoundAnnotation<T, I, AnnotationTemplate>;
};

template <class T>
struct Describe {
    static constexpr std::size_t num_fields = detail::countNonAnnotFields<T>();
    
    template <std::size_t I>
    using Field = FieldDescriptor<T, I>;
};
