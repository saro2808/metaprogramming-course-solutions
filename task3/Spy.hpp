#pragma once
#include <concepts>
#include <memory>
#include <utility>
#include <array>

template <bool A, bool B>
concept Implies = !A || B; // A -> B

constexpr std::size_t SMALL_BUFFER_SIZE = 64;

// Log-related data
struct LogInfo {
  void null() { accessCount = 0; endOfExpression = false; expressionLogged = false; }
  LogInfo exchangeWithNull() { LogInfo old(*this); null(); return old; }
  unsigned int accessCount = 0;
  bool endOfExpression = false;
  bool expressionLogged = false;
};

template <class T, typename Allocator>
class Spy;

// Artificial wrapper for the temporary objects to help recognize their destruction moment
template <class T, typename Allocator>
class Proxy {
public:
  Proxy(Spy<T, Allocator>* spy) : spy_{spy} {}
  
  T* operator->() {
    ++(*spy_).logInfo_.accessCount;
    return &((*spy_).value_);
  }

  ~Proxy() {
    (*spy_).logInfo_.endOfExpression = true;
    if ((*spy_).call_ && !(*spy_).logInfo_.expressionLogged) {
      (*spy_).call_((*spy_).logger_, (*spy_).logInfo_.accessCount);
      (*spy_).logInfo_.expressionLogged = true;
    }
  }
  
private:
  Spy<T, Allocator>* spy_;
};

// The stalker
template <class T, class Allocator = std::allocator<std::byte>>
class Spy {
public:
  using AllocTraits = std::allocator_traits<Allocator>;
  
  friend class Proxy<T, Allocator>;

  Spy(T& value, const Allocator& alloc = Allocator()) noexcept
    requires std::copyable<T>
    : value_{value}, allocator_{alloc} {}

  Spy(T&& value, const Allocator& alloc = Allocator()) noexcept
    requires std::movable<T>
    : value_{std::move(value)}, allocator_{alloc} {}

  explicit Spy(const Allocator& alloc) noexcept
    : allocator_{alloc} {}

  T& operator*() { return value_; }
  const T& operator*() const { return value_; }

  Proxy<T, Allocator> operator->() {
    if (logInfo_.endOfExpression)
      logInfo_.null();
    return Proxy<T, Allocator>(this);
  }

  Spy() = default;

  Spy(const Spy& other)
    requires std::copyable<T>
    : value_{other.value_} {

    if (other.copy_)
      other.copy_(logger_, other.logger_, other.allocator_);
    call_ = other.call_;
    destructor_ = other.destructor_;
    copy_ = other.copy_;
    move_ = other.move_;
  }
  
  Spy(Spy&& other) noexcept
    requires std::move_constructible<T>
    : value_{std::move(other.value_)} {
    
    if (other.move_)
      other.move_(logger_, other.logger_, other.allocator_);
    call_ = std::exchange(other.call_, nullptr);
    destructor_ = std::exchange(other.destructor_, nullptr);
    copy_ = std::exchange(other.copy_, nullptr);
    move_ = std::exchange(other.move_, nullptr);

    allocator_ = other.allocator_;
  }

  Spy& operator=(const Spy& other)
    requires std::copyable<T> {
    if (this != &other) {
      clearLogger();
   
      destructor_ = other.destructor_;
      call_ = other.call_; 
      copy_ = other.copy_;
      move_ = other.move_;
    
      if constexpr (std::is_same_v<typename AllocTraits::propagate_on_container_copy_assignment, std::true_type>)
        allocator_ = other.allocator_;
      
      if (copy_)
        copy_(logger_, other.logger_, allocator_);
      
      value_ = other.value_;
      logInfo_.null();
    }
    return *this;
  }
  
  Spy& operator=(Spy&& other)
    requires std::movable<T> {
    if (this != &other) {
      clearLogger();
      
      call_ = std::exchange(other.call_, nullptr);
      copy_ = std::exchange(other.copy_, nullptr);
      move_ = std::exchange(other.move_, nullptr);
      destructor_ = other.destructor_;

      if constexpr (!std::is_same_v<typename AllocTraits::propagate_on_container_move_assignment, std::true_type>) {
        if (allocator_ != other.allocator_ && copy_) {
          copy_(logger_, other.logger_, allocator_);
          allocator_ = other.allocator_;
          other.clearLogger();
        } else if (move_) {
          move_(logger_, other.logger_, other.allocator_);
          other.destructor_ = nullptr;
        }
      } else if (move_) {
          move_(logger_, other.logger_, other.allocator_);
          other.destructor_ = nullptr;
      }
    
      value_ = std::move(other.value_);
      logInfo_ = other.logInfo_.exchangeWithNull();
    }
    return *this;
  }

  ~Spy() noexcept(std::is_nothrow_destructible_v<T>) {
    clearLogger();
  }

  constexpr bool operator==(const Spy& other) const
    requires std::equality_comparable<T> {
    return value_ == other.value_;
  }

  template <std::invocable<unsigned int> Logger>
    requires (Implies<std::copyable<T>, std::copy_constructible<Logger>> &&
              Implies<std::movable<T> , std::move_constructible<Logger>>)
  void setLogger(Logger&& logger) {
    using LoggerNoRef = std::remove_reference_t<Logger>;
    clearLogger();
    if constexpr (sizeof(Logger) <= SMALL_BUFFER_SIZE) {
      AllocTraits::construct(allocator_, std::launder(reinterpret_cast<LoggerNoRef*>(logger_.staticLogger.data())), std::forward<Logger>(logger));
    } else {
      logger_.dynamicLogger = allocator_.allocate(sizeof(Logger));
      AllocTraits::construct(allocator_, reinterpret_cast<LoggerNoRef*>(logger_.dynamicLogger), std::forward<Logger>(logger));
    }
    call_ = &call<LoggerNoRef>;
    copy_ = &copy<LoggerNoRef>;
    move_ = &move<LoggerNoRef>;
    destructor_ = &destroy<LoggerNoRef>;
  }

private:
  union LoggerType {
    void* dynamicLogger;
    alignas(64) std::array<std::byte, SMALL_BUFFER_SIZE> staticLogger;
  };

  template<class L>
  static void call(LoggerType& ptr, unsigned int arg) {
    if constexpr (sizeof(L) < SMALL_BUFFER_SIZE) {
      L* logger = std::launder(reinterpret_cast<L*>(ptr.staticLogger.data()));
      (*logger)(arg);
    } else {
      (*static_cast<L*>(ptr.dynamicLogger))(arg);
    }
  }
  
  template<class L>
  static void destroy(LoggerType& ptr, Allocator allocator) {
    if constexpr (sizeof(L) < SMALL_BUFFER_SIZE) {
      AllocTraits::destroy(allocator, std::launder(reinterpret_cast<L*>(ptr.staticLogger.data())));
    } else {
      AllocTraits::destroy(allocator, static_cast<L*>(ptr.dynamicLogger));
      allocator.deallocate(static_cast<std::byte*>(ptr.dynamicLogger), sizeof(L));
    }
  }

  template<class L>
  static void copy(LoggerType& to, const LoggerType& from, Allocator allocator) {
    if constexpr (std::copy_constructible<L>) {
      if constexpr (sizeof(L) < SMALL_BUFFER_SIZE) {
        AllocTraits::construct(allocator, std::launder(reinterpret_cast<L*>(to.staticLogger.data())), *std::launder(reinterpret_cast<const L*>(from.staticLogger.data())));
      } else {
        to.dynamicLogger = allocator.allocate(sizeof(L));
        AllocTraits::construct(allocator, static_cast<L*>(to.dynamicLogger), *static_cast<L*>(from.dynamicLogger));
      }
    }
  }

  template<class L>
  static void move(LoggerType& to, LoggerType& from, Allocator allocator) {
    if constexpr (sizeof(L) < SMALL_BUFFER_SIZE) {
      auto* logger = const_cast<L*>(reinterpret_cast<const L*>(from.staticLogger.data()));
      AllocTraits::construct(allocator, std::launder(reinterpret_cast<L*>(to.staticLogger.data())), std::move(*logger));
      AllocTraits::destroy(allocator, logger);
    } else {
      to.dynamicLogger = std::exchange(from.dynamicLogger, nullptr);
    }
  }

  void clearLogger() {
    if (destructor_) {
      destructor_(logger_, allocator_);
      call_ = nullptr;
      copy_ = nullptr;
      move_ = nullptr;
      destructor_ = nullptr;
    }
  }

  void(*call_)(LoggerType&, unsigned int) {nullptr};
  void(*copy_)(LoggerType&, const LoggerType&, Allocator) {nullptr};
  void(*move_)(LoggerType&, LoggerType&, Allocator) {nullptr};
  void(*destructor_)(LoggerType&, Allocator) {nullptr};

  LogInfo logInfo_;
  
  T value_;
  Allocator allocator_;

  LoggerType logger_{nullptr};
};
