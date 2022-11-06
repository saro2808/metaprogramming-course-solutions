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

struct LogInfo {
  void null() {accessCount = 0; endOfExpression = false; expressionLogged = false;}
  unsigned int accessCount = 0;
  bool endOfExpression = false;
  bool expressionLogged = false;
};

template <class T>
class Proxy {
public:
  template <std::invocable<unsigned int> Logger>
  Proxy(T* value, std::shared_ptr<Logger> logger, LogInfo* logInfo)
  : value_(value), logger_(logger), logInfo_(logInfo) {}
  
  T* operator->() {
    ++logInfo_->accessCount;
    return value_;
  }

  ~Proxy() {
    logInfo_->endOfExpression = true;
    if (logger_ && !logInfo_->expressionLogged) {
      (*logger_)(logInfo_->accessCount);
      logInfo_->expressionLogged = true;
    }
  }
  
private:
  LogInfo* logInfo_{nullptr};
  
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
    if (logInfo_.endOfExpression) {
      logInfo_.null();
    }
    return Proxy<T>(&value_, logger_, &logInfo_);
  }

  Spy() = default;
  Spy(const Spy&) = default;
  
  Spy(Spy&&) noexcept
    requires std::movable<T> = default;
  
  Spy& operator=(const Spy& other)
    requires std::assignable_from<T&, T> {
    value_ = other.value_;
    logger_ = other.logger_;
    logInfo_.null();
    return *this;
  }
  
  Spy& operator=(Spy&& other)
    requires std::movable<T> {
    value_ = std::move(other.value_);
    logger_ = std::move(other.logger_);
    other.logInfo_.null();
    logInfo_.null();
    return *this;
  }

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
  LogInfo logInfo_;
  
  T value_;
  // Allocator allocator_;
  std::shared_ptr<AbstractLogger> logger_{nullptr};
};
