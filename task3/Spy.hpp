#pragma once
#include <concepts>
#include <memory>

template <bool A, bool B>
concept Implies = !A || (A && B); // A -> B

class AbstractLogger {
public:
  virtual void operator()(unsigned int) = 0;
  virtual ~AbstractLogger() = default;
  virtual std::unique_ptr<AbstractLogger> clone() = 0;
};

template <std::invocable<unsigned int> Logger>
class LoggerWrapper : public AbstractLogger {
public:
  template<class L = Logger>
  LoggerWrapper(L&& logger) : logger_{std::forward<Logger>(logger)} {}

  void operator()(unsigned int arg) override {
    logger_(arg);
  }

  std::unique_ptr<AbstractLogger> clone() override {
    return std::make_unique<LoggerWrapper>(logger_);
  }

private:
  Logger logger_;
};

struct LogInfo {
  void null() { accessCount = 0; endOfExpression = false; expressionLogged = false; }
  unsigned int accessCount = 0;
  bool endOfExpression = false;
  bool expressionLogged = false;
};

template <class T>
class Spy;

template <class T>
class Proxy {
public:
  Proxy(Spy<T>* spy)
  : spy_{spy} {}
  
  T* operator->() {
    ++(*spy_).logInfo_.accessCount;
    return &((*spy_).value_);
  }

  ~Proxy() {
    (*spy_).logInfo_.endOfExpression = true;
    if ((*spy_).logger_ && !(*spy_).logInfo_.expressionLogged) {
      (*((*spy_).logger_))((*spy_).logInfo_.accessCount);
      (*spy_).logInfo_.expressionLogged = true;
    }
  }
  
private:
  Spy<T>* spy_{nullptr};
};

template <class T>
class Spy {
public:
  explicit Spy(T& value)
    requires std::copyable<T>
    : value_{value} {}

  explicit Spy(T&& value) noexcept
    requires std::movable<T>
    : value_{std::move(value)} {}

  T& operator*() { return value_; }
  const T& operator*() const { return value_; }

  friend class Proxy<T>;

  Proxy<T> operator->() {
    if (logInfo_.endOfExpression)
      logInfo_.null();
    return Proxy<T>(this);
  }

  Spy() = default;

  Spy(Spy const& other)
    requires std::copyable<T>
    : logger_{other.logger_ ? other.logger_->clone() : nullptr},
      value_{other.value_}, logInfo_{other.logInfo_} {}
  
  Spy(Spy&&) noexcept
    requires std::movable<T> = default;
  
  Spy& operator=(const Spy& other)
    requires std::assignable_from<T&, T> {
    if (this != &other) {
      value_ = other.value_;
      logger_ = other.logger_ ? other.logger_->clone() : nullptr;
      logInfo_.null();
    }
    return *this;
  }
  
  Spy& operator=(Spy&& other)
    requires std::movable<T> {
    if (this != &other) {
      value_ = std::move(other.value_);
      logger_ = std::move(other.logger_);
      other.logInfo_.null();
      logInfo_.null();
    }
    return *this;
  }

  ~Spy() noexcept(std::is_nothrow_destructible_v<T>) {}

  constexpr bool operator==(const Spy& other) const
    requires std::equality_comparable<T> {
    return value_ == other.value_;
  }

  template <std::invocable<unsigned int> Logger>
    requires (Implies<std::copyable<T>, std::copy_constructible<Logger>> &&
              Implies<std::movable<T> , std::move_constructible<Logger>>)
  void setLogger(Logger&& logger) {
    logger_ = std::make_unique<LoggerWrapper<Logger>>(std::forward<Logger>(logger));
  }

private:
  LogInfo logInfo_; 
  T value_;
  std::unique_ptr<AbstractLogger> logger_{nullptr};
};
