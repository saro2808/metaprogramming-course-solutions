#pragma once
#include <concepts>
#include <memory>
#include <utility>

struct Storage;

class AbstractLogger {
public:
  virtual void operator()(unsigned int) = 0;
  virtual ~AbstractLogger() = default;
  virtual AbstractLogger* clone(Storage& storage) = 0;
  virtual AbstractLogger* move_to(Storage& storage) = 0;
};

template <std::invocable<unsigned int> Logger>
class LoggerWrapper : public AbstractLogger {
public:
  explicit LoggerWrapper(const Logger& logger)
    requires std::copyable<Logger> 
    : logger_(logger) {}
  
  LoggerWrapper(Logger&& logger)
    requires std::movable<Logger>
    : logger_(std::move(logger)) {}
  
  void operator()(unsigned int arg) override {
    logger_(arg);
  }

  AbstractLogger* clone(Storage& storage) override;
  LoggerWrapper* move_to(Storage& storage) override; 
  
  ~LoggerWrapper() override = default;
private:
  Logger logger_;
};

constexpr size_t storageSize = sizeof(LoggerWrapper<void(*)(unsigned int)>);

struct Storage {
  alignas(16) unsigned char data[storageSize];
};

template <std::invocable<unsigned int> Logger>
AbstractLogger* LoggerWrapper<Logger>::clone(Storage& storage) {
  if constexpr (std::copyable<Logger>) {
    if constexpr (sizeof(*this) <= sizeof(storage)) {
      return new(std::addressof(storage)) LoggerWrapper(logger_);
    } else {
      return new LoggerWrapper(logger_);
    }
  }
  return nullptr;
}

template <std::invocable<unsigned int> Logger>
LoggerWrapper<Logger>* LoggerWrapper<Logger>::move_to(Storage& storage) {
  if constexpr (sizeof(*this) <= sizeof(storage)) {
    return new(std::addressof(storage)) LoggerWrapper(std::move(logger_));
  } else {
    return nullptr;
  }
}

struct LogInfo {
  inline void null() { accessCount = 0; endOfExpression = false; expressionLogged = false; }
  inline LogInfo exchangeWithNull() { LogInfo old(*this); null(); return old; }
  unsigned int accessCount = 0;
  bool endOfExpression = false;
  bool expressionLogged = false;
};

template <class T>
class Proxy {
public:
  template <std::invocable<unsigned int> Logger>
  Proxy(T* value, Logger* logger, LogInfo* logInfo)
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
  AbstractLogger* logger_{nullptr};
};

template <class T/*, class Allocator = std::allocator<T>*/>
class Spy {
public:
  explicit Spy(T& value/*, const Allocator& alloc = Allocator()*/)
    : value_(T(value))/*, allocator_(alloc) */{}

  explicit Spy(T&& value/*, const Allocator& alloc = Allocator()*/) noexcept
    requires std::movable<T>
    : value_(std::move(value))/*, allocator_(alloc) */{}

  T& operator *() { return value_; }
  const T& operator *() const { return value_; }

  Proxy<T> operator ->() {
    if (logInfo_.endOfExpression) {
      logInfo_.null();
    }
    return Proxy<T>(&value_, logger_, &logInfo_);
  }

  Spy() = default;
  
  Spy(const Spy& other)
    requires std::copyable<T>
    : value_(other.value_) {
    if (other.logger_) {
      logger_ = other.logger_->clone(storage_);
    }
  }
  
  Spy(Spy&& other) noexcept
    requires std::movable<T>
    : value_(std::move(other.value_)) {
    if (other.logger_ == other.storage()) {
      logger_ = other.logger_->move_to(storage_);
      std::exchange(other.logger_, nullptr)->~AbstractLogger();
    } else if (other.logger_) {
      logger_ = std::exchange(other.logger_, nullptr);
    }
    other.logInfo_.null();
  }
  
  Spy& operator=(const Spy& other)
    requires (std::copyable<T>) {
    if (this == &other) {
      return *this;
    } else if (other.logger_) {
      logger_ = other.logger_->clone(storage_);
    } else {
      logger_ = nullptr;
    }
    value_ = other.value_;
    // if ()
    //allocator_ = other.allocator_; 
    logInfo_.null();
    return *this;
  }
  
  Spy& operator=(Spy&& other)
    requires (std::movable<T>) {
    if (this == &other) {
      return *this;
    } else if (other.logger_ == other.storage()) {
      logger_ = other.logger_->move_to(storage_);
      std::exchange(other.logger_, nullptr)->~AbstractLogger();
    } else {
      logger_ = std::exchange(other.logger_, nullptr);
    }
    // if ()
    //allocator_ = other.allocator_;
    value_ = std::move(other.value_);
    logInfo_ = other.logInfo_.exchangeWithNull();
    return *this;
  }

  ~Spy() noexcept(std::is_nothrow_destructible_v<T>) {
    if (logger_ == storage()) {
      logger_->~AbstractLogger();
    } else {
      delete logger_;
    }
    /*if (logger_) {
      std::allocator_traits<Allocator>::template destroy<std::byte>(allocator_, logger_);
      allocator_.deallocate(logger_, ??? sizeof(logger_));
    }*/
  }

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
    if constexpr (sizeof(LoggerWrapper<Logger>) <= storageSize) {
      logger_ = new(storage()) LoggerWrapper(std::forward<Logger>(logger));
    } else {
      logger_ = new LoggerWrapper(std::forward<Logger>(logger));
    }
    //logger_ = allocator_.allocate(sizeof(Logger));
    //std::allocator_traits<Allocator>::template rebind_alloc<std::byte>::template construct<Logger, Logger>(allocator_, logger_, std::forward<Logger>(logger));
  }
private:
  LogInfo logInfo_;
  Storage storage_;
  
  void* storage() { return std::addressof(storage_); }
  
  T value_;
  //Allocator allocator_;
  AbstractLogger* logger_{nullptr};
};
