#pragma once

#include <optional>


template<typename>
constexpr bool depFalse = false;

template <class From, auto target>
struct Mapping;

template <class Base, class Target, class... Mappings>
struct PolymorphicMapper {
  static std::optional<Target> map(const Base&) {
    return std::nullopt;
  }
};

template <class Base, class Target, class From, class MappingTarget, MappingTarget target, class... Mappings>
struct PolymorphicMapper<Base, Target, Mapping<From, target>, Mappings...> {
  static std::optional<Target> map(const Base& object) requires(std::same_as<Target, MappingTarget>) {
    if (dynamic_cast<const From*>(&object)) {
      return {target};
    }
    return PolymorphicMapper<Base, Target, Mappings...>::map(object);
  }
};