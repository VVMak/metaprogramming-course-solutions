#pragma once

#include <optional>


template <class From, auto target>
struct Mapping;

template <class Base, class Target, class... Mappings>
concept Mappable = requires (Mappings... m) {
  []<class... MappingTail, class From, class MappingTarget, MappingTarget target>(const Mapping<From, target>&, const MappingTail&...)
      requires std::derived_from<From, Base> && std::same_as<Target, MappingTarget> {
  } (m...);
};

template <class Base, class Target, class... Mappings>
  requires ((sizeof...(Mappings) == 0) || Mappable<Base, Target, Mappings...>)
struct PolymorphicMapper {
  static std::optional<Target> map(const Base&) {
    return {};
  }
};

template <class Base, class Target, class From, class MappingTarget, MappingTarget target, class... Mappings>
struct PolymorphicMapper<Base, Target, Mapping<From, target>, Mappings...> {
  static std::optional<Target> map(const Base& object) {
    if (dynamic_cast<const From*>(&object)) {
      return {target};
    }
    return PolymorphicMapper<Base, Target, Mappings...>::map(object);
  }
};
