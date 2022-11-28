#pragma once

#include <concepts>

#include <type_tuples.hpp>


template<typename T>
constexpr bool depFalse = false;

namespace type_lists
{

template<class TL>
concept TypeSequence =
    requires {
        typename TL::Head;
        typename TL::Tail;
    };

struct Nil {};

template<class TL>
concept Empty = std::derived_from<TL, Nil>;

template<class TL>
concept TypeList = Empty<TL> || TypeSequence<TL>;

// Your fun, fun metaalgorithms :)

template <typename T, TypeList TL>
struct Cons {
  using Head = T;
  using Tail = TL;
};

template<type_tuples::TypeTuple TT>
struct FromTuple {
  template<typename Head, typename... Tail>
  static Head GetHead(type_tuples::TTuple<Head, Tail...>);
  template<typename Head, typename... Tail>
  static type_tuples::TTuple<Tail...> GetTail(type_tuples::TTuple<Head, Tail...>);
  using Head = decltype(GetHead(std::declval<TT>()));
  using Tail = FromTuple<decltype(GetTail(std::declval<TT>()))>;
};

template<>
struct FromTuple<type_tuples::TTuple<>> : Nil {};


namespace details {
  template<TypeList TL>
  struct ToTuple {
    using Type = type_tuples::TTuple<>;
  };

  template<TypeSequence TL>
  struct ToTuple<TL> {
    using Type = type_tuples::PushFront<typename TL::Head, typename ToTuple<typename TL::Tail>::Type>;
  };
}

template<TypeList TL>
using ToTuple = typename details::ToTuple<TL>::Type;


template<typename T>
struct Repeat{
  using Head = T;
  using Tail = Repeat<Head>;
};

template<std::size_t N, TypeList TL>
struct Take : Cons<typename TL::Head, Take<N - 1, typename TL::Tail>> {};

template<TypeList TL>
struct Take<0, TL> : Nil {};

template<std::size_t N, Empty TL>
struct Take<N, TL> : Nil {};


template<std::size_t N, TypeList TL>
struct Drop : Drop<N - 1, typename TL::Tail> {};

template<TypeList TL>
struct Drop<0, TL> : TL {};

template<std::size_t N, Empty TL>
struct Drop<N, TL> : Nil {};


template<std::size_t N, typename T>
struct Replicate : Cons<T, Replicate<N - 1, T>> {};

template<typename T>
struct Replicate<0, T> : Nil {};


template<template<typename> typename F, typename T>
struct Iterate : Cons<T, Nil> {
  using Tail = Iterate<F, F<T>>;
};

namespace details {
  template<TypeList TL, TypeList Iter>
  struct CycleImpl : Cons<typename Iter::Head, Nil> {
    using Tail = CycleImpl<TL, typename Iter::Tail>;
  };
  template<TypeSequence TL, Empty Iter>
  struct CycleImpl<TL, Iter> : CycleImpl<TL, TL> {};
  template<Empty TL1, Empty TL2>
  struct CycleImpl<TL1, TL2> : Repeat<Nil> {};
}

template<TypeList TL>
using Cycle = details::CycleImpl<TL, TL>;


template<template<typename> typename F, TypeList TL>
struct Map : Cons<F<typename TL::Head>, Nil> {
  using Tail = Map<F, typename TL::Tail>;
};

template<template<typename> typename F, Empty TL>
struct Map<F, TL> : Nil {};


template<template<typename> typename P, TypeList TL>
struct Filter : Filter<P, typename TL::Tail> {};

template<template<typename> typename P, TypeList TL>
requires(P<typename TL::Head>::Value)
struct Filter<P, TL> {
  using Head = typename TL::Head;
  using Tail = Filter<P, typename TL::Tail>;
};

template<template<typename> typename P, Empty TL>
struct Filter<P, TL> : Nil {};

namespace helpers {
  template<TypeList TL, typename T>
  struct PushBack : Cons<typename TL::Head, PushBack<typename TL::Tail, T>> {};
  template<Empty TL, typename T>
  struct PushBack<TL, T> : Cons<T, Nil> {};
}

namespace details {
  template<TypeList Left, TypeList Right>
  struct InitsImpl : Cons<Left, InitsImpl<helpers::PushBack<Left, typename Right::Head>, typename Right::Tail>> {};
  template<TypeList Left, Empty Right>
  struct InitsImpl<Left, Right> : Cons<Left, Nil> {};
}

template<TypeList TL>
using Inits = details::InitsImpl<Nil, TL>;

template<TypeList TL>
struct Tails : Cons<TL, Tails<typename TL::Tail>> {};
template<Empty TL>
struct Tails<TL> : Cons<Nil, Nil> {};

namespace details {
  template<template<typename, typename> typename Op, typename T, TypeList TL>
  struct ScanlImpl : Cons<Op<T, typename TL::Head>, Nil> {
    using Tail = ScanlImpl<Op, typename TL::Head, typename TL::Tail>;
  };
  template<template<typename, typename> typename Op, typename T, Empty TL>
  struct ScanlImpl<Op, T, TL> : Nil {};
}

template<template<typename, typename> typename Op, typename T, TypeList TL>
using Scanl = Cons<T, details::ScanlImpl<Op, T, TL>>;

namespace details {
  template<template<typename, typename> typename Op, typename T, TypeList TL>
  struct FoldlImpl {
    using Type = typename FoldlImpl<Op, Op<T, typename TL::Head>, typename TL::Tail>::Type;
  };
  template<template<typename, typename> typename Op, typename T, Empty TL>
  struct FoldlImpl<Op, T, TL> {
    using Type = T;
  };
}

template<template<typename, typename> typename Op, typename T, TypeList TL>
using Foldl = typename details::FoldlImpl<Op, T, TL>::Type;

template<TypeList L, TypeList R>
struct Zip2 : Cons<type_tuples::TTuple<typename L::Head, typename R::Head>, Nil> {
  using Tail = Zip2<typename L::Tail, typename R::Tail>;
};

template<Empty L, TypeList R>
struct Zip2<L, R> : Nil {};

template<TypeList L, Empty R>
struct Zip2<L, R> : Nil {};

template<Empty L, Empty R>
struct Zip2<L, R> : Nil {};


namespace details {
  template<TypeSequence TL>
  using PopFront = typename TL::Tail;

  template<TypeList TL>
  struct HasEmptyImpl {
    static constexpr bool value = Empty<typename TL::Head> || HasEmptyImpl<typename TL::Tail>::value;
  };
  template<Empty TL>
  struct HasEmptyImpl<TL> {
    static constexpr bool value = false;
  };
  template<typename TL>
  concept HasEmpty = TypeSequence<TL> && HasEmptyImpl<TL>::value;

  template<TypeList TL>
  struct ZipImpl : Cons<Cons<typename TL::Head::Head, typename ZipImpl<typename TL::Tail>::Head>, Nil> {
    using Tail = ZipImpl<Map<PopFront, TL>>;
  };
  template<Empty TL>
  struct ZipImpl<TL> : Cons<Nil, Nil> {};
  template<HasEmpty TL>
  struct ZipImpl<TL> : Nil {};
}

// template<TypeList... TLs>
// struct Zip : Map<ToTuple, details::ZipImpl<FromTuple<type_tuples::TTuple<TLs...>>>> {};

template<TypeList... TLs>
struct Zip : Nil {};

template<TypeSequence TLHead, TypeSequence... TLTail>
struct Zip<TLHead, TLTail...> {
  using Head = type_tuples::TTuple<typename TLHead::Head, typename TLTail::Head...>;
  using Tail = Zip<typename TLHead::Tail, typename TLTail::Tail...>;
};


namespace details {
  template<template<typename, typename> typename EQ, TypeList TL>
  struct SkipEqual : Nil {};
  template<template<typename, typename> typename EQ, TypeSequence TL>
  requires(!EQ<typename TL::Head, typename TL::Tail::Head>::Value)
  struct SkipEqual<EQ, TL> : TL::Tail {};
  template<template<typename, typename> typename EQ, TypeSequence TL>
  requires(EQ<typename TL::Head, typename TL::Tail::Head>::Value)
  struct SkipEqual<EQ, TL> : SkipEqual<EQ, typename TL::Tail> {};

  template<template<typename, typename> typename EQ, TypeList TL>
  struct GetEqual : TL {};
  template<template<typename, typename> typename EQ, TypeSequence TL>
  requires(!EQ<typename TL::Head, typename TL::Tail::Head>::Value)
  struct GetEqual<EQ, TL> : Cons<typename TL::Head, Nil> {};
  template<template<typename, typename> typename EQ, TypeSequence TL>
  requires(EQ<typename TL::Head, typename TL::Tail::Head>::Value)
  struct GetEqual<EQ, TL> : Cons<typename TL::Head, GetEqual<EQ, typename TL::Tail>> {};
}

template<template<typename, typename> typename EQ, TypeList TL>
struct GroupBy : Nil {};

template<template<typename, typename> typename EQ, TypeSequence TL>
struct GroupBy<EQ, TL> : Cons<details::GetEqual<EQ, TL>, Nil> {
  using Tail = GroupBy<EQ, details::SkipEqual<EQ, TL>>;
};

// // не понимаю, почему не работает
// namespace details {
//   template<TypeSequence TL>
//   using GetHead = typename TL::Head;
//   template<TypeSequence TL>
//   using PopFront = typename TL::Tail;

//   // проверяет, есть ли пустой тайплист в тайплисте тайплистов
//   template<TypeList TL>
//   struct HasEmptyImpl {
//     static constexpr bool value = Empty<typename TL::Head> || HasEmptyImpl<typename TL::Tail>::value;
//   };
//   template<Empty TL>
//   struct HasEmptyImpl<TL> {
//     static constexpr bool value = false;
//   };
//   template<typename TL>
//   concept HasEmpty = TypeSequence<TL> && HasEmptyImpl<TL>::value;

//   template<TypeList TL>
//   struct ZipTL : Cons<ToTuple<Map<GetHead, TL>>, Nil> {
//     using Tail = ZipTL<Map<PopFront, TL>>;
//   };
//   template<HasEmpty TL>
//   struct ZipTL<TL> : Nil {};
// }

// template<TypeList... TLs>
// using Zip = details::ZipTL<FromTuple<type_tuples::TTuple<TLs...>>>;


} // namespace type_lists
