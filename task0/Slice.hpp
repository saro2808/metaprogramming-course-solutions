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
class Slice;

template<typename T, std::size_t extent, std::ptrdiff_t stride>
std::ptrdiff_t* skip = &Slice<T, extent, stride>::iterator::skip;

template
  < class T
  , std::size_t extent
  , std::ptrdiff_t stride
  >
class Slice {
public:
  using value_type = std::remove_cv_t<T>;
  using element_type = T;
  using size_type = std::size_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = element_type&;
  using const_reference = const element_type&;
  using difference_type = std::ptrdiff_t;

  ////////////////////
  /// constructors ///
  ////////////////////

  Slice() = default;
  Slice(const Slice&) = default;
  explicit Slice(Slice&&) = default;

  Slice(T* dt, ExtentStorage<extent> extnt, StrideStorage<stride> strd)
    requires (!std::is_same_v<T, std::remove_const_t<T>>) {
    data_ = const_cast<const T*>(dt);
    if (extent != std::dynamic_extent) {
      extent_() = extnt();
    }
    if (stride != dynamic_stride) {
      stride_() = strd();
    }
  }

  Slice(T* dt) requires (
    !std::is_same_v<T, std::remove_const_t<T>>
    && extent == std::dynamic_extent && stride == dynamic_stride
  ) {
    data_ = const_cast<const T*>(dt);
  }

  template<typename Type, size_t Size>
    requires (std::is_same_v<T, Type>)
  constexpr Slice(std::array<Type, Size>& array) {
    if constexpr (stride == dynamic_stride) {
      stride_() = 1;
    }
    if constexpr (extent == std::dynamic_extent) {
      extent_() = array.size() / stride_();
    }
    data_ = array.data();
    *::skip<T, extent, stride> = stride_();
  }

  template<class U>
  constexpr Slice(U& container)
    requires(
      (std::is_same_v<typename U::value_type, T> ||
      std::is_same_v<typename U::value_type, std::remove_const_t<T>>) &&
      extent == std::dynamic_extent &&
      requires() { container.size(); container.data(); }
    ) {
    if constexpr (stride == dynamic_stride) {
      stride_() = 1;
    }
    extent_() = container.size() / stride_();
    data_ = container.data();
    *::skip<T, extent, stride> = stride_();
  }

  template <std::contiguous_iterator It>
  Slice(It first, std::size_t count, std::ptrdiff_t skip) {
    *::skip<T, std::dynamic_extent, stride> = skip;
    if constexpr (stride == dynamic_stride) {
      stride_() = skip;
    } // else assert(stride_() == skip);
    if constexpr (extent == std::dynamic_extent) {
      extent_() = count;
    }
    data_ = &*first;
  }

  ~Slice() noexcept = default;

  ///////////////
  /// getters ///
  ///////////////

  constexpr pointer Data() const noexcept { return data_; }
  constexpr size_type Size() const noexcept { return extent_(); }
  constexpr difference_type Stride() const noexcept { return stride_(); }

  ////////////////
  /// iterator ///
  ////////////////

  class iterator: public std::random_access_iterator_tag {
  public:
    static std::ptrdiff_t skip;
    using value_type = Slice::value_type;
    using difference_type = Slice::difference_type;
    using iterator_category = std::random_access_iterator_tag;
    using pointer = Slice::pointer;
    using element_type = Slice::element_type;
    using size_type = Slice::size_type;
    using const_pointer = Slice::const_pointer;
    using reference = Slice::reference;
    using const_reference = Slice::const_reference;
    iterator() = default;
    iterator(pointer p) : ptr_(p) {}
    ~iterator() = default;
    reference operator*() const { return *ptr_; }
    pointer operator->() { return ptr_; }
    reference operator[](const difference_type i) const { return iterator(ptr_ + i*(*::skip<T, extent, stride>)); }
    iterator& operator++() { ptr_ += *::skip<T, extent, stride>; return *this; }
    iterator operator++(int) { iterator it = *this; (*this) += *::skip<T, extent, stride>; return it; }
    iterator& operator--() { ptr_ -= *::skip<T, extent, stride>; return *this; }
    iterator operator--(int) { iterator it = *this; (*this) -= *::skip<T, extent, stride>; return it; }
    iterator operator+(size_type rhs) const { return iterator(ptr_ + rhs * *::skip<T, extent, stride>); }
    iterator operator-(size_type rhs) const { return iterator(ptr_ - rhs * *::skip<T, extent, stride>); }
    friend iterator operator+(const iterator j, const difference_type n) { return iterator(j.ptr_ + n*(*::skip<T, extent, stride>)); }
    friend iterator operator+(const difference_type n, const iterator j) { return iterator(j.ptr_ + n*(*::skip<T, extent, stride>)); }
    friend iterator operator-(const iterator j, const difference_type n) { return iterator(j.ptr_ - n*(*::skip<T, extent, stride>)); }
    difference_type operator-(const iterator& that) const { return this->ptr_ - that.ptr_; }
    //friend difference_type operator-(const iterator& a, const iterator& b) { return a.ptr_ - b.ptr_; }
    iterator& operator+=(const difference_type n) { return iterator(ptr_ += n*(*::skip<T, extent, stride>)); }
    iterator& operator-=(const difference_type n) { return iterator(ptr_ -= n*(*::skip<T, extent, stride>)); }
    inline bool operator==(const iterator& rhs) const { return ptr_ == rhs.ptr_; }
    inline bool operator!=(const iterator& rhs) const { return ptr_ != rhs.ptr_; }
    inline bool operator>(const iterator& rhs) const { return ptr_ > rhs.ptr_; }
    inline bool operator<(const iterator& rhs) const { return ptr_ < rhs.ptr_; }
    inline bool operator>=(const iterator& rhs) const { return ptr_ >= rhs.ptr_; }
    inline bool operator<=(const iterator& rhs) const { return ptr_ <= rhs.ptr_; }
  private:
    pointer ptr_;
  };
  using reverse_iterator = std::reverse_iterator<iterator>;
  constexpr iterator begin() const noexcept { return data_; }
  constexpr iterator end() const noexcept { return data_ + stride_() * extent_(); }
  constexpr reverse_iterator rbegin() const noexcept { return end(); }
  constexpr reverse_iterator rend() const noexcept { return begin(); }

  //////////////////////////////////////////////
  /// first, last, dropfirst, droplast, skip ///
  //////////////////////////////////////////////

  Slice<T, std::dynamic_extent, stride>
    First(std::size_t count) const {
      return Slice<T, std::dynamic_extent, stride>(data_, count, stride_());
    }

  template <std::size_t count>
  Slice<T, count, stride>
    First() const {
      return Slice<T, count, stride>(data_, count, stride_());
    }

  Slice<T, std::dynamic_extent, stride>
    Last(std::size_t count) const {
      return Slice<T, std::dynamic_extent, stride>(data_ + (extent_() - count) * stride_(), count, stride_());
    }

  template <std::size_t count>
  Slice<T, count, stride>
    Last() const {
      return Slice<T, count, stride>(data_ + (extent_() - count) * stride_(), count, stride_());
    }

  template<std::size_t count>
  Slice<T, std::dynamic_extent, stride>
    DropFirst() const {
      return Slice<T, std::dynamic_extent, stride>(data_ + count * stride_(), extent_() - count, stride_());
    }

  Slice<T, std::dynamic_extent, stride>
    DropFirst(std::size_t count) const {
      return Slice<T, std::dynamic_extent, stride>(data_ + count * stride_(), extent_() - count, stride_());
    }

  Slice<T, std::dynamic_extent, stride>
    DropLast(std::size_t count) const {
      return Slice<T, std::dynamic_extent, stride>(data_, extent_() - count, stride_());
    }

  template <std::size_t count>
  Slice<T, std::dynamic_extent, stride>
    DropLast() const {
      return Slice<T, std::dynamic_extent, stride>(data_, extent_() - count, stride_());
    }

  Slice<T, std::dynamic_extent, dynamic_stride>
    Skip(std::ptrdiff_t skip) const {
      *::skip<T, std::dynamic_extent, dynamic_stride> = skip * stride_();
      return Slice<T, std::dynamic_extent, dynamic_stride>(data_, (skip - 1 + extent_()) / skip, skip * stride_());
    }

  template <std::ptrdiff_t skip>
  Slice<T, (extent - 1 + skip) / skip, stride == dynamic_stride ? dynamic_stride : stride * skip>
    Skip() const
    requires (extent != std::dynamic_extent) {
      std::size_t extnt = (skip - 1 + extent_()) / skip;
      std::ptrdiff_t strd = skip * stride_();
      *::skip<T, (extent - 1 + skip) / skip, stride == dynamic_stride ? dynamic_stride : stride * skip> = strd;
      return Slice<T, (extent - 1 + skip) / skip, stride == dynamic_stride ? dynamic_stride : stride * skip>(data_, extnt, strd);
    }

  template <std::ptrdiff_t skip>
  Slice<T, std::dynamic_extent, stride == dynamic_stride ? dynamic_stride : stride * skip>
    Skip() const {
      std::size_t extnt = (skip - 1 + extent_()) / skip;
      std::ptrdiff_t strd = skip * stride_();
      *::skip<T, std::dynamic_extent, stride == dynamic_stride ? dynamic_stride : stride * skip> = strd;
      return Slice<T, std::dynamic_extent, stride == dynamic_stride ? dynamic_stride : stride * skip>(data_, extnt, strd);
    }

  /////////////////
  /// operators ///
  /////////////////

  Slice& operator=(const Slice&) = default;
  Slice& operator=(Slice&&) = default;

  constexpr reference operator[](size_type i) const noexcept { return *(this->data_ + i * *::skip<T, extent, stride>); }
  
  bool operator==(const Slice<T, extent, stride>& that) const {
      auto a = begin();
      auto b = that.begin();
      while (*a == *b && a != end() && b != that.end()) {
        ++a;
        ++b;
      }
      if (a != end() || b != that.end()) {
        return false;
      }
      return true;
    }
  
  bool operator!=(const Slice<T, extent, stride>& that) const { return !(*this == that); }
  
  /////////////
  /// casts ///
  /////////////

  operator Slice<T, std::dynamic_extent, dynamic_stride>() const
    requires (stride != dynamic_stride && extent != std::dynamic_extent) {
    return Slice<T, std::dynamic_extent, dynamic_stride>(
      this->data_, this->extent_(), this->stride_()
    );
  }

  operator Slice<T, std::dynamic_extent, stride>() const
    requires (extent != std::dynamic_extent) {
    return Slice<T, std::dynamic_extent, stride>(
      this->data_, this->extent_(), this->stride_()
    );
  }

  operator Slice<T, extent, dynamic_stride>() const
    requires (stride != dynamic_stride) {
    return Slice<T, extent, dynamic_stride>(
      this->data_, this->extent_(), this->stride_()
    );
  }

  operator Slice<const T, extent, stride>() const
    requires (std::is_same_v<T, std::remove_const_t<T>>
    && extent != std::dynamic_extent && stride != dynamic_stride) {
    return Slice<const T, extent, stride>(
      const_cast<const T*>(this->data_),
      this->extent_(),
      this->stride_()
    );
  }

  operator Slice<const T, std::dynamic_extent, stride>() const
    requires (std::is_same_v<T, std::remove_const_t<T>> && stride != dynamic_stride) {
    return Slice<const T, std::dynamic_extent, stride>(
      const_cast<const T*>(this->data_),
      this->extent_(),
      this->stride_()
    );
  }

  operator Slice<const T, extent, dynamic_stride>() const
    requires (std::is_same_v<T, std::remove_const_t<T>> && extent != std::dynamic_extent) {
    return Slice<const T, extent, dynamic_stride>(
      const_cast<const T*>(this->data_),
      this->extent_(),
      this->stride_()
    );
  }

  operator Slice<const T, std::dynamic_extent, dynamic_stride>() const
    requires std::is_same_v<T, std::remove_const_t<T>> {
    return Slice<const T, std::dynamic_extent, dynamic_stride>(
      const_cast<const T*>(this->data_),
      this->extent_(),
      this->stride_()
    );
  }

private:
  T* data_;
  [[no_unique_address]] ExtentStorage<extent> extent_;
  [[no_unique_address]] StrideStorage<stride> stride_;
};

template<typename T, std::size_t extent, std::ptrdiff_t stride>
std::ptrdiff_t Slice<T, extent, stride>::iterator::skip = 1;

// deduction guide
// special thanks to https://stackoverflow.com/questions/44350952/how-to-infer-template-parameters-from-constructors
template<class U>
Slice(U&) -> Slice<typename U::value_type>;

template<typename Type, size_t Size>
Slice(std::array<Type, Size>&) -> Slice<Type, Size>;

template <std::contiguous_iterator It>
Slice(It first, std::size_t count, std::ptrdiff_t skip) -> Slice<typename It::value_type>;

template<typename T, std::size_t extent, std::ptrdiff_t stride>
Slice(const Slice<T, extent, stride>&) -> Slice<T, extent, stride>;