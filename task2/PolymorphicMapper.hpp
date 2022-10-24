#pragma once

#include <optional>
#include <type_traits>
#include "top_sort_type_list.hpp"

template<class From, auto target>
struct Mapping {
	using Class = From;
	static constexpr auto value() {
		return target;
	}
};

template <class Base, class Target, class... Mappings>
struct PolymorphicMapper;

template<class Base, class Target, class... Mappings>
struct CheckVaArgs;

template<class Base, class Target, class Mapping, class... Mappings>
struct CheckVaArgs<Base, Target, Mapping, Mappings...> {
	constexpr static bool Value = std::is_base_of_v<Base, typename Mapping::Class> &&
			std::is_same_v<Target, decltype(Mapping::value())> &&
			CheckVaArgs<Base, Target, Mappings...>::Value;
};

template<class Base, class Target>
struct CheckVaArgs<Base, Target> {
	constexpr static bool Value = true;
};

using type_lists::TypeList;
using type_lists::Empty;
using type_lists::VaArgsToTypeList;
using type_lists::TopSort;

template<class Base, class Target, TypeList TL>
struct PolymorphicMapperHelper;

template <class Base, class Target, class... Mappings>
	requires (CheckVaArgs<Base, Target, Mappings...>::Value)
struct PolymorphicMapper<Base, Target, Mappings...> {
	static std::optional<Target> map(const Base& object) {
		return PolymorphicMapperHelper<Base, Target, TopSort<VaArgsToTypeList<Mappings...>>>::map(object);
	}
};

template <class Base, class Target, TypeList TL>
struct PolymorphicMapperHelper {
	static std::optional<Target> map(const Base& object) {
		try {
			dynamic_cast<const typename TL::Head::Class&>(object);
			return TL::Head::value();
		}
		catch (const std::bad_cast& e) {
			return PolymorphicMapperHelper<Base, Target, typename TL::Tail>::map(object);
		}
	}
};

template<class Base, class Target, Empty E>
struct PolymorphicMapperHelper<Base, Target, E> {
	static std::optional<Target> map(const Base& object) {
		return std::nullopt;
	}
};

