#include <span>
#include <concepts>
#include <cstdlib>
#include <array>
#include <iterator>
#include <type_traits>

inline constexpr std::ptrdiff_t dynamic_stride = -1;

namespace {

template<typename T, T value, T special_value>
struct ValueWrapper {
  constexpr T GetValue() const { return value; }
};

template<typename T, T special_value>
struct ValueWrapper<T, special_value, special_value> {
  ValueWrapper(T value) : value_(value) {}

  template<T other_value>
  ValueWrapper(const ValueWrapper<T, other_value, special_value>& other) : ValueWrapper(other.GetValue()) {}

  constexpr T GetValue() const { return value_; }
private:
  T value_;
};

template<typename T, T current, T special_value>
inline constexpr T MakeValue(T value) {
  return (current == special_value ? special_value : value);
}

// template<typename T, T current, T special_value, T static_value>
// inline constexpr ValueWrapper<T, MakeValue<T, current, special_value>(static_value), special_value> MakeValueWrapper(T dynamic_value) {
//   return (current == special_value 
//           ? ValueWrapper<T, special_value, special_value>(dynamic_value)
//           : ValueWrapper<T, static_value, special_value>());
// }

template<typename T, T current, T special_value, T static_value>
struct MakeValueWrapper{
  ValueWrapper<T, static_value, special_value> operator()(T /* dynamic_value */) const {
    return {};
  }
};

template<typename T, T special_value, T static_value>
struct MakeValueWrapper<T, special_value, special_value, static_value> {
  ValueWrapper<T, special_value, special_value> operator()(T dynamic_value) const {
    return ValueWrapper<T, special_value, special_value>(dynamic_value);
  }
};


template<std::size_t extent>
using ExtentWrapper = ValueWrapper<std::size_t, extent, std::dynamic_extent>;
template<std::ptrdiff_t stride>
using StrideWrapper = ValueWrapper<std::ptrdiff_t, stride, dynamic_stride>;

template<std::size_t extent>
inline constexpr std::size_t MakeExtent(std::size_t value) {
  return MakeValue<std::size_t, extent, std::dynamic_extent>(value);
}
template<std::ptrdiff_t stride>
inline constexpr std::ptrdiff_t MakeStride(std::ptrdiff_t value) {
  return MakeValue<std::ptrdiff_t, stride, dynamic_stride>(value);
}

// template<std::size_t extent, std::size_t static_value>
// using MakeExtentWrapper = MakeValueWrapper<std::size_t, extent, std::dynamic_extent, static_value>;
// template<std::ptrdiff_t stride, std::ptrdiff_t static_value>
// using MakeStrideWrapper = MakeValueWrapper<std::ptrdiff_t, stride, dynamic_stride, static_value>;

} // namespace


template
  < typename T
  , std::size_t extent = std::dynamic_extent
  , std::ptrdiff_t stride = 1
  >
class Slice : private ExtentWrapper<extent>, private StrideWrapper<stride> {
public:
  using value_type = std::remove_const_t<T>;
  using element_type = T;
  using size_type = std::size_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using difference_type = std::ptrdiff_t;

  class iterator {
  public:
    using element_type = T;
    using difference_type = int;

    iterator() = default;

    iterator(T* ptr, std::ptrdiff_t step) : ptr_(ptr), stride_(step) {}
    iterator(const iterator&) = default;

    friend bool operator==(const iterator& first, const iterator& second) {
      return first.ptr_ == second.ptr_;
    }

    friend std::weak_ordering operator<=>(const iterator& first, const iterator& second) {
      return first.ptr_ <=> second.ptr_;
    }

    T& operator*() const { return *ptr_; }

    iterator& operator+=(int count) { ptr_ += stride_ * count; return *this; }
    iterator& operator-=(int count) { return static_cast<iterator&>(*this) += -count; }
    friend iterator operator+(const iterator& iter, int count) { auto copy = iter; return iter += count; }
    friend iterator operator+(int count, const iterator& iter) { return iter + count; }
    friend iterator operator-(const iterator& iter, int count) { return iter + -count; }
    friend iterator operator-(int count, const iterator& iter) { return iter - count; }
    friend difference_type operator-(const iterator& first, const iterator& second) { return (first.ptr_ - second.ptr_) / first.stride_; }
    iterator& operator++() { return static_cast<iterator&>(*this) += 1; }
    iterator& operator--() { return (*this) -= 1; }
    iterator operator++(int) {
      auto copy = *this;
      operator++();
      return copy; 
    }
    iterator operator--(int) {
      auto copy = *this;
      operator--();
      return copy;
    }
    
    element_type& operator[] (int count) const {
      return *((*this + count).ptr_);
    }
  protected:
    T* ptr_;
    std::ptrdiff_t stride_;
  };

  using reverse_iterator = std::reverse_iterator<iterator>;

  Slice() requires (extent == std::dynamic_extent || extent == 0) {}

  template<typename U>
  requires requires(U container) {
    container.data();
    container.size();
  }
  Slice(U& container);

  template<std::size_t N>
  Slice(std::array<T, N>& arr);

  template<typename U, std::size_t other_extent, std::ptrdiff_t other_stride>
  requires(
    (std::same_as<T, U> || std::same_as<T, const U>)
    && (extent == other_extent || extent == std::dynamic_extent)
    && (stride == other_stride || stride == dynamic_stride))
  Slice(const Slice<U, other_extent, other_stride>&);

  Slice(const Slice<T, extent, stride>&) = default;

  template<std::contiguous_iterator It>
  explicit Slice(It first, std::size_t count, std::ptrdiff_t skip);

  Slice<T, extent, stride>& operator=(const Slice<T, extent, stride>& other) = default;
  
  // Data, Size, Stride, begin, end, casts, etc...

  pointer Data() const;

  constexpr std::size_t Size() const;

  constexpr std::ptrdiff_t Stride() const;

  Slice<T, std::dynamic_extent, stride>
    First(std::size_t count) const requires(extent == std::dynamic_extent);

  template<std::size_t count>
  Slice<T, count, stride>
    First() const;

  Slice<T, std::dynamic_extent, stride>
    Last(std::size_t count) const requires(extent == std::dynamic_extent);

  template<std::size_t count>
  Slice<T, count, stride>
    Last() const;

  Slice<T, std::dynamic_extent, stride>
    DropFirst(std::size_t count) const requires(extent == std::dynamic_extent);

  template<std::size_t count>
  Slice<T, MakeExtent<extent>(extent - count), stride>
    DropFirst() const;

  Slice<T, std::dynamic_extent, stride>
    DropLast(std::size_t count) const requires(extent == std::dynamic_extent);

  template<std::size_t count>
  Slice<T, MakeExtent<extent>(extent - count), stride>
    DropLast() const;

  Slice<T, std::dynamic_extent, dynamic_stride>
    Skip(std::ptrdiff_t skip) const;

  template<std::ptrdiff_t skip>
  Slice<T, MakeExtent<extent>((extent + skip - 1) / skip), MakeStride<stride>(stride * skip)>
    Skip() const;

  Slice<T, extent, stride>::iterator begin() const;
  
  Slice<T, extent, stride>::iterator end() const;

  reverse_iterator rbegin() const;
  
  reverse_iterator rend() const;

  T& operator[] (int) const;

  ~Slice() noexcept = default;

protected:
  pointer data_ = nullptr;

  constexpr ExtentWrapper<extent> GetExtent() const;

  constexpr StrideWrapper<stride> GetStride() const;

  constexpr pointer GetPtr(int i) const;

  template<typename U, std::size_t other_extent, std::ptrdiff_t other_stride>
  friend class Slice;

  Slice(pointer, ExtentWrapper<extent>, StrideWrapper<stride>);

  template<std::size_t static_value>
  using MakeExtentWrapper = MakeValueWrapper<std::size_t, extent, std::dynamic_extent, static_value>;
  template<std::ptrdiff_t static_value>
  using MakeStrideWrapper = MakeValueWrapper<std::ptrdiff_t, stride, dynamic_stride, static_value>;

  // std::size_t extent_; ?
  // std::ptrdiff_t stride_; ?
};

template<typename U>
Slice(U& container) -> Slice<typename U::value_type, std::dynamic_extent, 1>;

template<typename T, std::size_t N>
Slice(std::array<T, N>& arr) -> Slice<T, N, 1>;

template<std::contiguous_iterator It>
explicit Slice(It first, std::size_t count, std::ptrdiff_t skip)
  -> Slice<typename It::value_type, std::dynamic_extent, dynamic_stride>;

template<typename T, std::size_t extent, std::ptrdiff_t stride>
template<typename U>
requires requires(U container) {
  container.data();
  container.size();
}
Slice<T, extent, stride>::Slice(U& container)
  : Slice(container.data(), ExtentWrapper<std::dynamic_extent>(container.size()), StrideWrapper<1>()) {}

template<typename T, std::size_t extent, std::ptrdiff_t stride>
template<std::size_t N>
Slice<T, extent, stride>::Slice(std::array<T, N>& arr)
  : Slice(arr.data(), ExtentWrapper<extent>(), StrideWrapper<1>()) {}


template<typename T, std::size_t extent, std::ptrdiff_t stride>
template<typename U, std::size_t other_extent, std::ptrdiff_t other_stride>
requires(
  (std::same_as<T, U> || std::same_as<T, const U>)
  && (extent == other_extent || extent == std::dynamic_extent)
  && (stride == other_stride || stride == dynamic_stride))
Slice<T, extent, stride>::Slice(const Slice<U, other_extent, other_stride>& other)
  : ExtentWrapper<extent>(other.GetExtent())
  , StrideWrapper<stride>(other.GetStride())
  , data_(other.Data()) {
}

template<typename T, std::size_t extent, std::ptrdiff_t stride>
template<std::contiguous_iterator It>
Slice<T, extent, stride>::Slice(It first, std::size_t count, std::ptrdiff_t skip)
  : ExtentWrapper<extent>(count)
  , StrideWrapper<stride>(skip)
  , data_(&*first) {
}

template<typename T, std::size_t extent, std::ptrdiff_t stride,
          std::equality_comparable_with<T> U, std::size_t other_extent, std::ptrdiff_t other_stride>
bool operator==(const Slice<T, extent, stride>& first, const Slice<U, other_extent, other_stride>& second) {
  if (first.Size() != second.Size()) {
    return false;
  }
  for (std::size_t i = 0; i < first.Size(); ++i) {
    if (first[i] != second[i]) {
      return false;
    }
  }
  return true;
}

template<typename T, std::size_t extent, std::ptrdiff_t stride>
typename Slice<T, extent, stride>::pointer Slice<T, extent, stride>::Data() const {
  return data_;
}

template<typename T, std::size_t extent, std::ptrdiff_t stride>
constexpr std::size_t Slice<T, extent, stride>::Size() const {
  return GetExtent().GetValue();
}

template<typename T, std::size_t extent, std::ptrdiff_t stride>
constexpr std::ptrdiff_t Slice<T, extent, stride>::Stride() const {
  return GetStride().GetValue();
}

template<typename T, std::size_t extent, std::ptrdiff_t stride>
Slice<T, std::dynamic_extent, stride>
  Slice<T, extent, stride>::First(std::size_t count) const requires(extent == std::dynamic_extent) {
    return {Data(), ExtentWrapper<std::dynamic_extent>(count), GetStride()};
  }

template<typename T, std::size_t extent, std::ptrdiff_t stride>
template<std::size_t count>
Slice<T, count, stride>
  Slice<T, extent, stride>::First() const {
    return {Data(), ExtentWrapper<count>(), GetStride()};
  }

template<typename T, std::size_t extent, std::ptrdiff_t stride>
Slice<T, std::dynamic_extent, stride>
  Slice<T, extent, stride>::Last(std::size_t count) const requires(extent == std::dynamic_extent) {
    return {GetPtr(Size() - count), ExtentWrapper<std::dynamic_extent>(count), GetStride()};
  }

template<typename T, std::size_t extent, std::ptrdiff_t stride>
template<std::size_t count>
Slice<T, count, stride>
  Slice<T, extent, stride>::Last() const {
    return {GetPtr(Size() - count), ExtentWrapper<count>(), GetStride()};
  }

template<typename T, std::size_t extent, std::ptrdiff_t stride>
Slice<T, std::dynamic_extent, stride>
  Slice<T, extent, stride>::DropFirst(std::size_t count) const requires(extent == std::dynamic_extent) {
    return Last(Size() - count);
  }

template<typename T, std::size_t extent, std::ptrdiff_t stride>
template<std::size_t count>
Slice<T, MakeExtent<extent>(extent - count), stride>
  Slice<T, extent, stride>::DropFirst() const {
    return {GetPtr(count), MakeExtentWrapper<extent - count>()(Size() - count), GetStride()};
  }

template<typename T, std::size_t extent, std::ptrdiff_t stride>
Slice<T, std::dynamic_extent, stride>
  Slice<T, extent, stride>::DropLast(std::size_t count) const requires(extent == std::dynamic_extent) {
    return First(Size() - count);
  }

template<typename T, std::size_t extent, std::ptrdiff_t stride>
template<std::size_t count>
Slice<T, MakeExtent<extent>(extent - count), stride>
  Slice<T, extent, stride>::DropLast() const {
    return {Data(), MakeExtentWrapper<extent - count>()(Size() - count), GetStride()};
  }


template<typename T, std::size_t extent, std::ptrdiff_t stride>
Slice<T, std::dynamic_extent, dynamic_stride>
  Slice<T, extent, stride>::Skip(std::ptrdiff_t skip) const {
    return {Data(), ExtentWrapper<std::dynamic_extent>((Size() + skip - 1) / skip), StrideWrapper<dynamic_stride>(skip * Stride())};
  }

template<typename T, std::size_t extent, std::ptrdiff_t stride>
template<std::ptrdiff_t skip>
Slice<T, MakeExtent<extent>((extent + skip - 1) / skip), MakeStride<stride>(stride * skip)>
  Slice<T, extent, stride>::Skip() const {
    return {Data(), MakeExtentWrapper<(extent + skip - 1) / skip>()((Size() + skip - 1) / skip), MakeStrideWrapper<stride * skip>()(Stride() * skip)};
  }


template<typename T, std::size_t extent, std::ptrdiff_t stride>
typename Slice<T, extent, stride>::iterator Slice<T, extent, stride>::begin() const {
  return iterator(Data(), Stride());
}

template<typename T, std::size_t extent, std::ptrdiff_t stride>
typename Slice<T, extent, stride>::iterator Slice<T, extent, stride>::end() const {
  return iterator(GetPtr(Size()), Stride());
}

template<typename T, std::size_t extent, std::ptrdiff_t stride>
typename Slice<T, extent, stride>::reverse_iterator Slice<T, extent, stride>::rbegin() const {
  return iterator(GetPtr(Size() - 1), Stride());
}

template<typename T, std::size_t extent, std::ptrdiff_t stride>
typename Slice<T, extent, stride>::reverse_iterator Slice<T, extent, stride>::rend() const {
  return iterator(GetPtr(-1), Stride());
}

template<typename T, std::size_t extent, std::ptrdiff_t stride>
T& Slice<T, extent, stride>::operator[] (int i) const {
  return *GetPtr(i);
}


template<typename T, std::size_t extent, std::ptrdiff_t stride>
Slice<T, extent, stride>::Slice(
  Slice<T, extent, stride>::pointer data,
  ExtentWrapper<extent> extent_wrapper,
  StrideWrapper<stride> stride_wrapper)
    : ExtentWrapper<extent>(extent_wrapper)
    , StrideWrapper<stride>(stride_wrapper)
    , data_(data) {
  }

template<typename T, std::size_t extent, std::ptrdiff_t stride>
constexpr ExtentWrapper<extent> Slice<T, extent, stride>::GetExtent() const {
  return static_cast<const ExtentWrapper<extent>&>(*this);
}

template<typename T, std::size_t extent, std::ptrdiff_t stride>
constexpr StrideWrapper<stride> Slice<T, extent, stride>::GetStride() const {
  return static_cast<const StrideWrapper<stride>&>(*this);
}

template<typename T, std::size_t extent, std::ptrdiff_t stride>
constexpr typename Slice<T, extent, stride>::pointer Slice<T, extent, stride>::GetPtr(int i) const {
  return Data() + Stride() * i;
}
