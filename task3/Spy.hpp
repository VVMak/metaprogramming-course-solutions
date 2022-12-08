#pragma once

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

class BaseLogger {
 protected:
  virtual void Call(unsigned int) = 0;
  unsigned int counter_ = 0;
  BaseLogger(unsigned int counter = 0) : counter_(counter) {}
 public:
  virtual ~BaseLogger() {};
  void Log() {
    if (counter_ != 0) {
      Call(counter_);
      Reset();
    }
  }
  void Add() {
    ++counter_;
  }
  void Reset() {
    counter_ = 0;
  }
};

class Clonable {
 public:
  virtual std::unique_ptr<BaseLogger> Clone() = 0;
};

template <std::invocable<unsigned int> Func> requires std::move_constructible<Func>
class ConcreteLogger : public BaseLogger {
 public:
  ConcreteLogger() = default;
  ConcreteLogger(const ConcreteLogger&) = default;
  ConcreteLogger(ConcreteLogger&&) = default;
  ConcreteLogger(Func&& func, unsigned int counter = 0) 
      : BaseLogger(counter), func_(std::forward<Func>(func)) {}
  ~ConcreteLogger() override {}
 protected:
  void Call(unsigned int value) override {
    func_(value);
  }
  Func func_;
};

template <std::invocable<unsigned int> Func> requires std::copy_constructible<Func>
class ClonableConcreteLogger : public ConcreteLogger<Func>, public Clonable {
 public:
  ClonableConcreteLogger() : ConcreteLogger<Func>() {}
  ClonableConcreteLogger(const ClonableConcreteLogger& other) : ConcreteLogger<Func>(other) {}
  ClonableConcreteLogger(ClonableConcreteLogger&& other) : ConcreteLogger<Func>(std::move(other)) {}
  ClonableConcreteLogger(const Func& func, unsigned int counter = 0) 
      : ConcreteLogger<Func>(Func(func), counter) {}
  std::unique_ptr<BaseLogger> Clone() override {
    return std::make_unique<ClonableConcreteLogger<Func>>(ConcreteLogger<Func>::func_);
  }
  ~ClonableConcreteLogger() override {}
};

// Allocator is only used in bonus SBO tests,
// ignore if you don't need bonus points.
template <class T /*, class Allocator = std::allocator<std::byte>*/ >
class Spy {
private:
  T value_;
  // Allocator allocator_;
  std::unique_ptr<BaseLogger> logger_ptr_ = nullptr;
public:
  explicit Spy(T&& value /* , const Allocator& alloc = Allocator()*/ ) : value_(std::forward<T>(value)) {}

  T& operator*() { return value_; }
  const T& operator*() const { return value_; };

  auto operator->() {
    if (logger_ptr_) {
      logger_ptr_->Add();
    }
    auto deleter = [this](T* /* ptr */) {
      if (this->logger_ptr_) {
        this->logger_ptr_->Log();
      }
    };
    return std::unique_ptr<T, decltype(deleter)>(&value_, deleter); 
  }

  /*
   * if needed (see task readme):
   *   default constructor
   *   copy and move construction
   *   copy and move assignment
   *   equality operators
   *   destructor
  */
  Spy() requires std::default_initializable<T> {}

  Spy(Spy<T>&& other) requires std::move_constructible<T>
      : value_(std::move(other.value_)), logger_ptr_(std::exchange(other.logger_ptr_, nullptr)) {}
  Spy(const Spy& other) requires std::copy_constructible<T>
      : value_(other.value_), logger_ptr_(other.CloneLogger()) {}


  Spy<T>& operator=(Spy<T>&& other) requires std::movable<T> {
    if (this == &other) { return (*this); }
    value_ = std::move(other.value_);
    logger_ptr_ = std::exchange(other.logger_ptr_, nullptr);
    return (*this);
  }
  Spy<T>& operator=(const Spy<T>& other) requires std::copyable<T> {
    if (this == &other) { return (*this); }
    value_ = other.value_;
    logger_ptr_ = other.CloneLogger();
    return (*this);
  }

  bool operator==(const Spy<T>& other) const requires std::equality_comparable<T> {
    return value_ == other.value_;
  }

  ~Spy() /* requires std::destructible<T> */ {}

  // Resets logger
  void setLogger() {
    logger_ptr_ = nullptr;
  }

  template <std::invocable<unsigned int> Logger> requires std::move_constructible<Logger> && std::movable<T> && (!std::copyable<T>)
  void setLogger(Logger&& logger) {
    logger_ptr_ = std::make_unique<ConcreteLogger<Logger>>(std::forward<Logger>(logger));
  }

  template <std::invocable<unsigned int> Logger> requires std::copy_constructible<Logger> && std::copyable<T>
  void setLogger(Logger&& logger) {
    logger_ptr_ = std::make_unique<ClonableConcreteLogger<Logger>>(logger);
  }
 private:
  inline std::unique_ptr<BaseLogger> CloneLogger() const requires std::copyable<T> {
    return (logger_ptr_ ? dynamic_cast<Clonable*>(logger_ptr_.get())->Clone() : nullptr);
  }
};
