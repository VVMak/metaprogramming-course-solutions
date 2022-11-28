#pragma once

#include <vector>

#include <value_types.hpp>
#include <type_lists.hpp>

namespace {
  using type_lists::Nil;
  using type_lists::TypeList;
  using type_lists::Cons;
  using value_types::ValueTag;
  template<typename Head, TypeList TL>
  struct LazyCons : Cons<Head, Nil> {
    using Tail = TL;
  };

  template<auto N>
  struct NatsFrom : Cons<ValueTag<N>, Nil> {
    using Tail = NatsFrom<N + 1>;
  };

  template<auto N>
  struct FibFrom : Cons<ValueTag<FibFrom<N - 1>::Head::Value + FibFrom<N - 2>::Head::Value>, Nil> {
    using Tail = FibFrom<N + 1>;
  };
  template<>
  struct FibFrom<0> : Cons<ValueTag<0>, Nil> {
    using Tail = FibFrom<1>;
  };
  template<>
  struct FibFrom<1> : Cons<ValueTag<1>, Nil> {
    using Tail = FibFrom<2>;
  };

  static consteval int GetNthPrime(std::size_t n) { // numeration from 0
    std::vector<int> primes{2, 3, 5, 7};
    for (std::size_t k = primes.size(); k <= n; ++k) {
      int x = primes.back();
      for (bool is_prime = false; !is_prime;) {
        x += 2;
        is_prime = true;
        for (std::size_t i = 0; primes[i] * primes[i] <= x; ++i) {
          if (x % primes[i] == 0) {
            is_prime = false;
            break;
          }
        }
      }
      primes.push_back(x);
    }
    return primes[n];
  }
  template<auto N>
  struct PrimesFrom : Cons<ValueTag<GetNthPrime(N)>, Nil> {
    using Tail = PrimesFrom<N + 1>;
  };
}


using Nats = NatsFrom<0>;
using Fib = FibFrom<0>;
using Primes = PrimesFrom<0>;
