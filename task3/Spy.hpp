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

  explicit Spy(T&& value) : value_(std::move(value)) {}

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

  /*
   * if needed (see task readme):
   *   default constructor
   *   copy and move construction
   *   copy and move assignment
   *   equality operators
   *   destructor
  */

  Spy() = default;
  Spy(const Spy&) = default;
  Spy(Spy&&) = default;
  
  Spy& operator=(const Spy& other) {
    value_ = other.value_;
    logger_ = other.logger_/*std::make_shared(*other.logger_)*/;
    endOfExpression_ = false/*other.endOfExpression_*/;
    expressionLogged_ = false/*other.expressionLogged_*/;
    accessCount_ = 0/*other.accessCount_*/;
    return *this;
  }
  
  Spy& operator=(Spy&&) = default;
  ~Spy() noexcept {}

  template <std::invocable<unsigned int> Logger>
    /* requires  see task readme */
  void setLogger(Logger&& logger) {
    logger_ = /*static_pointer_cast<AbstractLogger, LoggerWrapper<Logger>>(*/
        std::make_shared<LoggerWrapper<Logger>>(std::forward<Logger>(logger))/*)*/;
  }

private:
  T value_;
  // Allocator allocator_;
  std::shared_ptr<AbstractLogger> logger_{nullptr};

  bool endOfExpression_ = false;
  bool expressionLogged_ = false;
  unsigned int accessCount_ = 0;
};
