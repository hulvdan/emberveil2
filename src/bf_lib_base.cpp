#pragma once

#include <stdint.h>
#include <limits>
#include <utility>

#define UNUSED(x) (void)(x)

#define STATEMENT(statement) \
  do {                       \
    statement;               \
  } while (false)

#if (defined(__GNUC__) && !defined(__STRICT_ANSI__)) || defined(__cplusplus) \
  || defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L /* C99 */
#  define BF_INLINE_KEYWORD inline
#else
#  define BF_INLINE_KEYWORD
#endif

#if defined(__GNUC__) || defined(__ICCARM__)
#  define BF_FORCE_INLINE_ATTR __attribute__((always_inline))
#elif defined(_MSC_VER) && !defined(BF_LINT)
#  define BF_FORCE_INLINE_ATTR __forceinline
#else
#  define BF_FORCE_INLINE_ATTR
#endif

#define BF_FORCE_INLINE BF_INLINE_KEYWORD BF_FORCE_INLINE_ATTR

#if _WIN32 && !defined(BF_LINT)
#  define BF_FORCE_INLINE_LAMBDA noexcept [[msvc::forceinline]]
#elif defined(SDL_PLATFORM_EMSCRIPTEN)
#  define BF_FORCE_INLINE_LAMBDA
#elif __GNUC__ >= 4
#  define BF_FORCE_INLINE_LAMBDA noexcept __attribute__((always_inline))
#else
#  define BF_FORCE_INLINE_LAMBDA noexcept
#endif

#define LAMBDA(returnType_, name_, arguments) \
  const auto name_ = [&] arguments BF_FORCE_INLINE_LAMBDA -> returnType_

#define INLINE_LAMBDA [&]() BF_FORCE_INLINE_LAMBDA

#define BF_RELEASE (BF_DEBUG == 0)

#define PTR_FROM_UINT(value) ((void*)((u8*)(nullptr) + (value)))
#define UINT_FROM_PTR(value) ((uintptr_t)((u8*)(value)))

#define MEMBER(T, m) (((T*)nullptr)->m)
#define OFFSET_OF_MEMBER(T, m) UINT_FROM_PTR(&Member(T, m))

#define ARRAY_COUNT(a) (int)(sizeof(a) / sizeof((a)[0]))

#define EMPTY_STATEMENT ((void)0)

// #define CONTINUE_LABEL_DANGER(name)           \
//   /*NOLINTBEGIN(bugprone-macro-parentheses)*/ \
//   if (0) {                                    \
//   name:                                       \
//     continue;                                 \
//   }                                           \
//   /*NOLINTEND(bugprone-macro-parentheses)*/   \
//   (void)0;

using void_func = void (*)();

using uint = unsigned int;
using u8   = uint8_t;
using i8   = int8_t;
using u16  = uint16_t;
using i16  = int16_t;
using u32  = uint32_t;
using i32  = int32_t;
using u64  = uint64_t;
using i64  = int64_t;
using f32  = float;
using f64  = double;

constexpr uint   uint_max   = std::numeric_limits<uint>::max();
constexpr u8     u8_max     = std::numeric_limits<u8>::max();
constexpr u16    u16_max    = std::numeric_limits<u16>::max();
constexpr u32    u32_max    = std::numeric_limits<u32>::max();
constexpr u64    u64_max    = std::numeric_limits<u64>::max();
constexpr size_t size_t_max = std::numeric_limits<size_t>::max();

constexpr int int_max = std::numeric_limits<int>::max();
constexpr i8  i8_max  = std::numeric_limits<i8>::max();
constexpr i16 i16_max = std::numeric_limits<i16>::max();
constexpr i32 i32_max = std::numeric_limits<i32>::max();
constexpr i64 i64_max = std::numeric_limits<i64>::max();

constexpr int int_min = std::numeric_limits<int>::min();
constexpr i8  i8_min  = std::numeric_limits<i8>::min();
constexpr i16 i16_min = std::numeric_limits<i16>::min();
constexpr i32 i32_min = std::numeric_limits<i32>::min();
constexpr i64 i64_min = std::numeric_limits<i64>::min();

constexpr f32 f32_inf = std::numeric_limits<f32>::infinity();
constexpr f64 f64_inf = std::numeric_limits<f64>::infinity();

// NOLINTBEGIN(bugprone-macro-parentheses)
#define FOR_RANGE(type, variable_name, max_value_exclusive) \
  for (type variable_name = 0; (variable_name) < (max_value_exclusive); variable_name++)
// NOLINTEND(bugprone-macro-parentheses)

#define INVALID_PATH ASSERT(false)
#define NOT_IMPLEMENTED ASSERT(false)
#define NOT_SUPPORTED ASSERT(false)

#define SCAST static_cast
#define RCAST reinterpret_cast

#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#ifdef TESTS
#  define ASSERT(expr) CHECK(expr)
#  define ASSERT_FALSE(expr) CHECK_FALSE(expr)
#else  // TESTS
#  if BF_ENABLE_ASSERTS
#    include <cassert>
#    define ASSERT(expr) assert(expr)
#    define ASSERT_FALSE(expr) assert(!((bool)(expr)))
#  else
#    define ASSERT(expr) EMPTY_STATEMENT
#    define ASSERT_FALSE(expr) EMPTY_STATEMENT
#  endif  // BF_ENABLE_ASSERTS
#endif    // TESTS

#ifndef TEST_CASE
#  define h_with_counter_(counter) h_##counter
#  define h_(counter) h_with_counter_(counter)
#  define TEST_CASE(...) void h_(__COUNTER__)()
#endif

#ifndef CHECK
#  define CHECK
#endif

// #ifndef ZoneScoped
// #  define ZoneScoped
// #endif
//
// #ifndef ZoneScopedN
// #  define ZoneScopedN
// #endif
//
// #ifndef FrameMark
// #  define FrameMark
// #endif

constexpr float  floatInf  = std::numeric_limits<float>::infinity();
constexpr double doubleInf = std::numeric_limits<double>::infinity();

//----------------------------------------------------------------------------------
// Defer.
//----------------------------------------------------------------------------------
template <typename F>
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
struct Defer_ {
  explicit BF_FORCE_INLINE Defer_(F f)
      : f(f) {}
  BF_FORCE_INLINE ~Defer_() {
    f();
  }
  F f;
};

template <typename F>
BF_FORCE_INLINE Defer_<F> makeDefer_(F f) {
  return Defer_<F>(f);
}

#define defer_with_counter_(counter) defer_##counter
#define defer_(counter) defer_with_counter_(counter)

struct defer_dummy_ {};

template <typename F>
// NOLINTNEXTLINE(cppcoreguidelines-missing-std-forward)
Defer_<F> operator+(defer_dummy_, F&& f) {
  return makeDefer_<F>(std::forward<F>(f));
}

// Usage:
//     {
//         DEFER { printf("Deferred\n"); };
//         printf("Normal\n");
//     }
//
// Output:
//     Normal
//     Deferred
//
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define DEFER auto defer_(__COUNTER__) = defer_dummy_() + [&]() BF_FORCE_INLINE_LAMBDA

//----------------------------------------------------------------------------------
// Iterators.
//----------------------------------------------------------------------------------
//
// NOTE: Proudly taken from
// https://vector-of-bool.github.io/2020/06/13/cpp20-iter-facade.html
template <class Reference>
struct ArrowProxy {
  Reference  r;
  Reference* operator->() {
    return &r;
  }
};

// Usage:
//
//     struct A : public Equatable<A> {
//         int field1;
//         int field2;
//
//         NOTE: Требуется реализовать этот метод.
//         [[nodiscard]] bool EqualTo(const A& other) const {
//             auto result = (
//                 field1 == other.field1
//                 && field2 == other.field2
//             );
//             return result;
//         }
//     }
//
template <typename Derived>
struct Equatable {
  private:
  using SelfType = Derived;

  public:
  friend bool operator==(const SelfType& v1, const SelfType& v2) {
    return v1.EqualTo(v2);
  }
  // SHIT: Fuken `clang-tidy` requires this function to be specified
  friend bool operator!=(const SelfType& v1, const SelfType& v2) {
    return !v1.EqualTo(v2);
  }
};

template <typename Derived>
struct IteratorFacade {
  protected:
  using SelfType = Derived;

  private:
  SelfType& _self() {
    return SCAST<SelfType&>(*this);
  }
  [[nodiscard]] const SelfType& _self() const {
    return SCAST<const SelfType&>(*this);
  }

  public:
  decltype(auto) operator*() const {
    return _self().Dereference();
  }

  auto operator->() const {
    decltype(auto) ref = **this;
    if constexpr (std::is_reference_v<decltype(ref)>)
      return std::addressof(ref);
    else
      return ArrowProxy<Derived>(std::move(ref));
  }

  friend bool operator==(const SelfType& v1, const SelfType& v2) {
    return v1.EqualTo(v2);
  }
  // SHIT: Fuken `clang-tidy` requires this function to be specified
  friend bool operator!=(const SelfType& v1, const SelfType& v2) {
    return !v1.EqualTo(v2);
  }

  SelfType& operator++() {
    _self().Increment();
    return _self();
  }

  SelfType operator++(int) {
    auto copy = _self();
    ++*this;
    return copy;
  }
};
