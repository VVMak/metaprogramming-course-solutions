#pragma once

#include <string>

constexpr std::size_t LENGTH = 256;

template<std::size_t max_length>
struct FixedString {
  constexpr FixedString(const char* string, std::size_t length) : length(length) {
    for (std::size_t i = 0; i < length; ++i) {
      impl[i] = string[i];
    }
    for (std::size_t i = length; i < max_length; ++i) {
      impl[i] = '\0';
    }
  }
  constexpr operator std::string_view() const { return std::string_view(impl, length); }

  char impl[max_length];
  std::size_t length;
};

template<typename CharT, CharT... Cs>
constexpr FixedString<LENGTH> operator ""_cstr() {
  constexpr char str[] = {Cs...};
  return FixedString<LENGTH>(str, sizeof...(Cs));
} 
