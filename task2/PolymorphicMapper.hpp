#pragma once

#include <optional>
#include <type_traits>
#include "top_sort_type_list.hpp"

using type_lists::TypeList;
using type_lists::Empty;
using type_lists::VaArgsToTypeList;
using type_lists::TopSort;

template<class From, auto target>
struct Mapping {
	using Class = From;
	static constexpr auto value() {
		return target;
	}
};

template <class Base, class Target, class Mapping>
concept CorrectMapping = std::is_base_of_v<Base, typename Mapping::Class>
                      && std::is_same_v<Target, decltype(Mapping::value())>;

namespace detail {

template <class Base, class Target, TypeList TL>
struct PolymorphicMapper {
	static std::optional<Target> map(const Base* object) {
		return dynamic_cast<const typename TL::Head::Class*>(object) ?
           TL::Head::value() :
           PolymorphicMapper<Base, Target, typename TL::Tail>::map(object);
	}
};

template<class Base, class Target, Empty E>
struct PolymorphicMapper<Base, Target, E> {
	static std::optional<Target> map(const Base*) {
		return std::nullopt;
	}
};

} //namespace detail

template <class Base, class Target, class... Mappings>
  requires (CorrectMapping<Base, Target, Mappings> && ...)
struct PolymorphicMapper {
  static std::optional<Target> map(const Base& object) {
    return detail::PolymorphicMapper<Base, Target, TopSort<VaArgsToTypeList<Mappings...>>>::map(&object);
  }
};

