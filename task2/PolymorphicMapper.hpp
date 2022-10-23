#pragma once

#include <optional>
#include <type_traits>

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

template <class Base, class Target, class Mapping, class... Mappings>
	requires (CheckVaArgs<Base, Target, Mapping, Mappings...>::Value)
struct PolymorphicMapper<Base, Target, Mapping, Mappings...> {
	static std::optional<Target> map(const Base& object) {
		try {
			dynamic_cast<const typename Mapping::Class&>(object);
			
			return Mapping::value();
		}
		catch (const std::bad_cast& e) {
			return PolymorphicMapper<Base, Target, Mappings...>::map(object);
		}
	}

};

template <class Base, class Target>
struct PolymorphicMapper<Base, Target> {
	static std::optional<Target> map(const Base&) {
		return std::nullopt;
	}
};
