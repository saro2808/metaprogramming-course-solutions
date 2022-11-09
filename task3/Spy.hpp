#pragma once
#include <concepts>
#include <memory>
#include <utility>

struct Storage;

template<typename Alloc>
class AbstractLogger {
public:
  virtual void operator()(unsigned int) = 0;
  virtual ~AbstractLogger() = default;
  virtual AbstractLogger* clone(Storage& storage) = 0;
  virtual AbstractLogger* move_to(Storage& storage, Alloc& alloc) = 0;
  virtual AbstractLogger* allocate(Alloc&) = 0;
  virtual void construct(Alloc, AbstractLogger<Alloc>*) = 0;
};

template <std::invocable<unsigned int> Logger, typename Allocator>
class LoggerWrapper : public AbstractLogger<Allocator> {
public:
  LoggerWrapper(const std::remove_reference_t<Logger>& logger, const Allocator& allocator = Allocator())
    requires std::copy_constructible<Logger>
    : logger_(logger), allocator_(allocator) {}
  
  LoggerWrapper(Logger&& logger, const Allocator& allocator = Allocator())
    requires std::movable<Logger>
    : logger_(std::move(logger)), allocator_(allocator) {}
  
  void operator()(unsigned int arg) override {
    logger_(arg);
  }

  AbstractLogger<Allocator>* clone(Storage& storage) override;
  LoggerWrapper* move_to(Storage& storage, Allocator&) override; 
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
AbstractLogger<Allocator>* LoggerWrapper<Logger, Allocator>::clone(Storage& storage) {
  if constexpr (std::copyable<Logger>) {
    if constexpr (sizeof(*this) <= sizeof(storage)) {
      std::allocator_traits<Allocator>::construct(allocator_, reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(std::addressof(storage)), logger_, allocator_);
      return reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(std::addressof(storage));
    } else {
      auto newLogger = reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(allocator_.allocate(sizeof(LoggerWrapper<Logger, Allocator>)));
      std::allocator_traits<Allocator>::construct(allocator_, reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(newLogger), logger_, allocator_);
      return newLogger;
    }
  } return nullptr;
}

template <std::invocable<unsigned int> Logger, typename Allocator>
LoggerWrapper<Logger, Allocator>* LoggerWrapper<Logger, Allocator>::move_to(Storage& storage, Allocator& alloc) {
  if constexpr (std::movable<Logger> && sizeof(*this) <= sizeof(storage)) {
    std::allocator_traits<Allocator>::construct(alloc, reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(std::addressof(storage)), std::move(logger_), alloc);
    return reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(std::addressof(storage));
  }
  return nullptr;
}

template <std::invocable<unsigned int> Logger, typename Allocator>
void LoggerWrapper<Logger, Allocator>::construct(Allocator alloc, AbstractLogger<Allocator>* destination) {
  std::allocator_traits<Allocator>::template construct<LoggerWrapper<Logger, Allocator>, std::remove_reference_t<Logger>&&, Allocator&&>(
      alloc,
      reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(destination),
      std::move(logger_),
      std::move(alloc));
}

template <std::invocable<unsigned int> Logger, typename Allocator>
AbstractLogger<Allocator>* LoggerWrapper<Logger, Allocator>::allocate(Allocator& alloc) {
  return reinterpret_cast<AbstractLogger<Allocator>*>(alloc.allocate(sizeof(LoggerWrapper<Logger, Allocator>)));
}

struct LogInfo {
  inline void null() { accessCount = 0; endOfExpression = false; expressionLogged = false; }
  inline LogInfo exchangeWithNull() { LogInfo old(*this); null(); return old; }
  unsigned int accessCount = 0;
  bool endOfExpression = false;
  bool expressionLogged = false;
};

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

template <class T, class Allocator = std::allocator<std::byte>>
class Spy {
public:
  explicit Spy(T& value, const Allocator& alloc = Allocator())
    : value_(T(value)), allocator_(alloc) {}

  explicit Spy(T&& value, const Allocator& alloc = Allocator()) noexcept
    requires std::movable<T>
    : value_(std::move(value)), allocator_(alloc) {}

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
    if (other.logger_) {
      logger_ = other.logger_->clone(storage_);
    }
  }
  
  Spy(Spy&& other) noexcept
    requires std::movable<T>
    : value_(std::move(other.value_)) {
    logInfo_ = other.logInfo_.exchangeWithNull();
    if constexpr (std::is_same_v<typename Allocator::propagate_on_container_move_assignment, std::true_type>) {
      allocator_ = std::move(other.allocator_);
    }
    if (other.logger_ == other.storage()) {
      logger_ = other.logger_->move_to(storage_, other.allocator_);
      std::allocator_traits<Allocator>::destroy(allocator_, reinterpret_cast<AbstractLogger<Allocator>*>(std::exchange(other.logger_, nullptr)));
    } else if (other.logger_) {
      logger_ = std::exchange(other.logger_, nullptr);
    }
    other.logInfo_.null();
  }
  
  Spy& operator=(const Spy& other)
    requires std::copyable<T> {
    if (this == &other) {
      return *this;
    }
    if constexpr (propagate_on_copy_assignment_v<Allocator>) {
      allocator_ = other.allocator_;
    }
    if (other.logger_) {
      logger_ = other.logger_->clone(storage_);
    } else {
      logger_ = nullptr;
    }
    value_ = other.value_;
    logInfo_.null();
    return *this;
  }
  
  Spy& operator=(Spy&& other)
    requires std::movable<T> {
    if (this == &other) {
      return *this;
    }
    if constexpr (!std::is_same_v<typename Allocator::propagate_on_container_move_assignment, std::true_type>) {
      if (allocator_ != other.allocator_) {
        allocator_ = std::move(other.allocator_);
        if (other.logger_) {
          if (other.logger_ != other.storage()) {
            logger_ = other.logger_->allocate(allocator_);
            other.logger_->construct(allocator_, logger_);
            std::allocator_traits<Allocator>::destroy(other.allocator_, other.logger_);
            other.allocator_.deallocate(reinterpret_cast<std::byte*>(other.logger_), sizeof(AbstractLogger<Allocator>));
            other.logger_ = nullptr;
          } else {
            logger_ = other.logger_->move_to(storage_, allocator_);
            std::allocator_traits<Allocator>::destroy(other.allocator_, reinterpret_cast<AbstractLogger<Allocator>*>(std::exchange(other.logger_, nullptr)));
            other.logger_ = nullptr;
          }
        }
      }
    } else {
      allocator_ = std::move(other.allocator_);
      if (other.logger_ == other.storage()) {
        logger_ = other.logger_->move_to(storage_, other.allocator_);
        std::allocator_traits<Allocator>::destroy(allocator_, reinterpret_cast<AbstractLogger<Allocator>*>(std::exchange(other.logger_, nullptr)));
      } else {
        logger_ = std::exchange(other.logger_, nullptr);
      }
    }
    value_ = std::move(other.value_);
    logInfo_ = other.logInfo_.exchangeWithNull();
    return *this;
  }

  ~Spy() noexcept(std::is_nothrow_destructible_v<T>) {
    if (logger_ == storage()) {
      std::allocator_traits<Allocator>::destroy(allocator_, static_cast<AbstractLogger<Allocator>*>(logger_));
    } else if (logger_) {
      std::allocator_traits<Allocator>::destroy(allocator_, static_cast<AbstractLogger<Allocator>*>(logger_));
      allocator_.deallocate(reinterpret_cast<std::byte*>(logger_), sizeof(AbstractLogger<Allocator>));
    }
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
    if constexpr (sizeof(LoggerWrapper<Logger, Allocator>) <= storageSize) {
      std::allocator_traits<Allocator>::construct(allocator_, reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(storage()), std::forward<Logger>(logger), allocator_);
      logger_ = reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(storage());
    } else {
      logger_ = reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(allocator_.allocate(sizeof(LoggerWrapper<Logger, Allocator>)));
      std::allocator_traits<Allocator>::construct(allocator_, reinterpret_cast<LoggerWrapper<Logger, Allocator>*>(logger_), std::forward<Logger>(logger), allocator_);
    }
  }

private:
  LogInfo logInfo_;
  Storage storage_;
  void* storage() { return std::addressof(storage_); }
  
  T value_;
  Allocator allocator_;
  AbstractLogger<Allocator>* logger_{nullptr};
};
