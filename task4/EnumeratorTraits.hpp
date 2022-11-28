#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>


namespace {

template<auto T>
constexpr auto Helper() {
  return __PRETTY_FUNCTION__;
}

template <class Enum, auto I>
	requires std::is_enum_v<Enum>
constexpr Enum GetEnumValue() {
  return static_cast<Enum>(I);
}


template <class Enum, long I>
	requires std::is_enum_v<Enum>
constexpr bool IsInEnum() {
  const std::string_view str = Helper<GetEnumValue<Enum, I>()>();
  return str[str.find_last_of('=') + 2] != '(';
}

template <class Enum, long I>
	requires std::is_enum_v<Enum>
consteval std::string_view GetName() {
  std::string_view str = Helper<GetEnumValue<Enum, I>()>();
  std::size_t pos = str.find_last_of('=') + 2;
  str = str.substr(pos, str.size() - pos - 1);
  pos = str.find_last_of(':') + 1;
  if (pos != str.npos) {
    str = str.substr(pos, str.size() - pos);
  }
  return str;
}

template <class Enum, std::size_t MAXN>
	requires std::is_enum_v<Enum>
consteval std::size_t CalculateEnumSize() {
  constexpr int max_n = std::min(static_cast<unsigned long long>(std::numeric_limits<std::underlying_type_t<Enum>>::max()), static_cast<unsigned long long>(MAXN));
  constexpr int min_n = std::max(static_cast<long long>(std::numeric_limits<std::underlying_type_t<Enum>>::min()), -static_cast<long long>(MAXN));
  std::size_t size = 0;
  if constexpr(std::is_signed<std::underlying_type_t<Enum>>::value && min_n < 0) {
    [&]<int... I>(std::integer_sequence<int, I...>) {
      ([&](){
        if (IsInEnum<Enum, I + min_n>()) {
          ++size;
        }
      }(), ...);
    }(std::make_integer_sequence<int, -min_n>());
  }
  [&]<int... I>(std::integer_sequence<int, I...>) {
    ([&](){
      if (IsInEnum<Enum, I>()) {
        ++size;
      }
    }(), ...);
  }(std::make_integer_sequence<int, max_n + 1>());
  return size;
}

template <class Enum, std::size_t MAXN>
	requires std::is_enum_v<Enum>
consteval auto CalculateEnumValues() {
  constexpr int max_n = std::min(static_cast<unsigned long long>(std::numeric_limits<std::underlying_type_t<Enum>>::max()), static_cast<unsigned long long>(MAXN));
  constexpr int min_n = std::max(static_cast<long long>(std::numeric_limits<std::underlying_type_t<Enum>>::min()), -static_cast<long long>(MAXN));
  std::array<Enum, CalculateEnumSize<Enum, MAXN>()> values;
  std::size_t i = 0;
  if constexpr(std::is_signed<std::underlying_type_t<Enum>>::value && min_n < 0) {
    [&]<int... I>(std::integer_sequence<int, I...>) {
      ([&](){
        if (IsInEnum<Enum, I + min_n>()) {
          values[i] = GetEnumValue<Enum, I + min_n>();
          ++i;
        }
      }(), ...);
    }(std::make_integer_sequence<int, -min_n>());
  }
  [&]<int... I>(std::integer_sequence<int, I...>) {
    ([&](){
      if (IsInEnum<Enum, I>()) {
        values[i] = GetEnumValue<Enum, I>();
        ++i;
      }
    }(), ...);
  }(std::make_integer_sequence<int, max_n + 1>());
  return values;
}

template <class Enum, std::size_t MAXN>
	requires std::is_enum_v<Enum>
consteval auto CalculateEnumNames() {
  constexpr int max_n = std::min(static_cast<unsigned long long>(std::numeric_limits<std::underlying_type_t<Enum>>::max()), static_cast<unsigned long long>(MAXN));
  constexpr int min_n = std::max(static_cast<long long>(std::numeric_limits<std::underlying_type_t<Enum>>::min()), -static_cast<long long>(MAXN));
  std::array<std::string_view, CalculateEnumSize<Enum, MAXN>()> names;
  std::size_t i = 0;
  if constexpr(std::is_signed<std::underlying_type_t<Enum>>::value && min_n < 0) {
    [&]<int... I>(std::integer_sequence<int, I...>) {
      ([&](){
        if (IsInEnum<Enum, I + min_n>()) {
          names[i] = GetName<Enum, I + min_n>();
          ++i;
        }
      }(), ...);
    }(std::make_integer_sequence<int, -min_n>());
  }
  [&]<int... I>(std::integer_sequence<int, I...>) {
    ([&](){
      if (IsInEnum<Enum, I>()) {
        names[i] = GetName<Enum, I>();
        ++i;
      }
    }(), ...);
  }(std::make_integer_sequence<int, max_n + 1>());
  return names;
}

} // namespace

template <class Enum, std::size_t MAXN = 512>
	requires std::is_enum_v<Enum>
struct EnumeratorTraits {
  static constexpr std::size_t size() noexcept;
  static constexpr Enum at(std::size_t i) noexcept;
  static constexpr std::string_view nameAt(std::size_t i) noexcept;
 private:
  static constexpr auto enum_values_ = CalculateEnumValues<Enum, MAXN>();
  static constexpr auto enum_names_ = CalculateEnumNames<Enum, MAXN>();
};

template <class Enum, std::size_t MAXN>
	requires std::is_enum_v<Enum>
constexpr std::size_t EnumeratorTraits<Enum, MAXN>::size() noexcept {
  return enum_values_.size();
}

template <class Enum, std::size_t MAXN>
	requires std::is_enum_v<Enum>
constexpr Enum EnumeratorTraits<Enum, MAXN>::at(std::size_t i) noexcept {
  return enum_values_[i];
}

template <class Enum, std::size_t MAXN>
	requires std::is_enum_v<Enum>
constexpr std::string_view EnumeratorTraits<Enum, MAXN>::nameAt(std::size_t i) noexcept {
  return enum_names_[i];
}
