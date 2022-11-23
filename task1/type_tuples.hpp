#pragma once

#include <utility>

namespace type_tuples
{

template<class... Ts>
struct TTuple {};

template<class TT>
concept TypeTuple = requires(TT t) { []<class... Ts>(TTuple<Ts...>){}(t); };

namespace details {
  template<typename Head, typename... Tail>
  TTuple<Head, Tail...> PushFrontImpl(Head, TTuple<Tail...>);
}

template<typename Head, type_tuples::TypeTuple TT>
using PushFront = decltype(details::PushFrontImpl(std::declval<Head>(), std::declval<TT>()));

} // namespace type_tuples
