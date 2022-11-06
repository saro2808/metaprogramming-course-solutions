#pragma once
#include <concepts>
#include <memory>

class AbstractLogger {
public:
  virtual void operator()(unsigned int) = 0;
  virtual ~AbstractLogger() = default;
};

template <std::invocable<unsigned int> Logger>
class LoggerWrapper : public AbstractLogger {
public:
  template <class L = Logger>
  explicit LoggerWrapper(L&& logger)
    : logger_(std::forward<L>(logger)) {}
  
  void operator()(unsigned int arg) override {
    logger_(arg);
  }

private:
  Logger logger_;
};

template <class T>
class Proxy {
public:
  template <std::invocable<unsigned int> Logger>
  Proxy(T* value, std::shared_ptr<Logger> logger, unsigned int* accessCount, bool* endOfExpression, bool* expressionLogged)
  : value_(value), logger_(logger), 
  accessCount_(accessCount), endOfExpression_(endOfExpression), expressionLogged_(expressionLogged) {}
  
  T* operator->() {
    ++*accessCount_;
    return value_;
  }

  ~Proxy() {
    *endOfExpression_ = true;
    if (logger_ && !*expressionLogged_) {
      (*logger_)(*accessCount_);
      *expressionLogged_ = true;
    }
  }
  
private:
  bool* endOfExpression_ = nullptr;
  bool* expressionLogged_ = nullptr;
  unsigned int* accessCount_ = nullptr;
  
  T* value_{nullptr};
  std::shared_ptr<AbstractLogger> logger_{nullptr};
};

// Allocator is only used in bonus SBO tests,
// ignore if you don't need bonus points.
template <class T /*, class Allocator = std::allocator<T>*/ >
class Spy {
public:
  explicit Spy(T& value /* , const Allocator& alloc = Allocator()*/ )
    : value_(T(value)) {}

  explicit Spy(T&& value) noexcept
    requires std::movable<T>
    : value_(std::move(value)) {}

  T& operator *() { return value_; }
  const T& operator *() const { return value_; }

  Proxy<T> operator ->() {
    if (endOfExpression_) {
      accessCount_ = 0;
      endOfExpression_ = false;
      expressionLogged_ = false;
    }
    return Proxy<T>(&value_, logger_,
        &accessCount_, &endOfExpression_, &expressionLogged_);
  }

  Spy() = default;
  Spy(const Spy&) = default;
  
  Spy(Spy&&) noexcept
    requires std::movable<T> = default;
  
  Spy& operator=(const Spy& other)
    requires std::assignable_from<T&, T> {
    value_ = other.value_;
    logger_ = other.logger_;
    endOfExpression_ = false;
    expressionLogged_ = false;
    accessCount_ = 0;
    return *this;
  }
  
  Spy& operator=(Spy&&) noexcept
    requires std::movable<T> = default;

  ~Spy() noexcept(std::is_nothrow_destructible_v<T>) {}

  constexpr bool operator==(const Spy& other) const
    requires std::equality_comparable<T> {
    return value_ == other.value_;
  }

  constexpr bool operator!=(const Spy& other) const
    requires std::equality_comparable<T> {
    return value_ != other.value_;
  }

  template <std::invocable<unsigned int> Logger>
    requires ((std::copyable<T> && std::copy_constructible<Logger>) ||
              (!std::copyable<T> && std::movable<T> && std::move_constructible<Logger>) ||
              std::is_same_v<Logger, void(*)(unsigned int)>)
  void setLogger(Logger&& logger) {
    logger_ = std::make_shared<LoggerWrapper<Logger>>(std::forward<Logger>(logger));
  }

private:
  T value_;
  // Allocator allocator_;
  std::shared_ptr<AbstractLogger> logger_{nullptr};

  bool endOfExpression_ = false;
  bool expressionLogged_ = false;
  unsigned int accessCount_ = 0;
};
