#pragma once

#include <optional>

template<class From, auto target>
struct Mapping {
	using Class = From;
	static constexpr auto value() {
		return target;
	}
};

template <class Base, class Target, class... Mappings>
struct PolymorphicMapper;

template <class Base, class Target, class Mapping, class... Mappings>
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
