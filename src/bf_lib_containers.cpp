// cppcheck-suppress-file unusedFunction

#pragma once

#define ARRAY_PUSH(array, arrayCount, arrayMaxCount, value) \
  STATEMENT({                                               \
    *((array) + (arrayCount)) = value;                      \
    (arrayCount)++;                                         \
    ASSERT((arrayCount) <= (arrayMaxCount));                \
  })

template <typename T>
T ARRAY_POP(T* array, auto* arrayCount) {
  ASSERT(*arrayCount > 0);
  T result    = *(array + *arrayCount - 1);
  *arrayCount = *arrayCount - 1;
  return result;
}

#define ARRAY_REVERSE(array, count)     \
  STATEMENT({                           \
    ASSERT((count) >= 0);               \
    FOR_RANGE (int, l, (count) / 2) {   \
      auto r         = (count) - l - 1; \
      auto t         = *((array) + l);  \
      *((array) + l) = *((array) + r);  \
      *((array) + r) = t;               \
    }                                   \
  })

template <typename T>
int ArrayFind(const T* base, int count, const T& value) {
  FOR_RANGE (int, i, count) {
    if (base[i] == value)
      return i;
  }
  return -1;
}

template <typename T>
int ArrayBinaryFind(const T* base, int count, const T& value) {
  int low  = 0;
  int high = count - 1;
  while (low <= high) {
    int mid = low + (high - low) / 2;

    if (base[mid] == value)
      return mid;

    if (base[mid] < value)
      low = mid + 1;
    else
      high = mid - 1;
  }

  return -1;
}

template <typename T>
BF_FORCE_INLINE bool ArrayContains(const T* base, int count, const T& value) {
  return ArrayFind(base, count, value) != -1;
}

TEST_CASE ("ArrayFind, ArrayContains") {
  {
    int  values[] = {0, 1, 2, 3, 4, 5};
    auto c        = ARRAY_COUNT(values);
    ASSERT(ArrayFind(values, c, 5) == 5);
    ASSERT(ArrayContains(values, c, 5));
    ASSERT_FALSE(ArrayContains(values, c, 6));
  }
  {
    int  values[] = {1, 2, 3, 4, 5};
    auto c        = ARRAY_COUNT(values);
    ASSERT(ArrayFind(values, c, 5) == 4);
    ASSERT(ArrayContains(values, c, 5));
    ASSERT_FALSE(ArrayContains(values, c, 6));
  }
  {
    int  values[] = {1, 2, 3, 4, 5};
    auto c        = ARRAY_COUNT(values);
    ASSERT(ArrayFind(values, c, 6) == -1);
    ASSERT(ArrayContains(values, c, 5));
    ASSERT_FALSE(ArrayContains(values, c, 6));
  }
}

void ArraySort_(
  void*                               buffer,
  int                                 elementSize,
  int                                 n,
  std::invocable<void*, void*> auto&& cmp,
  std::invocable<int, int> auto&&     callback,
  void*                               swapBuf
) {
  ASSERT(elementSize > 0);

  FOR_RANGE (int, i, n - 1) {
    auto swapped = false;

    FOR_RANGE (int, j, n - i - 1) {
      auto v1 = (u8*)buffer + (j + 0) * elementSize;
      auto v2 = (u8*)buffer + (j + 1) * elementSize;

      if (cmp((void*)v1, (void*)v2) == 1) {
        memcpy(swapBuf, v1, elementSize);
        memcpy(v1, v2, elementSize);
        memcpy(v2, swapBuf, elementSize);

        callback(j, j + 1);

        swapped = true;
      }
    }

    if (!swapped)
      break;
  }
}

#define ARRAY_SORT(ptr, n, cmp, callback)                                             \
  STATEMENT({                                                                         \
    u8 swapBuf[sizeof(*(ptr))];                                                       \
    ArraySort_((void*)(ptr), sizeof(*(ptr)), (n), (cmp), (callback), (void*)swapBuf); \
  })

int IntCmp(const int* v1, const int* v2) {
  if (*v1 > *v2)
    return 1;
  if (*v1 < *v2)
    return -1;
  return 0;
}

template <typename T>
bool Contains(const std::vector<T>& v, T x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

//----------------------------------------------------------------------------------
// Контейнеры.
//----------------------------------------------------------------------------------

template <typename T>
struct View {
  int count = 0;
  T*  base  = nullptr;

  T& operator[](int index) const {
    ASSERT(base != nullptr);
    ASSERT(index >= 0);
    ASSERT(index < count);
    return base[index];
  }

  void Init(int count_) {
    ASSERT(count == 0);
    ASSERT(base == nullptr);

    count = count_;
    base  = (T*)malloc(sizeof(T) * count);
  }

  void Deinit() {
    if (base) {
      ASSERT(count > 0);
      free(base);
      base = nullptr;
    }
    else {
      ASSERT(count == 0);
    }

    count = 0;
  }

  void Zeroify() {
    ASSERT(base != nullptr);
    ASSERT(count > 0);
    memset(base, 0, sizeof(T) * count);
  }

  int IndexOf(const T& value) const {
    FOR_RANGE (int, i, count) {
      auto& v = base[i];
      if (v == value)
        return i;
    }

    return -1;
  }

  bool Contains(const T& value) const {
    return IndexOf(value) != -1;
  }

  T* begin() {
    return base;
  }

  T* end() {
    return base + count;
  }
};

#define VIEW_FROM_ARRAY_DANGER(name)                          \
  View<std::remove_reference_t<decltype(*(name##_))>>(name) { \
    .count = ARRAY_COUNT(name##_), .base = (name##_),         \
  }

template <typename T, int _count>
struct Array {
  T                base[_count] = {};
  static const int count        = _count;

  T& operator[](int index) {
    ASSERT(index >= 0);
    ASSERT(index < _count);
    return base[index];
  }

  void Zeroify() {
    ASSERT(_count > 0);
    memset(base, 0, sizeof(T) * _count);
  }

  int IndexOf(const T& value) const {
    FOR_RANGE (int, i, _count) {
      auto& v = base[i];
      if (v == value)
        return i;
    }

    return -1;
  }

  bool Contains(const T& value) const {
    return IndexOf(value) != -1;
  }

  T* begin() {
    return base;
  }

  T* end() {
    return base + _count;
  }

  View<T> ToView() const {
    return {
      .count = _count,
      .base  = (T*)base,
    };
  }
};

template <typename T>
struct Vector {
  T*  base     = nullptr;
  int count    = 0;
  u32 maxCount = 0;

  T& operator[](int index) {
    ASSERT(base != nullptr);
    ASSERT(index >= 0);
    ASSERT(index < count);
    return base[index];
  }

  int IndexOf(const T& value) const {
    FOR_RANGE (int, i, count) {
      auto& v = *(base + i);
      if (v == value)
        return i;
    }

    return -1;
  }

  bool Contains(const T& value) const {
    return IndexOf(value) != -1;
  }

  T* Add() {
    if (base == nullptr) {
      ASSERT(maxCount == 0);
      ASSERT(count == 0);

      maxCount = 8;
      base     = (T*)malloc(sizeof(T) * maxCount);
    }
    else if (maxCount == count) {
      u32 newMaxCount = maxCount * 2;
      ASSERT(maxCount < newMaxCount);  // NOTE: Ловим overflow

      auto oldSize = sizeof(T) * maxCount;
      auto oldPtr  = base;

      base = (T*)malloc(oldSize * 2);
      memcpy((void*)base, (void*)oldPtr, oldSize);
      free(oldPtr);

      maxCount = newMaxCount;
    }

    auto result = base + count;
    count += 1;

    return result;
  }

  void RemoveAt(const int i) {
    ASSERT(i >= 0);
    ASSERT(i < count);

    auto moveFromRightCount = count - i - 1;
    ASSERT(moveFromRightCount >= 0);

    if (moveFromRightCount > 0) {
      memmove((void*)(base + i), (void*)(base + i + 1), sizeof(T) * moveFromRightCount);
    }

    count--;
  }

  void UnstableRemoveAt(const int i) {
    ASSERT(i >= 0);
    ASSERT(i < count);

    if (i != count - 1)
      base[i] = base[count - 1];

    count--;
  }

  void UnstableRemoveUniqueAssert(T value) {
#if BF_ENABLE_ASSERTS
    int found = 0;
    for (auto& v : *this) {
      if (v == value)
        found++;
    }
    ASSERT(found == 1);
#endif

    int i = 0;
    for (auto& v : *this) {
      if (v == value) {
        UnstableRemoveAt(i);
        break;
      }
      i++;
    }
  }

  // Вектор сможет содержать как минимум столько элементов без реаллокации.
  void Reserve(u32 elementsCount) {
    if (base == nullptr) {
      base     = (T*)malloc(sizeof(T) * elementsCount);
      maxCount = elementsCount;
      return;
    }

    if (maxCount < elementsCount) {
      // TODO test realloc
      base     = (T*)realloc(base, sizeof(T) * elementsCount);
      maxCount = elementsCount;
    }
  }

  void Reset() {
    count = 0;
  }

  void Deinit() {
    if (base) {
      ASSERT(maxCount > 0);
      free(base);
      base = nullptr;
    }
    else {
      ASSERT(maxCount == 0);
      ASSERT(count == 0);
    }

    count    = 0;
    maxCount = 0;
  }

  T* begin() {
    return base;
  }

  T* end() {
    return base + count;
  }
};

template <typename T>
struct VectorIterator : public IteratorFacade<VectorIterator<T>> {
  VectorIterator() = delete;
  explicit VectorIterator(Vector<T>* container)
      : VectorIterator(container, 0) {}
  VectorIterator(Vector<T>* container, int current)
      : _container(container)
      , _current(current)  //
  {
    ASSERT(container != nullptr);
  }

  [[nodiscard]] VectorIterator begin() const {
    return {_container, _current};
  }
  [[nodiscard]] VectorIterator end() const {
    return {_container, _container->count};
  }

  [[nodiscard]] T* Dereference() const {
    ASSERT(_current >= 0);
    ASSERT(_current < _container->count);
    return _container->base + _current;
  }

  void Increment() {
    _current++;
  }

  [[nodiscard]] bool EqualTo(const VectorIterator& o) const {
    return _current == o._current;
  }

  private:
  Vector<T>* _container;
  int        _current = 0;
};

template <typename T>
auto Iter(Vector<T>* container) {
  return VectorIterator(container);
}

///
