#pragma once
#include <concepts>
#include <memory>
#include <utility>

// The small buffer
struct Storage;

// Interface for callables
template<typename Alloc>
class AbstractLogger {
public:
  virtual void operator()(unsigned int) = 0;
  virtual ~AbstractLogger() = default;
  virtual AbstractLogger* clone(Storage&, std::size_t&) = 0;
  virtual AbstractLogger* move(Storage&, Alloc&) = 0;
  virtual AbstractLogger* allocate(Alloc&) = 0;
  virtual void construct(Alloc, AbstractLogger<Alloc>*) = 0;
};

// Implementation of callables
template <std::invocable<unsigned int> Logger, typename Allocator>
class LoggerWrapper : public AbstractLogger<Allocator> {
public:
  using AllocTraits = std::allocator_traits<Allocator>;

  LoggerWrapper(const Logger& logger, const Allocator& allocator = Allocator())
    requires (std::is_same_v<std::remove_const_t<std::remove_reference_t<Logger>>&, std::remove_const_t<Logger>&>
      && std::copy_constructible<Logger>)
    : logger_(logger), allocator_(allocator) {}
  
  LoggerWrapper(Logger&& logger, const Allocator& allocator = Allocator())
    requires (std::is_same_v<std::remove_reference_t<Logger>&&, Logger&&>
      && std::movable<Logger>)
    : logger_(std::move(logger)), allocator_(allocator) {}

  LoggerWrapper(Logger&& logger, Allocator&& allocator = std::move(Allocator()))
    requires (std::is_same_v<std::remove_reference_t<Logger>&&, Logger&&>)
    : logger_(std::move(logger)), allocator_(std::move(allocator)) {}

  LoggerWrapper(const Logger& logger, Allocator&& allocator)
    requires (std::is_same_v<std::remove_const_t<std::remove_reference_t<Logger>>&, std::remove_const<Logger>&>)
    : logger_(logger), allocator_(std::move(allocator)) {}

  void operator()(unsigned int arg) override {
    logger_(arg);
  }

  AbstractLogger<Allocator>* clone(Storage& storage, std::size_t&) override;
  LoggerWrapper* move(Storage& storage, Allocator&) override; 
  AbstractLogger<Allocator>* allocate(Allocator&) override;
  void construct(Allocator, AbstractLogger<Allocator>*) override;

  ~LoggerWrapper() override = default;

private:
  Logger logger_;
  Allocator allocator_;
};

constexpr size_t storageSize = sizeof(LoggerWrapper<void(*)(unsigned int), std::allocator<std::byte>>);

struct Storage {
  alignas(16) unsigned char data[storageSize];
};

template <std::invocable<unsigned int> Logger, typename Allocator>
AbstractLogger<Allocator>* LoggerWrapper<Logger, Allocator>::clone(Storage& storage, std::size_t& loggerSize) {
  if constexpr (std::copyable<Logger>) {
    loggerSize = sizeof(LoggerWrapper<Logger, Allocator>);
    auto newLogger = loggerSize <= sizeof(storage) ?
                     reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(std::addressof(storage)) :
                     reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(allocator_.allocate(loggerSize));
    AllocTraits::construct(allocator_, newLogger, logger_, allocator_);
    return newLogger;
  }
  return nullptr;
}

template <std::invocable<unsigned int> Logger, typename Allocator>
LoggerWrapper<Logger, Allocator>* LoggerWrapper<Logger, Allocator>::move(Storage& storage, Allocator& alloc) {
  if constexpr (std::movable<Logger> && sizeof(*this) <= sizeof(storage)) {
    auto movedTo = reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(std::addressof(storage));
    AllocTraits::construct(alloc, movedTo, std::move(logger_), std::move(alloc));
    return movedTo;
  }
  return nullptr;
}

template <std::invocable<unsigned int> Logger, typename Allocator>
void LoggerWrapper<Logger, Allocator>::construct(Allocator alloc, AbstractLogger<Allocator>* destination) {
  AllocTraits::template construct<LoggerWrapper<Logger, Allocator>, Logger, Allocator&&>(alloc,
      reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(destination), std::forward<Logger>(logger_), std::move(alloc));
}

template <std::invocable<unsigned int> Logger, typename Allocator>
AbstractLogger<Allocator>* LoggerWrapper<Logger, Allocator>::allocate(Allocator& alloc) {
  return reinterpret_cast<AbstractLogger<Allocator>*>(alloc.allocate(sizeof(LoggerWrapper<Logger, Allocator>)));
}

// Log-related data
struct LogInfo {
  void null() { accessCount = 0; endOfExpression = false; expressionLogged = false; }
  LogInfo exchangeWithNull() { LogInfo old(*this); null(); return old; }
  unsigned int accessCount = 0;
  bool endOfExpression = false;
  bool expressionLogged = false;
};

// Substitute for the non-existent flag in allocators
template<typename Alloc>
struct propagate_on_copy_assignment {
  static constexpr bool Value = false;
};

template<typename Alloc>
  requires requires { typename Alloc::propagate_on_container_copy_assignment; }
struct propagate_on_copy_assignment<Alloc> {
  static constexpr bool Value = std::is_same_v<typename Alloc::propagate_on_container_copy_assignment, std::true_type>; 
};

template<typename Alloc>
constexpr bool propagate_on_copy_assignment_v = propagate_on_copy_assignment<Alloc>::Value;

// Artificial wrapper for the temporary objects to help recognize their destruction moment
template <class T, typename Allocator>
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
  AbstractLogger<Allocator>* logger_{nullptr};
};

// The stalker
template <class T, class Allocator = std::allocator<std::byte>>
class Spy {
public:
  using AllocTraits = std::allocator_traits<Allocator>;
  
  explicit Spy(T& value, const Allocator& alloc = Allocator())
    : value_(T(value)), allocator_(alloc) {}

  explicit Spy(T&& value, const Allocator& alloc = Allocator()) noexcept
    requires std::movable<T>
    : value_(std::move(value)), allocator_(std::move(alloc)) {}

  explicit Spy(const Allocator& alloc)
    : allocator_(alloc) {}

  T& operator *() { return value_; }
  const T& operator *() const { return value_; }

  Proxy<T, Allocator> operator ->() {
    if (logInfo_.endOfExpression) {
      logInfo_.null();
    }
    return Proxy<T, Allocator>(&value_, logger_, &logInfo_);
  }

  Spy() = default;

  Spy(const Spy& other)
    requires std::copyable<T>
    : value_(other.value_) {
    if constexpr (propagate_on_copy_assignment_v<Allocator>) {
      allocator_ = other.allocator_;
    }
    logger_ = other.logger_->clone(storage_, loggerSize_);
  }
  
  Spy(Spy&& other) noexcept
    requires std::movable<T>
    : value_(std::move(other.value_)) {
    if (other.logger_ == other.storage()) {
      logger_ = other.logger_->move(storage_, other.allocator_);
      AllocTraits::destroy(other.allocator_, std::exchange(other.logger_, nullptr));
    } else if (other.logger_) {
      logger_ = std::exchange(other.logger_, nullptr);
    }
    if constexpr (std::is_same_v<typename Allocator::propagate_on_container_move_assignment, std::true_type>) {
      allocator_ = std::move(other.allocator_);
    }
    logInfo_ = other.logInfo_.exchangeWithNull();
    other.logInfo_.null();
  }
  
  Spy& operator=(const Spy& other)
    requires std::copyable<T> {
    if (this == &other) {
      return *this;
    }
    cleanLogger();
    if constexpr (propagate_on_copy_assignment_v<Allocator>) {
      allocator_ = other.allocator_;
    }
    logger_ = other.logger_ ? other.logger_->clone(storage_, loggerSize_) : nullptr;
    value_ = other.value_;
    logInfo_.null();
    return *this;
  }
  
  Spy& operator=(Spy&& other)
    requires std::movable<T> {
    
    if (this == &other) {
      return *this;
    }
    cleanLogger();
    
    if (other.logger_ == other.storage()) {
      logger_ = other.logger_->move(storage_, other.allocator_);
      AllocTraits::destroy(other.allocator_, std::exchange(other.logger_, nullptr));
    } else if (other.logger_) {
      if constexpr (!std::is_same_v<typename Allocator::propagate_on_container_move_assignment, std::true_type>) {
        if (allocator_ != other.allocator_) {
          logger_ = other.logger_->allocate(allocator_);
          other.logger_->construct(allocator_, logger_);
          AllocTraits::destroy(other.allocator_, other.logger_);
          other.allocator_.deallocate(reinterpret_cast<std::byte*>(std::exchange(other.logger_, nullptr)), other.loggerSize_);
        }
      } else {
        logger_ = std::exchange(other.logger_, nullptr);
      }
    }

    allocator_ = std::move(other.allocator_);
    value_ = std::move(other.value_);
    logInfo_ = other.logInfo_.exchangeWithNull();
    return *this;
  }

  ~Spy() noexcept(std::is_nothrow_destructible_v<T>) {
    cleanLogger();
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
    cleanLogger();
    loggerSize_ = sizeof(LoggerWrapper<Logger, Allocator>);
    logger_ = reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(loggerSize_ <= storageSize ? storage() : allocator_.allocate(loggerSize_));
    AllocTraits::construct(allocator_, reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(logger_), std::forward<Logger>(logger), allocator_);
  }

private:
  void* storage() { return std::addressof(storage_); }
  
  void cleanLogger() {
    if (logger_) {
      AllocTraits::destroy(allocator_, logger_);
      if (logger_ != storage()) {
        allocator_.deallocate(reinterpret_cast<std::byte*>(logger_), loggerSize_);
      }
      logger_ = nullptr;
      loggerSize_ = 0;
    }
  }

  LogInfo logInfo_;
  std::size_t loggerSize_{0};
  Storage storage_;
  
  T value_;
  Allocator allocator_;
  AbstractLogger<Allocator>* logger_{nullptr};
};
