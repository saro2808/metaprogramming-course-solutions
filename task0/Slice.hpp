#include <span>
#include <concepts>
#include <cstdlib>
#include <array>
#include <iterator>


inline constexpr std::ptrdiff_t dynamic_stride = -1;


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

template
  < class T
  , std::size_t extent = std::dynamic_extent
  , std::ptrdiff_t stride = 1
  >
class Slice {
public:
  using value_type = std::remove_cv_t<T>;
  using element_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = element_type&;
  using const_reference = const element_type&;

  // constructors
  Slice() = default;
  Slice(const Slice&) = default;
  explicit Slice(Slice&&) = default;

  template<typename Type, size_t Size>
  constexpr Slice(std::array<Type, Size>& array) {
    if constexpr (stride == dynamic_stride)      stride_() = 1;
    if constexpr (extent == std::dynamic_extent) extent_() = array.size() / stride_();
    data_ = array.data();
  }

  template<class U>
  constexpr Slice(U& container)
    requires(
      (std::is_same_v<typename U::value_type, T> ||
      std::is_same_v<typename U::value_type, std::remove_const_t<T>>) &&
      extent == std::dynamic_extent &&
      requires { container.size(); container.data(); }
    ) {
    if constexpr (stride == dynamic_stride) { stride_() = 1; }
    extent_() = container.size() / stride_();
    data_ = container.data();
  }

  template <std::contiguous_iterator It>
  Slice(It first, std::size_t count, std::ptrdiff_t skip)
  : data_(&*first) {
    if constexpr (stride == dynamic_stride)      stride_() = skip;
    if constexpr (extent == std::dynamic_extent) extent_() = count;
  }

  ~Slice() noexcept = default;

  // getters
  constexpr pointer Data() const noexcept { return data_; }
  constexpr size_type Size() const noexcept { return extent_(); }
  constexpr difference_type Stride() const noexcept { return stride_(); }

  class iterator: public std::random_access_iterator_tag {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Slice::value_type;
    using element_type = Slice::element_type;
    using size_type = Slice::size_type;
    using difference_type = Slice::difference_type;
    using pointer = Slice::pointer;
    using const_pointer = Slice::const_pointer;
    using reference = Slice::reference;
    using const_reference = Slice::const_reference;
    
    iterator() = default;
    iterator(pointer ptr, const Slice* slc) : ptr_(ptr), slc_(slc) {}    
    ~iterator() = default;
    
    reference operator*() const { return *ptr_; }
    pointer operator->() { return ptr_; }
    reference operator[](const difference_type i) const { return iterator(ptr_ + i * stride_(), slc_); }
    
    iterator& operator++() { ptr_ += stride_(); return *this; }
    iterator operator++(int) { iterator it = *this; *this += stride_(); return it; }
    iterator& operator--() { ptr_ -= stride_(); return *this; }
    iterator operator--(int) { iterator it = *this; *this -= stride_(); return it; }
    iterator operator+(size_type rhs) const { return iterator(ptr_ + rhs * stride_(), slc_); }
    iterator operator-(size_type rhs) const { return iterator(ptr_ - rhs * stride_(), slc_); }
    
    friend iterator operator+(const iterator j, const difference_type n) { return iterator(j.ptr_ + n * stride_(), j.slc_); }
    friend iterator operator+(const difference_type n, const iterator j) { return iterator(j.ptr_ + n * stride_(), j.slc_); }
    friend iterator operator-(const iterator j, const difference_type n) { return iterator(j.ptr_ - n * stride_(), j.slc_); }
    difference_type operator-(const iterator& that) const { return this->ptr_ - that.ptr_; }
    
    iterator& operator+=(const difference_type n) { return iterator(ptr_ += n * stride_(), slc_); }
    iterator& operator-=(const difference_type n) { return iterator(ptr_ -= n * stride_(), slc_); }
    
    bool operator==(const iterator& rhs) const { return ptr_ == rhs.ptr_; }
    bool operator!=(const iterator& rhs) const { return ptr_ != rhs.ptr_; }
    bool operator>(const iterator& rhs) const { return ptr_ > rhs.ptr_; }
    bool operator<(const iterator& rhs) const { return ptr_ < rhs.ptr_; }
    bool operator>=(const iterator& rhs) const { return ptr_ >= rhs.ptr_; }
    bool operator<=(const iterator& rhs) const { return ptr_ <= rhs.ptr_; }

  private:
    difference_type stride_() const { return slc_->stride_(); }

    pointer ptr_;
    const Slice* slc_;
  };

  using reverse_iterator = std::reverse_iterator<iterator>;
  constexpr iterator begin() const noexcept { return iterator(data_, this); }
  constexpr iterator end() const noexcept { return iterator(data_ + stride_() * extent_(), this); }
  constexpr reverse_iterator rbegin() const noexcept { return end(); }
  constexpr reverse_iterator rend() const noexcept { return begin(); }

  auto First(std::size_t count) const {
    return Slice<T, std::dynamic_extent, stride>(data_, count, stride_());
  }

  template <std::size_t count>
  auto First() const {
    return Slice<T, count, stride>(data_, count, stride_());
  }

  auto Last(std::size_t count) const {
    return Slice<T, std::dynamic_extent, stride>(data_ + (extent_() - count) * stride_(), count, stride_());
  }

  template <std::size_t count>
  auto Last() const {
    return Slice<T, count, stride>(data_ + (extent_() - count) * stride_(), count, stride_());
  }

  template<std::size_t count>
  auto DropFirst() const {
    return Slice<T, std::dynamic_extent, stride>(data_ + count * stride_(), extent_() - count, stride_());
  }

  auto DropFirst(std::size_t count) const {
    return Slice<T, std::dynamic_extent, stride>(data_ + count * stride_(), extent_() - count, stride_());
  }

  auto DropLast(std::size_t count) const {
    return Slice<T, std::dynamic_extent, stride>(data_, extent_() - count, stride_());
  }

  template <std::size_t count>
  auto DropLast() const {
    return Slice<T, std::dynamic_extent, stride>(data_, extent_() - count, stride_());
  }

  auto Skip(std::ptrdiff_t skip) const {
    return Slice<T, std::dynamic_extent, dynamic_stride>(data_, extentSkipped_(skip), strideSkipped_(skip));
  }

  template <std::ptrdiff_t skip>
  auto Skip() const {
    return Slice<T, skippedExtent_(skip), skippedStride_(skip)>(data_, extentSkipped_(skip), strideSkipped_(skip));
  }
  
  // operators
  Slice& operator=(const Slice&) = default;
  Slice& operator=(Slice&&) = default;

  constexpr reference operator[](size_type i) const noexcept { return *(this->data_ + i * stride_()); }
  
  bool operator==(const Slice<T, extent, stride>& that) const {
    auto a = begin();
    auto b = that.begin();
    while (*a == *b && a != end() && b != that.end()) { ++a; ++b; }
    if (a != end() || b != that.end())
      return false;
    return true;
  }
  
  bool operator!=(const Slice<T, extent, stride>& that) const { return !(*this == that); }
  
  template<class newT, std::size_t newExtent, std::ptrdiff_t newStride>
    requires (std::is_same_v<newT, T> || std::is_same_v<newT, const T>)
  operator Slice<newT, newExtent, newStride>() const {
    return Slice<newT, newExtent, newStride>(data_, extent_(), stride_());
  }

private:
  Slice(T* dt, ExtentStorage<extent> extnt, StrideStorage<stride> strd) {
    if constexpr (extent != std::dynamic_extent) { extent_() = extnt(); }
    if constexpr (stride != dynamic_stride)      { stride_() = strd(); }
  }
  
  size_type extentSkipped_(const difference_type skip) const {
    return (skip - 1 + extent_()) / skip;
  }

  difference_type strideSkipped_(const difference_type skip) const {
    return stride_() * skip;
  }

  static consteval size_type skippedExtent_(const difference_type skip) {
    return extent == std::dynamic_extent ? std::dynamic_extent : (skip - 1 + extent) / skip;
  }

  static consteval difference_type skippedStride_(const difference_type skip) {
    return stride == dynamic_stride ? dynamic_stride : stride * skip;
  }

  T* data_;
  [[no_unique_address]] ExtentStorage<extent> extent_;
  [[no_unique_address]] StrideStorage<stride> stride_;
};

// deduction guides
// special thanks to https://stackoverflow.com/questions/44350952/how-to-infer-template-parameters-from-constructors
template<class U>
Slice(U&) -> Slice<typename U::value_type>;

template<typename Type, size_t Size>
Slice(std::array<Type, Size>&) -> Slice<Type, Size>;

template <std::contiguous_iterator It>
Slice(It first, std::size_t count, std::ptrdiff_t skip) -> Slice<typename It::value_type, std::dynamic_extent, dynamic_stride>;

template<typename T, std::size_t extent, std::ptrdiff_t stride>
Slice(const Slice<T, extent, stride>&) -> Slice<T, extent, stride>;
