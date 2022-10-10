#include <span>
#include <concepts>
#include <cstdlib>
#include <array>
#include <iterator>


inline constexpr std::ptrdiff_t dynamic_stride = -1;

<<<<<<< HEAD

template<typename T, T value>
class CompileTimeStorage {
public:
  constexpr CompileTimeStorage() = default;
  constexpr CompileTimeStorage(T) noexcept {}

  constexpr const T operator()() const {
    return value;
  }
};

template<typename T>
class RuntimeStorage {
public:
  constexpr RuntimeStorage() = default;
  constexpr RuntimeStorage(T value) noexcept : value_(value) {}

  constexpr T& operator()() {
    return value_;
  }

  constexpr const T& operator()() const {
    return value_;
  }

private:
  T value_;
};

template<std::size_t value>
class ExtentStorage: public CompileTimeStorage<std::size_t, value> {
public:
  constexpr ExtentStorage() = default;
};

template<>
class ExtentStorage<std::dynamic_extent>: public RuntimeStorage<std::size_t> {
public:
  constexpr ExtentStorage() = default;
};

template<std::ptrdiff_t value>
class StrideStorage: public CompileTimeStorage<std::ptrdiff_t, value> {
public:
  constexpr StrideStorage() = default;
};

template<>
class StrideStorage<dynamic_stride>: public RuntimeStorage<std::ptrdiff_t> {
public:
  constexpr StrideStorage() = default;
};
=======
>>>>>>> 860ea5f (Revert "Task0")

template
  < class T
  , std::size_t extent = std::dynamic_extent
  , std::ptrdiff_t stride = 1
  >
class Slice {
public:
  template<class U>
  Slice(U& container);

  template <std::contiguous_iterator It>
  Slice(It first, std::size_t count, std::ptrdiff_t skip);

  // Data, Size, Stride, begin, end, casts, etc...

  Slice<T, std::dynamic_extent, stride>
    First(std::size_t count) const;

  template <std::size_t count>
  Slice<T, /*?*/, stride>
    First() const;

  Slice<T, std::dynamic_extent, stride>
    Last(std::size_t count) const;

  template <std::size_t count>
  Slice<T, /*?*/, stride>
    Last() const;

  Slice<T, std::dynamic_extent, stride>
    DropFirst(std::size_t count) const;

  template <std::size_t count>
  Slice<T, /*?*/, stride>
    DropFirst() const;

  Slice<T, std::dynamic_extent, stride>
    DropLast(std::size_t count) const;

  template <std::size_t count>
  Slice<T, /*?*/, stride>
    DropLast() const;

  Slice<T, /*?*/, /*?*/>
    Skip(std::ptrdiff_t skip) const;

  template <std::ptrdiff_t skip>
  Slice<T, /*?*/, /*?*/>
    Skip() const;

private:
  T* data_;
  // std::size_t extent_; ?
  // std::ptrdiff_t stride_; ?
};
