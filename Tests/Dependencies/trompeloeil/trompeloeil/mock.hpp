/*
 * Trompeloeil C++ mocking framework
 *
 * Copyright (C) Björn Fahller
 * Copyright (C) 2017, 2018 Andrew Paxie
 * Copyright Tore Martin Hagen 2019
 *
 *  Use, modification and distribution is subject to the
 *  Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy atl
 *  http://www.boost.org/LICENSE_1_0.txt)
 *
 * Project home: https://github.com/rollbear/trompeloeil
 */


#ifndef TROMPELOEIL_MOCK_HPP_
#define TROMPELOEIL_MOCK_HPP_


// trompe l'oeil noun    (Concise Encyclopedia)
// Style of representation in which a painted object is intended
// to deceive the viewer into believing it is the object itself...

// project home: https://github.com/rollbear/trompeloeil


// Deficiencies and missing features
// * Mocking function templates is not supported
// * If a macro kills a kitten, this threatens extinction of all felines!


#if defined(_MSC_VER)

#  define TROMPELOEIL_NORETURN __declspec(noreturn)

#  if (!defined(__cplusplus) || _MSC_VER < 1900)
#    error requires C++ in Visual Studio 2015 RC or later
#  endif

#else

#  define TROMPELOEIL_NORETURN [[noreturn]]

#  if (!defined(__cplusplus) || __cplusplus < 201103L)
#    error requires C++11 or higher
#  endif

#endif

#if defined(__clang__)

#  define TROMPELOEIL_CLANG 1
#  define TROMPELOEIL_GCC 0
#  define TROMPELOEIL_MSVC 0

#  define TROMPELOEIL_CLANG_VERSION \
  (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)

#  define TROMPELOEIL_GCC_VERSION 0

#  define TROMPELOEIL_CPLUSPLUS __cplusplus

#  define TROMPELOEIL_NOT_IMPLEMENTED(...)                         \
  _Pragma("clang diagnostic push")                                   \
  _Pragma("clang diagnostic ignored \"-Wunneeded-member-function\"") \
  __VA_ARGS__                                                        \
  _Pragma("clang diagnostic pop")

#elif defined(__GNUC__)

#  define TROMPELOEIL_CLANG 0
#  define TROMPELOEIL_GCC 1
#  define TROMPELOEIL_MSVC 0

#  define TROMPELOEIL_CLANG_VERSION 0
#  define TROMPELOEIL_GCC_VERSION \
  (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#  define TROMPELOEIL_CPLUSPLUS __cplusplus

#elif defined(_MSC_VER)

#  define TROMPELOEIL_CLANG 0
#  define TROMPELOEIL_GCC 0
#  define TROMPELOEIL_MSVC 1

#  define TROMPELOEIL_CLANG_VERSION 0
#  define TROMPELOEIL_GCC_VERSION 0

#  if defined(_MSVC_LANG)

    // Compiler is at least Microsoft Visual Studio 2015 Update 3.
#    define TROMPELOEIL_CPLUSPLUS _MSVC_LANG

#  else /* defined(_MSVC_LANG) */

    /*
     * This version of Microsoft Visual C++ is released
     * in a version of Microsoft Visual Studio between
     * 2015 RC and less than 2015 Update 3.
     *
     * It is an amalgam of C++ versions, with no provision
     * to specify C++11 mode.
     *
     * It also has a __cplusplus macro stuck at 199711L with
     * no way to change it, such as /Zc:__cplusplus.
     *
     * Assume the C++14 code path, but don't promise that it is a
     * fully conforming implementation of C++14 either.
     * Hence a value of 201401L, which less than 201402L,
     * the standards conforming value of __cplusplus.
     */
#    define TROMPELOEIL_CPLUSPLUS 201401L

#  endif /* !defined(_MSVC_LANG) */

#endif


#ifndef TROMPELOEIL_NOT_IMPLEMENTED
#define TROMPELOEIL_NOT_IMPLEMENTED(...) __VA_ARGS__
#endif

#include <algorithm>
#include <tuple>
#include <iomanip>
#include <sstream>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <array>
#include <initializer_list>
#include <type_traits>
#include <utility>

#ifdef __cpp_impl_coroutine
#include <coroutine>
#endif


#if TROMPELOEIL_CPLUSPLUS >= 202211L
#  if __has_include(<expected>)
#    include <expected>
#  endif
#  if defined(__cpp_lib_expected)
#    define   TROMPELOEIL_HAS_EXPECTED 1
#  else
#    define TROMPELOEIL_HAS_EXPECTED 0
#  endif
#else
#define TROMPELOEIL_HAS_EXPECTED 0
#endif
#ifndef TROMPELOEIL_CUSTOM_ATOMIC
#include <atomic>
namespace trompeloeil { using std::atomic; }
#else
#include <trompeloeil/custom_atomic.hpp>
#endif

#ifndef TROMPELOEIL_CUSTOM_UNIQUE_LOCK
namespace trompeloeil { using std::unique_lock; }
#else
#include <trompeloeil/custom_unique_lock.hpp>
#endif


#ifdef TROMPELOEIL_SANITY_CHECKS
#include <cassert>
#define TROMPELOEIL_ASSERT(x) assert(x)
#else
#define TROMPELOEIL_ASSERT(x) do {} while (false)
#endif

#define TROMPELOEIL_IDENTITY(...) __VA_ARGS__ // work around stupid MS VS2015 RC bug

#define TROMPELOEIL_ARG16(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15, ...) _15

#define TROMPELOEIL_COUNT(...)                                                 \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_ARG16(__VA_ARGS__,                          \
                                         15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0))

#if TROMPELOEIL_MSVC

#define TROMPELOEIL_CONCAT_(x, y, ...) x ## y __VA_ARGS__
#define TROMPELOEIL_CONCAT(x, ...) TROMPELOEIL_CONCAT_(x, __VA_ARGS__)

#else /* TROMPELOEIL_MSVC */

#define TROMPELOEIL_CONCAT_(x, ...) x ## __VA_ARGS__
#define TROMPELOEIL_CONCAT(x, ...) TROMPELOEIL_CONCAT_(x, __VA_ARGS__)

#endif /* !TROMPELOEIL_MSVC */

#define TROMPELOEIL_SEPARATE1(p1) p1
#define TROMPELOEIL_SEPARATE2(p1,p2) p1 p2
#define TROMPELOEIL_SEPARATE3(p1,...) p1 TROMPELOEIL_SEPARATE2(__VA_ARGS__)
#define TROMPELOEIL_SEPARATE4(p1,...) p1 TROMPELOEIL_SEPARATE3(__VA_ARGS__)
#define TROMPELOEIL_SEPARATE5(p1,...) p1 TROMPELOEIL_SEPARATE4(__VA_ARGS__)
#define TROMPELOEIL_SEPARATE6(p1,...) p1 TROMPELOEIL_SEPARATE5(__VA_ARGS__)
#define TROMPELOEIL_SEPARATE7(p1,...) p1 TROMPELOEIL_SEPARATE6(__VA_ARGS__)
#define TROMPELOEIL_SEPARATE8(p1,...) p1 TROMPELOEIL_SEPARATE7(__VA_ARGS__)
#define TROMPELOEIL_SEPARATE9(p1,...) p1 TROMPELOEIL_SEPARATE8(__VA_ARGS__)
#define TROMPELOEIL_SEPARATE(...) \
  TROMPELOEIL_CONCAT(TROMPELOEIL_SEPARATE,\
                     TROMPELOEIL_COUNT(__VA_ARGS__))(__VA_ARGS__)


#define TROMPELOEIL_REMOVE_PAREN(...) TROMPELOEIL_CONCAT(TROMPELOEIL_CLEAR_,   \
  TROMPELOEIL_REMOVE_PAREN_INTERNAL __VA_ARGS__)
#define TROMPELOEIL_REMOVE_PAREN_INTERNAL(...)                                 \
  TROMPELOEIL_REMOVE_PAREN_INTERNAL __VA_ARGS__
#define TROMPELOEIL_CLEAR_TROMPELOEIL_REMOVE_PAREN_INTERNAL

#define TROMPELOEIL_EXPAND(x) x

#define TROMPELOEIL_INIT_WITH_STR15(base, x, ...)                              \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR14(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR14(base, x, ...)                              \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR13(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR13(base, x, ...)                              \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR12(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR12(base, x, ...)                              \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR11(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR11(base, x, ...)                              \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR10(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR10(base, x, ...)                              \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR9(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR9(base, x, ...)                               \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR8(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR8(base, x, ...)                               \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR7(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR7(base, x, ...)                               \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR6(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR6(base, x, ...)                               \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR5(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR5(base, x, ...)                               \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR4(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR4(base, x, ...)                               \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR3(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR3(base, x, ...)                               \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR2(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR2(base, x, ...)                               \
  base{#x, x}, TROMPELOEIL_EXPAND(TROMPELOEIL_INIT_WITH_STR1(base, __VA_ARGS__))

#define TROMPELOEIL_INIT_WITH_STR1(base, x)                                    \
  base{#x, x}

#define TROMPELOEIL_INIT_WITH_STR0(base)

#define TROMPELOEIL_INIT_WITH_STR(base, ...)                                   \
  TROMPELOEIL_CONCAT(TROMPELOEIL_INIT_WITH_STR,                                \
                     TROMPELOEIL_COUNT(__VA_ARGS__))(base, __VA_ARGS__)

#define TROMPELOEIL_PARAM_LIST15(...)                                          \
  TROMPELOEIL_PARAM_LIST14(__VA_ARGS__),                                       \
  ::trompeloeil::param_list_t<__VA_ARGS__, 14> p15

#define TROMPELOEIL_PARAM_LIST14(...)                                          \
  TROMPELOEIL_PARAM_LIST13(__VA_ARGS__),                                       \
  ::trompeloeil::param_list_t<__VA_ARGS__, 13> p14

#define TROMPELOEIL_PARAM_LIST13(...)                                          \
  TROMPELOEIL_PARAM_LIST12(__VA_ARGS__),                                       \
  ::trompeloeil::param_list_t<__VA_ARGS__, 12> p13

#define TROMPELOEIL_PARAM_LIST12(...)                                          \
  TROMPELOEIL_PARAM_LIST11(__VA_ARGS__),                                       \
  ::trompeloeil::param_list_t<__VA_ARGS__, 11> p12

#define TROMPELOEIL_PARAM_LIST11(...)                                          \
  TROMPELOEIL_PARAM_LIST10(__VA_ARGS__),                                       \
  ::trompeloeil::param_list_t<__VA_ARGS__, 10> p11

#define TROMPELOEIL_PARAM_LIST10(...)                                          \
  TROMPELOEIL_PARAM_LIST9(__VA_ARGS__),                                        \
  ::trompeloeil::param_list_t<__VA_ARGS__, 9> p10

#define TROMPELOEIL_PARAM_LIST9(...)                                           \
  TROMPELOEIL_PARAM_LIST8(__VA_ARGS__),                                        \
  ::trompeloeil::param_list_t<__VA_ARGS__, 8> p9

#define TROMPELOEIL_PARAM_LIST8(...)                                           \
  TROMPELOEIL_PARAM_LIST7(__VA_ARGS__),                                        \
  ::trompeloeil::param_list_t<__VA_ARGS__, 7> p8

#define TROMPELOEIL_PARAM_LIST7(...)                                           \
  TROMPELOEIL_PARAM_LIST6(__VA_ARGS__),                                        \
  ::trompeloeil::param_list_t<__VA_ARGS__, 6> p7

#define TROMPELOEIL_PARAM_LIST6(...)                                           \
  TROMPELOEIL_PARAM_LIST5(__VA_ARGS__),                                        \
  ::trompeloeil::param_list_t<__VA_ARGS__, 5> p6

#define TROMPELOEIL_PARAM_LIST5(...)                                           \
  TROMPELOEIL_PARAM_LIST4(__VA_ARGS__),                                        \
    ::trompeloeil::param_list_t<__VA_ARGS__, 4> p5

#define TROMPELOEIL_PARAM_LIST4(...)                                           \
  TROMPELOEIL_PARAM_LIST3(__VA_ARGS__),                                        \
    ::trompeloeil::param_list_t<__VA_ARGS__, 3> p4

#define TROMPELOEIL_PARAM_LIST3(...)                                           \
  TROMPELOEIL_PARAM_LIST2(__VA_ARGS__),                                        \
  ::trompeloeil::param_list_t<__VA_ARGS__, 2> p3

#define TROMPELOEIL_PARAM_LIST2(...)                                           \
  TROMPELOEIL_PARAM_LIST1(__VA_ARGS__),                                        \
  ::trompeloeil::param_list_t<__VA_ARGS__, 1> p2

#define TROMPELOEIL_PARAM_LIST1(...)                                           \
  ::trompeloeil::param_list_t<__VA_ARGS__, 0> p1

#define TROMPELOEIL_PARAM_LIST0(func_type)

#define TROMPELOEIL_PARAM_LIST(num, func_type)                                 \
  TROMPELOEIL_CONCAT(TROMPELOEIL_PARAM_LIST, num)                              \
    (TROMPELOEIL_REMOVE_PAREN(func_type))


#define TROMPELOEIL_PARAMS15 TROMPELOEIL_PARAMS14, p15
#define TROMPELOEIL_PARAMS14 TROMPELOEIL_PARAMS13, p14
#define TROMPELOEIL_PARAMS13 TROMPELOEIL_PARAMS12, p13
#define TROMPELOEIL_PARAMS12 TROMPELOEIL_PARAMS11, p12
#define TROMPELOEIL_PARAMS11 TROMPELOEIL_PARAMS10, p11
#define TROMPELOEIL_PARAMS10 TROMPELOEIL_PARAMS9,  p10
#define TROMPELOEIL_PARAMS9  TROMPELOEIL_PARAMS8,  p9
#define TROMPELOEIL_PARAMS8  TROMPELOEIL_PARAMS7,  p8
#define TROMPELOEIL_PARAMS7  TROMPELOEIL_PARAMS6,  p7
#define TROMPELOEIL_PARAMS6  TROMPELOEIL_PARAMS5,  p6
#define TROMPELOEIL_PARAMS5  TROMPELOEIL_PARAMS4,  p5
#define TROMPELOEIL_PARAMS4  TROMPELOEIL_PARAMS3,  p4
#define TROMPELOEIL_PARAMS3  TROMPELOEIL_PARAMS2,  p3
#define TROMPELOEIL_PARAMS2  TROMPELOEIL_PARAMS1,  p2
#define TROMPELOEIL_PARAMS1  ,                     p1
#define TROMPELOEIL_PARAMS0

#define TROMPELOEIL_PARAMS(num) TROMPELOEIL_CONCAT(TROMPELOEIL_PARAMS, num)

#if (TROMPELOEIL_CPLUSPLUS == 201103L)

#define TROMPELOEIL_DECLTYPE_AUTO \
  auto

#define TROMPELOEIL_TRAILING_RETURN_TYPE(return_type) \
  -> return_type

#else /* (TROMPELOEIL_CPLUSPLUS == 201103L) */

#define TROMPELOEIL_DECLTYPE_AUTO \
  decltype(auto)

#define TROMPELOEIL_TRAILING_RETURN_TYPE(return_type) \
  /**/

#endif /* !(TROMPELOEIL_CPLUSPLUS == 201103L) */
#if TROMPELOEIL_CPLUSPLUS > 201403L && (!TROMPELOEIL_GCC || TROMPELOEIL_GCC_VERSION >= 70000)
#  define TROMPELOEIL_INLINE_VAR [[maybe_unused]] static inline
#else
#  define TROMPELOEIL_INLINE_VAR static
#endif

static constexpr bool trompeloeil_movable_mock = false;

namespace trompeloeil
{
  template <typename T>
  struct identity_type
  {
    using type = T;
  };

  template <typename R, typename C, typename ... Args>
  identity_type<R(Args...)>
  nonconst_member_signature(R (C::*)(Args...));

  template <typename R, typename C, typename ... Args>
  identity_type<R(Args...)>
  const_member_signature(R (C::*)(Args...) const);

#ifdef STDMETHODCALLTYPE
  template<typename R, typename C, typename ... Args>
  identity_type<R(Args...)>
  stdcall_member_signature(R (STDMETHODCALLTYPE C::*)(Args...));
#endif

  template <typename ...>
  struct void_t_
  {
    using type = void;
  };

  template <typename ... T>
  using void_t = typename void_t_<T...>::type;

  template <template <typename ...> class, typename, typename ...>
  struct is_detected_{
    using type = std::false_type;
  };

  template <template <typename ...> class D, typename ... Ts>
  struct is_detected_<D, void_t<D<Ts...>>, Ts...>
  {
    using type = std::true_type;
  };

  template <template <typename ...> class D, typename ... Ts>
  using is_detected = typename is_detected_<D, void, Ts...>::type;
}
# if (TROMPELOEIL_CPLUSPLUS == 201103L)

#include "cpp11_shenanigans.hpp"

# else /* (TROMPELOEIL_CPLUSPLUS == 201103L) */
namespace trompeloeil {
  /* Only these entities really need to
   * be available in namespace detail, but
   * VS 2015 has a problem with this approach.
   *
   *  namespace detail {
   *
   *  using std::make_unique;
   *
   *  using std::conditional_t;
   *  using std::decay_t;
   *  using std::enable_if_t;
   *  using std::remove_pointer_t;
   *  using std::remove_reference_t;
   *
   *  using std::exchange;
   *  using std::index_sequence;
   *  using std::index_sequence_for;
   *  using std::integer_sequence;
   *  using std::make_index_sequence;
   *
   *  }
   *
   * Instead, use a namespace alias.
   */
  namespace detail = std;
}
# endif /* !(TROMPELOEIL_CPLUSPLUS == 201103L) */
namespace trompeloeil {
# if TROMPELOEIL_CPLUSPLUS >= 201703L
#   if TROMPELOEIL_CLANG && TROMPELOEIL_CLANG_VERSION >= 60000

  // these are mostly added to work around clang++ bugs
  // https://bugs.llvm.org/show_bug.cgi?id=38033
  // https://bugs.llvm.org/show_bug.cgi?id=38010

  template <typename T>
  using move_construct_type = decltype(T(std::declval<T&&>()));

  template <typename T>
  using copy_construct_type = decltype(T(std::declval<const T&>()));

  template <typename T>
  using can_move_construct = is_detected<move_construct_type, detail::decay_t<T>>;

  template <typename T>
  using can_copy_construct = is_detected<copy_construct_type, detail::decay_t<T>>;

#   else
  template <typename T>
  using can_move_construct = std::is_move_constructible<T>;

  template <typename T>
  using can_copy_construct = std::is_copy_constructible<T>;
#   endif

  template <typename F, typename ... A>
  using invoke_result_type = decltype(std::declval<F&>()(std::declval<A>()...));

# else

  template <typename T>
  using can_move_construct = std::is_move_constructible<T>;

  template <typename T>
  using can_copy_construct = std::is_copy_constructible<T>;

  template <typename F, typename ... A>
  using invoke_result_type = decltype(std::declval<F&>()(std::declval<A>()...));

# endif

#if defined(__cpp_impl_coroutine)
  template <typename Sig, typename = void>
  struct is_coroutine : std::false_type {};

  template <typename Sig>
  struct is_coroutine<Sig, std::void_t<decltype(&std::coroutine_traits<Sig>::promise_type::initial_suspend)>> : std::true_type {};
#else
  template <typename>
  using is_coroutine = std::false_type;
#endif

  template <typename T>
  class mini_span
  {
  public:
    mini_span(T* address, size_t size) noexcept : begin_(address), end_(address + size) {}
    T* begin() const noexcept { return begin_; }
    T* end() const noexcept { return end_; }
  private:
    T* begin_;
    T* end_;
  };

  class specialized;


#ifndef TROMPELOEIL_CUSTOM_RECURSIVE_MUTEX

  template <typename T = void>
  unique_lock<std::recursive_mutex> get_lock()
  {
    // Ugly hack for lifetime of mutex. The statically allocated
    // recursive_mutex is intentionally leaked, to ensure that the
    // mutex is available and valid even if the last use is from
    // the destructor of a global object in a translation unit
    // without #include <trompeloeil.hpp>

    alignas(std::recursive_mutex)
    static char buffer[sizeof(std::recursive_mutex)];

    static auto mutex = new (&buffer) std::recursive_mutex;
    return unique_lock<std::recursive_mutex>{*mutex};
  }

#else

  class custom_recursive_mutex {
  public:
    virtual ~custom_recursive_mutex() = default;
    virtual void lock() = 0;
    virtual void unlock() = 0;
  };

  // User has to provide an own recursive mutex.
  std::unique_ptr<custom_recursive_mutex> create_custom_recursive_mutex();

  template <typename T = void>
  unique_lock<custom_recursive_mutex> get_lock()
  {
    static std::unique_ptr<custom_recursive_mutex> mtx =
        create_custom_recursive_mutex();
    return unique_lock<custom_recursive_mutex>{*mtx};
  }

#endif

  template <size_t N, typename T>
  using conditional_tuple_element
    = detail::conditional_t<(N < std::tuple_size<T>::value),
                         typename std::tuple_element<N, T>::type,
                         int>;

  template <typename T>
  struct param_list;

  template <typename R, typename ... P>
  struct param_list<R(P...)>
  {
    static size_t constexpr const size = sizeof...(P);
    template <size_t N>
    using type = conditional_tuple_element<N, std::tuple<P...>>;
  };

  template <typename Sig, size_t N>
  using param_list_t = typename param_list<Sig>::template type<N>;

  class expectation_violation : public std::logic_error
  {
  public:
    using std::logic_error::logic_error;
  };

  struct location
  {
    location()
    noexcept
      : file("")
      , line(0U)
    {}

    location(
      char const* file_,
      unsigned long line_
    )
    noexcept
      : file{file_}
      , line{line_}
    {}

    char const *file;
    unsigned long line;

    friend
    std::ostream&
    operator<<(
      std::ostream& os,
      const location& loc)
    {
      if (loc.line != 0U) os << loc.file << ':' << loc.line;
      return os;
    }
  };

  enum class severity { fatal, nonfatal };

  using reporter_func = std::function<void(severity,
                                           char const *file,
                                           unsigned long line,
                                           std::string const &msg)>;

  using ok_reporter_func = std::function<void(char const *msg)>;

  inline
  void
  default_reporter(
    severity,
    char const *file,
    unsigned long line,
    std::string const &msg)
  {
    if (!std::current_exception())
    {
      std::stringstream os;
      os << location{ file, line } << "\n" << msg;
      throw expectation_violation(os.str());
    }
  }

  inline
  void
  default_ok_reporter(char const* /*msg*/)
  {
    /* OK reporter defaults to doing nothing. */
  }

  inline
  reporter_func&
  reporter_obj()
  {
    static reporter_func obj = default_reporter;
    return obj;
  }

  inline
  ok_reporter_func&
  ok_reporter_obj()
  {
    static ok_reporter_func obj = default_ok_reporter;
    return obj;
  }

  inline
  reporter_func
  set_reporter(
    reporter_func f)
  {
    return detail::exchange(reporter_obj(), std::move(f));
  }

  inline
  std::pair<reporter_func, ok_reporter_func>
  set_reporter(
    reporter_func rf,
    ok_reporter_func orf)
  {
    return {
      set_reporter(std::move(rf)),
      detail::exchange(ok_reporter_obj(), std::move(orf))
    };
  }

  class tracer;

  inline
  tracer*&
  tracer_obj()
  noexcept
  {
    static tracer* ptr = nullptr;
    return ptr;
  }

  inline
  tracer*
  set_tracer(
    tracer* obj)
  noexcept
  {
    // std::exchange would be sane here, but it costs compilation time
    auto& ptr = tracer_obj();
    auto rv = ptr;
    ptr = obj;
    return rv;
  }

  class tracer
  {
  public:
    virtual
    void
    trace(
      char const *file,
      unsigned long line,
      std::string const &call) = 0;
  protected:
    tracer() = default;
    tracer(tracer const&) = delete;
    tracer& operator=(tracer const&) = delete;
    virtual
    ~tracer()
    {
      set_tracer(previous);
    }
  private:
    tracer* previous = set_tracer(this);
  };

  class trace_agent;

  template <typename T>
  struct reporter;

  template <typename T>
  void
  send_report(
    severity s,
    location loc,
    std::string const &msg)
  {
    reporter<T>::send(s, loc.file, loc.line, msg.c_str());
  }

  template <typename T>
  void
  send_ok_report(
    std::string const &msg)
  {
    reporter<T>::sendOk(msg.c_str());
  }

  template <typename T>
  struct reporter
  {
    static
    void
    send(
      severity s,
      char const *file,
      unsigned long line,
      char const *msg);

    static
    void
    sendOk(
      char const *msg);

  };

  template <typename T>
  void reporter<T>::
    send(
      severity s,
      char const *file,
      unsigned long line,
      char const *msg)
    {
      reporter_obj()(s, file, line, msg);
    }

  template <typename T>
  void reporter<T>::
    sendOk(char const* msg)
    {
      ok_reporter_obj()(msg);
    }

  template <typename ... T>
  constexpr
  bool
  ignore(
    T const& ...)
  noexcept
  {
    return true;
  }

  struct illegal_argument
  {
    template <bool b = false>
    constexpr
    illegal_argument const& operator&() const
    {
      static_assert(b, "illegal argument");
      return *this;
    }

    template <bool b = false>
    constexpr
    illegal_argument const& operator*() const
    {
      static_assert(b, "illegal argument");
      return *this;
    }

    template <typename T, bool b = false>
    constexpr
    illegal_argument const& operator=(T const&) const
    {
      static_assert(b, "illegal argument");
      return *this;
    }

    template <typename T, bool b = false>
    constexpr
    operator T() const
    {
      static_assert(b, "illegal argument");
      return {};
    }
  };

  struct matcher { };

  struct wildcard : public matcher
  {
    template <typename T>
    struct wrapper {
      operator T() {
        static_assert(std::is_same<T, void>{},
                      "Getting a value from wildcard is not allowed.\n"
                      "See https://github.com/rollbear/trompeloeil/issues/270\n"
                      "and https://github.com/rollbear/trompeloeil/issues/290");
        return *this;
      }
    };
    template <typename T, typename std::enable_if<!std::is_convertible<wildcard&, T>{}>::type* = nullptr>
    operator T()
    {
      return wrapper<T>{};
    }

    template <typename T>
    operator T&&() const
    {
#if (!TROMPELOEIL_GCC  || TROMPELOEIL_GCC_VERSION < 60000 || TROMPELOEIL_GCC_VERSION >= 90000) \
      && (!TROMPELOEIL_CLANG || TROMPELOEIL_CLANG_VERSION >= 40000 || !defined(_LIBCPP_STD_VER) || _LIBCPP_STD_VER != 14)
      return wrapper<T &&>{};
#else
      return *this;
#endif
    }

    template <typename T>
    operator T&() const volatile
    {
      return wrapper<T&>{};
    }

    template <typename T>
    constexpr
    bool
    matches(
      T const&)
    const
    noexcept
    {
      return true;
    }
  };

  TROMPELOEIL_INLINE_VAR wildcard _{};

template <typename T>
  using matcher_access = decltype(static_cast<matcher*>(std::declval<typename std::add_pointer<T>::type>()));

  template <typename T>
  using is_matcher = typename is_detected<matcher_access, T>::type;


  template <typename T>
  using ostream_insertion = decltype(std::declval<std::ostream&>() << std::declval<T>());

  template <typename T>
  using is_output_streamable = std::integral_constant<bool, is_detected<ostream_insertion, T>::value && !std::is_array<T>::value>;

  struct stream_sentry
  {
    explicit
    stream_sentry(
      std::ostream& os_)
      : os(os_)
      , width(os.width(0))
      , flags(os.flags(std::ios_base::dec | std::ios_base::left))
      , fill(os.fill(' '))
      {  }
      stream_sentry(
        stream_sentry const&)
      = delete;
    ~stream_sentry()
    {
      os.flags(flags);
      os.fill(fill);
      os.width(width);
    }
  private:
    std::ostream& os;
    std::streamsize width;
    std::ios_base::fmtflags flags;
    char fill;
  };

  struct indirect_null {
#if !TROMPELOEIL_CLANG
    template <
      typename T,
      typename = detail::enable_if_t<std::is_convertible<std::nullptr_t, T>::value>
    >
    operator T&&() const = delete;
#endif
#if TROMPELOEIL_GCC || TROMPELOEIL_MSVC

    template <typename T, typename C, typename ... As>
    using memfunptr = T (C::*)(As...);

    template <typename T>
    operator T*() const;
    template <typename T, typename C>
    operator T C::*() const;
    template <typename T, typename C, typename ... As>
    operator memfunptr<T,C,As...>() const;
#endif /* TROMPELOEIL_GCC || TROMPELOEIL_MSVC */
    operator std::nullptr_t() const;
  };

  template <typename T, typename U>
  using equality_comparison = decltype((std::declval<T const&>() == std::declval<U const&>())
                                       ? true
                                       : false);

  template <typename T, typename U>
  using is_equal_comparable = is_detected<equality_comparison, T, U>;

#if defined(_MSC_VER) && (_MSC_VER < 1910)
  template <typename T>
  using is_null_comparable = is_equal_comparable<T, std::nullptr_t>;
#else
  template <typename T>
  using is_null_comparable = is_equal_comparable<T, indirect_null>;
#endif

  template <typename T, typename = decltype(std::declval<T const&>() == nullptr)>
  constexpr
  auto
  is_null_redirect(
    T const &t)
  noexcept(noexcept(std::declval<T const&>() == nullptr))
  -> decltype(t == nullptr)
  {
    return t == nullptr;
  }

  template <typename T>
  constexpr
  auto
  is_null(
    T const &t,
    std::true_type)
  noexcept(noexcept(is_null_redirect(t)))
  -> decltype(is_null_redirect(t))
  {
    // Redirect evaluation to suppress wrong non-null warnings in g++ 9 and 10.
    return is_null_redirect(t);
  }

  template <typename T, typename V>
  constexpr
  bool
  is_null(
    T const &,
    V)
  noexcept
  {
    return false;
  }

  template <typename T>
  constexpr
  bool
  is_null(
    T const &t)
  {
    // g++-4.9 uses C++11 rules for constexpr function, so can't
    // break this up into smaller bits
    using tag = std::integral_constant<bool, is_null_comparable<T>::value
                                       && !is_matcher<T>::value
                                       && !std::is_array<T>::value>;

    return ::trompeloeil::is_null(t, tag{});
  }

  template <typename T>
  constexpr
  bool
  is_null(
    std::reference_wrapper<T> t)
  {
    return is_null(t.get());
  }

#if TROMPELOEIL_HAS_EXPECTED
  template <typename T, typename E>
  constexpr
  bool
  is_null(const std::expected<T, E>& e)
  {
    if constexpr (requires { e.value() == nullptr; }) {
      return e == nullptr;
    }
    else
    {
      return false;
    }
  }
#endif

  template <typename T>
  void
  print(
    std::ostream& os,
    T const &t);

  template <typename T>
  using iterable = decltype(std::begin(std::declval<T&>()) == std::end(std::declval<T&>()));

  template <typename T>
  using is_collection = is_detected<iterable, T>;

  template <typename T,
            bool = is_output_streamable<const T>::value,
            bool = is_collection<const detail::remove_reference_t<T>>::value>
  struct streamer
  {
    static
    void
    print(
      std::ostream& os,
      T const &t)
    {
      stream_sentry s(os);
      os << t;
    }
  };

  template <typename ... T>
  struct streamer<std::tuple<T...>, false, false>
  {
    static
    void
    print(
      std::ostream& os,
      std::tuple<T...> const&  t)
    {
      print_tuple(os, t, detail::index_sequence_for<T...>{});
    }
    template <size_t ... I>
    static
    void
    print_tuple(
      std::ostream& os,
      std::tuple<T...> const& t,
      detail::index_sequence<I...>)
    {
      os << "{ ";
      const char* sep = "";
      std::initializer_list<const char*> v{((os << sep),
                                            ::trompeloeil::print(os, std::get<I>(t)),
                                            (sep = ", "))...};
      ignore(v);
      os << " }";
    }
  };

  template <typename T, typename U>
  struct streamer<std::pair<T, U>, false, false>
  {
    static
    void
    print(
      std::ostream& os,
      std::pair<T, U> const& t)
    {
      os << "{ ";
      ::trompeloeil::print(os, t.first);
      os << ", ";
      ::trompeloeil::print(os, t.second);
      os << " }";
    }
  };

  template <typename T>
  struct streamer<T, false, true>
  {
    static
    void
    print(
      std::ostream& os,
      T const& t)
    {
      os << "{ ";
      const char* sep = "";
      using element_type = decltype(*std::begin(t));
      std::for_each(std::begin(t), std::end(t),
                    [&os, &sep](element_type element) {
                      os << sep;
                      ::trompeloeil::print(os, element);
                      sep = ", ";
                    });
      os << " }";
    }
  };

  inline void hexdump(const void* begin, size_t size, std::ostream& os)
  {
    stream_sentry s(os);
    os << size << "-byte object={";
    if (size > 8)
    {
      os << '\n';
    }
    os << std::setfill('0') << std::hex;
    mini_span<uint8_t const> bytes(static_cast<uint8_t const*>(begin), size);
    size_t byte_number = 0;
    std::for_each(bytes.begin(), bytes.end(),  [&os, &byte_number](unsigned byte) {
      os << " 0x" << std::setw(2) << std::right << byte;
      if ((byte_number & 0xf) == 0xf) os << '\n';
      ++byte_number;
    });
    os << " }";
  }

  template <typename T>
  struct streamer<T, false, false>
  {
    static
    void
    print(
      std::ostream& os,
      T const &t)
    {
      hexdump(&t, sizeof(T), os);
    }
  };

  template <typename T, typename = void>
  struct printer
  {
    static
    void
    print(
      std::ostream& os,
      T const & t)
    {
      streamer<T>::print(os, t);
    }
  };

  template <>
  struct printer<wildcard>
  {
    static
    void
    print(
    std::ostream& os,
    wildcard const&)
    {
      os << " matching _";
    }
  };

  template <typename T>
  struct printer<std::reference_wrapper<T>>
  {
    static
    void
    print(
    std::ostream& os,
    std::reference_wrapper<T> ref)
    {
        ::trompeloeil::print(os, ref.get());
    }
  };

  template <typename T>
  void
  print(
    std::ostream& os,
    T const &t)
  {
    if (is_null(t))
    {
      os << "nullptr";
    }
    else
    {
      printer<T>::print(os, t);
    }
  }

  inline
  void
  print(
      std::ostream& os,
      std::nullptr_t)
  {
    os << "nullptr";
  }

  constexpr
  auto
  param_compare_operator(
    void const*)
  TROMPELOEIL_TRAILING_RETURN_TYPE(const char*)
  {
    return " == ";
  }

  constexpr
  auto
  param_compare_operator(
    matcher const*)
  TROMPELOEIL_TRAILING_RETURN_TYPE(const char*)
  {
    return "";
  }

  template <typename T>
  void
  print_expectation(
    std::ostream& os,
    T const& t)
  {
    os << param_compare_operator(&t);
    print(os, t);
    os << '\n';
  }

  template <typename T>
  class list_elem
  {
  public:
    list_elem(
      const list_elem&)
    = delete;

    list_elem(
      list_elem &&r)
    noexcept
    {
      *this = std::move(r);
    }

    list_elem&
    operator=(
      list_elem &&r)
    noexcept
    {
      if (this != &r)
      {
        next = r.next;
        prev = &r;
        r.invariant_check();

        next->prev = this;
        r.next = this;

        TROMPELOEIL_ASSERT(next->prev == this);
        TROMPELOEIL_ASSERT(prev->next == this);

        r.unlink();

        TROMPELOEIL_ASSERT(!r.is_linked());
        invariant_check();
      }
      return *this;
    }

    list_elem&
    operator=(
      const list_elem&)
    = delete;

    virtual
    ~list_elem()
    {
      unlink();
    }

    void
    unlink()
    noexcept
    {
      invariant_check();
      auto n = next;
      auto p = prev;
      n->prev = p;
      p->next = n;
      next = this;
      prev = this;
      invariant_check();
    }

    void
    invariant_check()
    const
    noexcept
    {
#ifdef TROMPELOEIL_SANITY_CHECKS
      TROMPELOEIL_ASSERT(next->prev == this);
      TROMPELOEIL_ASSERT(prev->next == this);
      TROMPELOEIL_ASSERT((next == this) == (prev == this));
      TROMPELOEIL_ASSERT((prev->next == next) == (next == this));
      TROMPELOEIL_ASSERT((next->prev == prev) == (prev == this));
      auto pp = prev;
      auto nn = next;
      do {
        TROMPELOEIL_ASSERT((nn == this) == (pp == this));
        TROMPELOEIL_ASSERT(nn->next->prev == nn);
        TROMPELOEIL_ASSERT(nn->prev->next == nn);
        TROMPELOEIL_ASSERT(pp->next->prev == pp);
        TROMPELOEIL_ASSERT(pp->prev->next == pp);
        TROMPELOEIL_ASSERT((nn->next == nn) == (nn == this));
        TROMPELOEIL_ASSERT((pp->prev == pp) == (pp == this));
        nn = nn->next;
        pp = pp->prev;
      } while (nn != this);
#endif
    }

    bool
    is_linked()
    const
    noexcept
    {
      invariant_check();
      return next != this;
    }
  protected:
    list_elem() = default;
  public:
    list_elem* next = this;
    list_elem* prev = this;
  };

  class ignore_disposer
  {
  protected:
    template <typename T>
    TROMPELOEIL_NORETURN
    void
    dispose(
      T*)
    const
    noexcept
    {
      std::abort(); // must never be called
    }
  };

  class delete_disposer
  {
  protected:
    template <typename T>
    void
    dispose(
      T* t)
    const
    {
      delete t;
    }
  };

  template <typename T, typename Disposer = ignore_disposer>
  class list : private list_elem<T>, private Disposer
  {
  public:
    list() noexcept;
    list(list&&) noexcept;
    list(const list&) = delete;
    list& operator=(list&&) noexcept;
    list& operator=(const list&) = delete;
    ~list() override;
    class iterator;
    iterator begin() const noexcept;
    iterator end() const noexcept;
    iterator push_front(T* t) noexcept;
    iterator push_back(T* t) noexcept;
    bool empty() const noexcept { return begin() == end(); }
  private:
    using list_elem<T>::invariant_check;
    using list_elem<T>::next;
    using list_elem<T>::prev;
  };

  template <typename T, typename Disposer>
  class list<T, Disposer>::iterator
  {
    friend class list<T, Disposer>;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T&;
  public:
    friend
    bool
    operator==(
      iterator const &lh,
      iterator const &rh)
    noexcept
    {
      return lh.p == rh.p;
    }

    friend
    bool
    operator!=(
      iterator const &lh,
      iterator const &rh)
    noexcept
    {
      return !(lh == rh);
    }

    iterator&
    operator++()
    noexcept
    {
      p = p->next;
      return *this;
    }

    iterator
    operator++(int)
    noexcept
    {
      auto rv = *this;
      operator++();
      return rv;
    }

    T&
    operator*()
    noexcept
    {
      return static_cast<T&>(*p);
    }

    T*
    operator->()
    noexcept
    {
      return static_cast<T*>(p);
    }

  private:
    explicit
    iterator(
      list_elem<T> const *t)
    noexcept
    : p{const_cast<list_elem<T>*>(t)}
    {}

    list_elem<T>* p = nullptr;
  };

  template <typename T, typename Disposer>
  list<T, Disposer>::list() noexcept = default;

  template <typename T, typename Disposer>
  list<T, Disposer>::list(list&&) noexcept = default;

  template <typename T, typename Disposer>
  list<T, Disposer>& list<T, Disposer>::operator=(list&&) noexcept = default;

  template <typename T, typename Disposer>
  list<T, Disposer>::~list()
  {
    auto i = this->begin();
    while (i != this->end())
    {
      auto& elem = *i;
      ++i; // intrusive list, so advance before destroying
      Disposer::dispose(&elem);
    }
  }

  template <typename T, typename Disposer>
  auto
  list<T, Disposer>::begin()
  const
  noexcept
  -> iterator
  {
    return iterator{next};
  }

  template <typename T, typename Disposer>
  auto
  list<T, Disposer>::end()
  const
  noexcept
  -> iterator
  {
    return iterator{this};
  }

  template <typename T, typename Disposer>
  auto
  list<T, Disposer>::push_front(
    T* t)
  noexcept
  -> iterator
  {
    invariant_check();
    t->next = next;
    t->prev = this;
    next->prev = t;
    next = t;
    invariant_check();
    return iterator{t};
  }

  template <typename T, typename Disposer>
  auto
  list<T, Disposer>::push_back(
    T* t)
  noexcept
  -> iterator
  {
    invariant_check();
    t->prev = prev;
    t->next = this;
    prev->next = t;
    prev = t;
    invariant_check();
    return iterator{t};
  }


  class sequence_matcher;


  class sequence;
  struct sequence_handler_base;

  template <size_t N>
  struct sequence_matchers;

  template <>
  struct sequence_matchers<0>
  {
    void
    validate(
      severity,
      char const*,
      location)
    {
    }
    unsigned
    order()
      const
      noexcept
    {
      return 0;
    }
    void
    retire()
      noexcept
    {
    }
    void
    retire_predecessors()
      noexcept
    {
    }
  };


  template <typename T>
  void can_match_parameter(T&);

  template <typename T>
  void can_match_parameter(T&&);


  inline
  std::string
  param_name_prefix(
    void const*)
  {
    return "";
  }

  template <typename T>
  struct null_on_move
  {
  public:
    null_on_move()
    noexcept
    = default;

    null_on_move(
      null_on_move&&)
    noexcept
    {}

    null_on_move(
      null_on_move const&)
    noexcept
    {}

    ~null_on_move()
    = default;

    null_on_move&
    operator=(
      const null_on_move&)
    noexcept
    {
      p = nullptr;
      return *this;
    }

    null_on_move&
    operator=(
      null_on_move&&)
    noexcept
    {
      p = nullptr;
      return *this;
    }

    null_on_move&
    operator=(
      T* t)
    noexcept
    {
      p = t;
      return *this;
    }

    T*&
    leak()
    noexcept
    {
      return p;
    }

    T&
    operator*()
    const
    noexcept
    {
      return *p;
    }

    T*
    operator->()
    const
    noexcept
    {
      return p;
    }

    explicit
    operator bool()
    const
    noexcept
    {
      return p != nullptr;
    }
  private:
    T* p = nullptr;
  };

  struct sequence_handler_base
  {
  private:
    size_t min_calls{1};
    size_t max_calls{1};
    size_t call_count{0};
  public:

    virtual
    ~sequence_handler_base()
    = default;

    void
      increment_call()
      noexcept
    {
      ++call_count;
    }
    bool
      is_satisfied()
      const
      noexcept
    {
      return call_count >= min_calls;
    }

    bool
      is_saturated()
      const
      noexcept
    {
      return call_count == max_calls;
    }

    bool
      is_forbidden()
      const
      noexcept
    {
      return max_calls == 0ULL;
    }

    void
    set_limits(size_t L, size_t H)
      noexcept
    {
      min_calls = L;
      max_calls = H;
    }

    size_t
    get_min_calls()
      const
      noexcept
    {
      return min_calls;
    }

    size_t
      get_calls()
      const
      noexcept
    {
      return call_count;
    }

    virtual
    void
      validate(severity s, char const *, location) = 0;

    virtual
    bool
    can_be_called()
    const
    noexcept = 0;

    virtual
    unsigned
    order()
    const
    noexcept = 0;

    virtual
    void
      retire()
      noexcept = 0;

    virtual
    void
    retire_predecessors()
    noexcept = 0;
  protected:
    sequence_handler_base() = default;
    sequence_handler_base(const sequence_handler_base&) = default;
  };


  template <size_t N>
  struct sequence_handler : public sequence_handler_base
  {
  public:
    template <size_t M = N, typename detail::enable_if_t<M == 0>* = nullptr>
    sequence_handler()
      noexcept
      : matchers{}
    {}

    template <typename ... S>
    sequence_handler(
      const sequence_handler_base& base,
      char const *name,
      location loc,
      S&& ... s)
    noexcept
      : sequence_handler_base(base)
      , matchers{{{sequence_matcher{name, loc, *this, std::forward<S>(s)}...}}}
    {
    }

    void
    validate(
      severity s,
      char const *match_name,
      location loc)
    override
    {
      matchers.validate(s, match_name, loc);
    }

    unsigned
    order()
    const
    noexcept
    override
    {
      return matchers.order();
    }

    bool
    can_be_called()
    const
    noexcept
    override
    {
      return order() != ~0U;
    }

    void
    retire()
    noexcept
    override
    {
      matchers.retire();
    }

    void
    retire_predecessors()
    noexcept
      override
    {
      matchers.retire_predecessors();
    }
  private:
    sequence_matchers<N> matchers;
  };


  struct expectation {
    virtual ~expectation() = default;
    virtual bool is_satisfied() const noexcept = 0;
    virtual bool is_saturated() const noexcept = 0;
  };


  template <typename T>
  struct return_of;

  template <typename R, typename ... A>
  struct return_of<R(A...)>
  {
    using type = R;
  };

  template <typename T>
  using return_of_t = typename return_of<T>::type;

  template <typename T>
  struct call_params_type;

  template <typename R, typename ... T>
  struct call_params_type<R(T...)>
  {
    using type = std::tuple<std::reference_wrapper<detail::remove_reference_t<T>>...>;
  };

  template <typename T>
  using call_params_type_t = typename call_params_type<T>::type;


  template <typename R>
  struct default_return_t
  {
    TROMPELOEIL_NORETURN static R value()
    {
      std::abort(); // must never be called
    }
  };

  template <typename Sig, bool = is_coroutine<return_of_t<Sig>>::value>
  struct yield_expr_base;

  template <typename Sig>
  using yield_expr_list = list<yield_expr_base<Sig>, delete_disposer>;

#if TROMPELOEIL_MSVC
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
  template <typename R>
  inline
  R
  default_return()
  {
    return default_return_t<R>::value();
  }
#if TROMPELOEIL_MSVC
#pragma warning(pop)
#endif



  template <>
  inline
  void
  default_return<void>()
  {
    // Implicitly do nothing when returning from function returning void
  }

  template <typename Sig>
  struct call_matcher_base;

  template <typename Sig>
  struct call_matcher_list : public list<call_matcher_base<Sig>>
  {
    void decommission()
    {
      auto lock = get_lock();
      auto iter = this->begin();
      auto const e = this->end();
      while (iter != e)
      {
        auto &m = *iter;
        ++iter; // intrusive list, so must advance to next before destroying
        m.mock_destroyed();
        m.unlink();
      }
    }
  };

  template <typename Sig>
  struct call_matcher_base : public list_elem<call_matcher_base<Sig>>
  {
    using sig = Sig;
    call_matcher_base(
      location loc_,
      char const* name_)
    : loc{loc_}
    , name{name_}
    {
    }

    call_matcher_base(call_matcher_base&&) = delete;

    ~call_matcher_base() override = default;

    virtual
    void
    mock_destroyed() = 0;

    virtual
    bool
    matches(
      call_params_type_t<Sig> const&)
    const = 0;

    virtual
    unsigned
    sequence_cost()
    const
    noexcept = 0;

    virtual
    void
    run_actions(
      call_params_type_t<Sig> &,
      call_matcher_list<Sig> &saturated_list
    ) = 0;

    virtual
    std::ostream&
    report_signature(
      std::ostream&)
    const = 0;

    TROMPELOEIL_NORETURN
    virtual
    std::ostream&
    report_mismatch(
      std::ostream&,
      call_params_type_t<Sig> const &) = 0;

    virtual
    return_of_t<Sig>
    return_value(
      trace_agent&,
      call_params_type_t<Sig>& p) = 0;

    location loc;
    char const *name;
  };

  template <typename T, typename U>
  bool
  param_matches_impl(
    T const& t,
    std::reference_wrapper<U> u,
    matcher const*)
  noexcept(noexcept(t.matches(u.get())))
  {
    return t.matches(u.get());
  }

  template <typename T,
            typename U,
            typename = detail::enable_if_t<is_equal_comparable<T, U>::value>>
  inline
  U&
  identity(
    U& t)
  noexcept
  {
    return t;
  }

  template <typename T,
            typename U,
            typename = detail::enable_if_t<!is_equal_comparable<T, U>::value>>
  inline
  T
  identity(
    const U& u)
  noexcept(noexcept(T(u)))
  {
    return u;
  }

  template <typename T, typename U>
  bool
  param_matches_impl(
    T const& t,
    std::reference_wrapper<U> u,
    void const*)
  noexcept(noexcept(::trompeloeil::identity<U>(t) == u.get()))
  {
    return ::trompeloeil::identity<U>(t) == u.get();
  }

  template <typename T, typename U>
  bool
  param_matches(
    T const& t,
    U const& u)
  noexcept(noexcept(param_matches_impl(t, u, &t)))
  {
    return ::trompeloeil::param_matches_impl(t, u, &t);
  }

  template <size_t ... I, typename T, typename U>
  bool
  match_parameters(
    detail::index_sequence<I...>,
    T const& t,
    U const& u)
    noexcept(noexcept(std::initializer_list<bool>{trompeloeil::param_matches(std::get<I>(t),std::get<I>(u))...}))
  {
    bool all_true = true;
    ::trompeloeil::ignore(t, u); // Kills unmotivated VS2015 warning in the empty case
    ::trompeloeil::ignore(std::initializer_list<bool>{all_true = all_true && ::trompeloeil::param_matches(std::get<I>(t), std::get<I>(u))...});
    return all_true;
  }

  template <typename ... T, typename ... U>
  bool
  match_parameters(
    std::tuple<T...> const& t,
    std::tuple<U...> const& u)
  noexcept(noexcept(match_parameters(detail::make_index_sequence<sizeof...(T)>{}, t, u)))
  {
    return ::trompeloeil::match_parameters(detail::make_index_sequence<sizeof...(T)>{}, t, u);
  }

  template <typename V, typename P>
  void print_mismatch(
    std::ostream& os,
    size_t num,
    V const& v,
    P const& p)
  {
    if (!::trompeloeil::param_matches(v, p))
    {
      auto prefix = param_name_prefix(&v) + "_";
      os << "  Expected " << std::setw((num < 9) ? 2 : 1) << prefix << num+1;
      ::trompeloeil::print_expectation(os, v);
    }
  }

  template <typename ... V, typename ... P, size_t ... I>
  void print_mismatch(
    std::ostream& os,
    detail::index_sequence<I...>,
    std::tuple<V...> const& v,
    std::tuple<P...> const& p)
  {
    ::trompeloeil::ignore(os, v, p);  // Kills unmotivated VS2015 warning in the empty case
    ::trompeloeil::ignore(std::initializer_list<int>{(print_mismatch(os, I, std::get<I>(v), std::get<I>(p)),0)...});
  }

  template <typename ... V, typename ... P>
  void print_mismatch(
    std::ostream& os,
    std::tuple<V...> const& v,
    std::tuple<P...> const& p)
  {
    print_mismatch(os, detail::make_index_sequence<sizeof...(V)>{}, v, p);
  }

  template <typename T>
  void missed_value(
    std::ostream& os,
    int i,
    T const& t)
  {
    auto prefix = param_name_prefix(&t) + "_";
    os << "  param " << std::setw((i < 9) ? 2 : 1) << prefix << i + 1
       << ::trompeloeil::param_compare_operator(&t);
    ::trompeloeil::print(os, t);
    os << '\n';
  }

  template <size_t ... I, typename ... T>
  void stream_params(
    std::ostream &os,
    detail::index_sequence<I...>,
    std::tuple<T...> const &t)
  {
    ::trompeloeil::ignore(os, t);  // Kills unmotivated VS2015 warning in the empty case
    ::trompeloeil::ignore(std::initializer_list<int>{(missed_value(os, I, std::get<I>(t)),0)...});
  }

  template <typename ... T>
  void
  stream_params(
    std::ostream &os,
    std::tuple<T...> const &t)
  {
    stream_params(os, detail::make_index_sequence<sizeof...(T)>{}, t);
  }

  template <typename ... T>
  std::string
  params_string(
    std::tuple<T...> const& t)
  {
    std::ostringstream os;
    stream_params(os, t);
    return os.str();
  }

  class trace_agent
  {
  public:
    trace_agent(
      location loc_,
      char const* name_,
      tracer* t_)
    : loc{loc_}
    , t{t_}
    {
      if (t)
      {
        os << name_ << " with.\n";
      }
    }

    trace_agent(trace_agent const&) = delete;

    trace_agent(trace_agent &&) = delete;

    ~trace_agent()
    {
      if (t)
      {
        t->trace(loc.file, loc.line, os.str());
      }
    }

    trace_agent&
    operator=(trace_agent const&) = delete;

    trace_agent&
    operator=(trace_agent &&) = delete;

    template <typename ... T>
    void
    trace_params(
      std::tuple<T...> const& params)
    {
      if (t)
      {
        stream_params(os, params);
      }
    }

    template <typename T>
    auto
    trace_return(
      T&& rv)
    -> T
    {
      if (t)
      {
        os << " -> ";
        print(os, rv);
        os << '\n';
      }
      return std::forward<T>(rv);
    }

    void
    trace_exception()
    {
      if (t)
      {
        try {
          throw;
        }
        catch (std::exception const& e)
        {
          os << "threw exception: what() = " << e.what() << '\n';
        }
        catch (...)
        {
          os << "threw unknown exception\n";
        }
      }
    }
  private:
    location loc;
    tracer* t;
    std::ostringstream os{};
  };

  template <typename Sig>
  call_matcher_base <Sig> *
  find(
    call_matcher_list <Sig> &list,
    call_params_type_t <Sig> const &p)
  noexcept
  {
    call_matcher_base<Sig>* first_match = nullptr;
    unsigned lowest_cost = ~0U;
    for (auto& i : list)
    {
      if (i.matches(p))
      {
        unsigned cost = i.sequence_cost();
        if (cost == 0)
        {
          return &i;
        }
        if (!first_match || cost < lowest_cost)
        {
          first_match = &i;
          lowest_cost = cost;
        }
      }
    }
    return first_match;
  }

  template <typename Sig>
  TROMPELOEIL_NORETURN
  void
  report_mismatch(
    call_matcher_list <Sig> &matcher_list,
    call_matcher_list <Sig> &saturated_list,
    std::string const &name,
    call_params_type_t <Sig> const &p)
  {
    std::ostringstream os;
    os << "No match for call of " << name << " with.\n";
    stream_params(os, p);
    bool saturated_match = false;
    for (auto& m : saturated_list)
    {
      if (m.matches(p))
      {
        if (!saturated_match)
        {
          os << "\nMatches saturated call requirement\n";
          saturated_match = true;
        }
        os << "  ";
        m.report_signature(os) << '\n';
      }
    }
    if (!saturated_match)
    {
      for (auto& m : matcher_list)
      {
        os << "\nTried ";
        m.report_mismatch(os, p);
      }
    }
    send_report<specialized>(severity::fatal, location{}, os.str());
    std::abort(); // must never get here.
  }

  template <typename Sig>
  void
  report_match(
    call_matcher_list <Sig> &matcher_list)
  {
    if(! matcher_list.empty())
    {
        send_ok_report<specialized>((matcher_list.begin())->name);
    }
  }

  template <typename Sig>
  class return_handler
  {
  public:
    virtual
    ~return_handler() = default;

    virtual
    return_of_t<Sig>
    call(
      trace_agent&,
      call_params_type_t<Sig>& params) = 0;
  };

  template <typename Ret, typename F, typename P, typename = detail::enable_if_t<std::is_void<Ret>::value>>
  void
  trace_return(
    trace_agent const&,
    F& func,
    P& params)
  {
    func(params);
  }

  template <typename Ret, typename F, typename P, typename = detail::enable_if_t<!std::is_void<Ret>::value>>
  Ret
  trace_return(
    trace_agent& agent,
    F& func,
    P& params)
  {
    /* Work around VS 2017 15.7.x C4702 warning by
     * enclosing the operation in an otherwise
     * unnecessary try/catch block.
     */
    try
    {
      return agent.trace_return(func(params));
    }
    catch (...)
    {
      throw;
    }
  }

  template <typename Sig, typename T>
  class return_handler_t : public return_handler<Sig>
  {
  public:
    template <typename U>
    explicit
    return_handler_t(
      U&& u)
    : func(std::forward<U>(u))
    {}

    return_of_t<Sig>
    call(
      trace_agent& agent,
      call_params_type_t<Sig>& params)
    override
    {
      return trace_return<return_of_t<Sig>>(agent, func, params);
    }
  private:
    T func;
  };

  template <typename Sig>
  class condition_base : public list_elem<condition_base<Sig>>
  {
  public:
    explicit
    condition_base(
      char const *n)
    noexcept
      : id(n)
    {}

    ~condition_base() override = default;

    virtual
    bool
    check(
      call_params_type_t<Sig> const&)
    const = 0;

    virtual
    char const*
    name()
    const
    noexcept
    {
      return id;
    }
  private:
    char const *id;
  };

  template <typename Sig>
  using condition_list = list<condition_base<Sig>, delete_disposer>;

  template <typename Sig, typename Cond>
  struct condition : public condition_base<Sig>
  {
    condition(
      char const *str_,
      Cond c_)
      : condition_base<Sig>(str_)
      , c(c_) {}

    bool
    check(
      call_params_type_t<Sig> const & t)
    const
    override
    {
      return c(t);
    }

  private:
    Cond c;
  };

  template <typename Sig>
  struct side_effect_base : public list_elem<side_effect_base<Sig>>
  {
    ~side_effect_base() override = default;

    virtual
    void
    action(
      call_params_type_t<Sig> &)
    const = 0;
  };

  template <typename Sig>
  using side_effect_list = list<side_effect_base<Sig>, delete_disposer>;

  template <typename Sig, typename Action>
  struct side_effect : public side_effect_base<Sig>
  {
    template <typename A>
    explicit
    side_effect(
      A&& a_)
    : a(std::forward<A>(a_))
    {}

    void
    action(
      call_params_type_t<Sig> &t)
    const
    override
    {
      a(t);
    }
  private:
    Action a;
  };
  template <size_t L, size_t H = L>
  struct multiplicity { };

  template <typename R, typename Parent>
  struct co_return_injector : Parent
  {
    using co_return_type = R;
  };

  template <typename R, typename Parent>
  struct return_injector : Parent
  {
    using return_type = R;
  };

  template <typename Parent>
  struct throw_injector : Parent
  {
    static bool const throws = true;
  };

  template <typename Parent>
  struct sideeffect_injector : Parent
  {
    static bool const side_effects = true;
  };

  template <typename Parent, size_t H>
  struct call_limit_injector : Parent
  {
    static bool   const call_limit_set = true;
    static size_t const upper_call_limit = H;
  };

  template <typename Parent>
  struct call_limit_injector<Parent, 0> : Parent
  {
    static bool   const call_limit_set = true;
    static size_t const upper_call_limit = 0;
  };

  template <typename Parent>
  struct sequence_injector : Parent
  {
    static bool const sequence_set = true;
  };

  template <typename H, typename signature>
  struct throw_handler_t
  {
    using R = decltype(default_return<return_of_t<signature>>());

    explicit
    throw_handler_t(H&& h_)
      : h(std::move(h_))
    {}

    template <typename T>
    R operator()(T& p)
    {
      /* Work around VS 2017 15.7.x C4702 warning by
       * enclosing the operation in an otherwise
       * unnecessary try/catch block.
       */
      try
      {
        h(p);
        abort();
      }
      catch (...)
      {
        throw;
      }
      return default_return<R>(); // unreachable code
    }

  private:
    H h;
  };


  template <typename Matcher, typename modifier_tag, typename Parent>
  struct call_modifier : public Parent
  {
    using typename Parent::signature;
    using typename Parent::co_return_type;
    using typename Parent::return_type;
    using Parent::call_limit_set;
    using Parent::upper_call_limit;
    using Parent::sequence_set;
    using Parent::throws;
    using Parent::side_effects;

    call_modifier(
       std::unique_ptr<Matcher>&& m)
    noexcept
      : matcher{std::move(m)}
    {}

    template <typename F, typename ... Ts>
    auto action(Ts&& ... ts) &&
    -> decltype(F::action(std::move(*this), std::forward<Ts>(ts)...))
    {
        return F::action(std::move(*this), std::forward<Ts>(ts)...);
    }
    call_modifier&&
    null_modifier()
    {
      return std::move(*this);
    }

    template <typename ... T,
              bool b = sequence_set>
    call_modifier<Matcher, modifier_tag, sequence_injector<Parent>>
    in_sequence(
      T&& ... t)
    {
      static_assert(!b,
                    "Multiple IN_SEQUENCE does not make sense."
                    " You can list several sequence objects at once");

      static_assert(upper_call_limit > 0,
                    "IN_SEQUENCE for forbidden call does not make sense");

      matcher->set_sequence(std::forward<T>(t)...);
      return {std::move(matcher)};
    }
    std::unique_ptr<Matcher> matcher;
  };

  struct with
  {
    template <typename Matcher, typename modifier_tag, typename Parent, typename D>
    static
    call_modifier<Matcher, modifier_tag, Parent>&&
    action(
      call_modifier<Matcher, modifier_tag, Parent>&& m,
      const char* str,
      D&& d)
    {
      m.matcher->add_condition(str, std::forward<D>(d));
      return std::move(m);
    }
  };

  struct sideeffect
  {
    template <typename Matcher, typename modifier_tag, typename Parent, typename A>
    static
    call_modifier<Matcher, modifier_tag, sideeffect_injector<Parent>>
    action(
      call_modifier<Matcher, modifier_tag, Parent>&& m,
      A&& a)
    {
      constexpr bool forbidden = Parent::upper_call_limit == 0U;
      static_assert(!forbidden,
                    "SIDE_EFFECT for forbidden call does not make sense");
      m.matcher->add_side_effect(std::forward<A>(a));
      return {std::move(m.matcher)};
    }
  };

  struct handle_return
  {
    template <typename Matcher, typename modifier_tag, typename Parent, typename H>
    static
    call_modifier<Matcher, modifier_tag, return_injector<return_of_t<typename Parent::signature>, Parent>>
    action(call_modifier<Matcher, modifier_tag, Parent>&& m,
           H&& h)
    {
      using signature = typename Parent::signature;
      using return_type = typename Parent::return_type;
      using co_return_type = typename Parent::co_return_type;
      using params_type = call_params_type_t<signature>&;
      using sigret = return_of_t<signature>;
      using ret = decltype(std::declval<H>()(std::declval<params_type>()));
      // don't know why MS VS 2015 RC doesn't like std::result_of

      constexpr bool is_coroutine      = trompeloeil::is_coroutine<sigret>::value;
      constexpr bool is_illegal_type   = std::is_same<detail::decay_t<ret>, illegal_argument>::value;
      constexpr bool has_co_return     = !std::is_same<co_return_type, void>::value;
      constexpr bool is_first_return   = std::is_same<return_type, void>::value;
      constexpr bool void_signature    = std::is_same<sigret, void>::value;
      constexpr bool is_pointer_sigret = std::is_pointer<sigret>::value;
      constexpr bool is_pointer_ret    = std::is_pointer<detail::decay_t<ret>>::value;
      constexpr bool ptr_const_mismatch =
        is_pointer_ret &&
        is_pointer_sigret &&
        !std::is_const<detail::remove_pointer_t<sigret>>{} &&
        std::is_const<detail::remove_pointer_t<detail::decay_t<ret>>>{};
      constexpr bool is_ref_sigret     = std::is_reference<sigret>::value;
      constexpr bool is_ref_ret        = std::is_reference<ret>::value;
      constexpr bool ref_const_mismatch=
        is_ref_ret &&
        is_ref_sigret &&
        !std::is_const<detail::remove_reference_t<sigret>>::value &&
        std::is_const<detail::remove_reference_t<ret>>::value;
      constexpr bool matching_ret_type = std::is_constructible<sigret, ret>::value;
      constexpr bool ref_value_mismatch = !is_ref_ret && is_ref_sigret;

      static_assert(!has_co_return,
                    "RETURN and CO_RETURN cannot be combined");
      static_assert(!is_coroutine,
                    "Do not use RETURN from a coroutine, use CO_RETURN");
      static_assert(is_coroutine || matching_ret_type || !void_signature,
                    "RETURN does not make sense for void-function");
      static_assert(is_coroutine || !is_illegal_type,
                    "RETURN illegal argument");
      static_assert(is_coroutine || !ptr_const_mismatch,
                    "RETURN const* from function returning pointer to non-const");
      static_assert(is_coroutine || !ref_value_mismatch || matching_ret_type,
                    "RETURN non-reference from function returning reference");
      static_assert(is_coroutine || ref_value_mismatch || !ref_const_mismatch,
                    "RETURN const& from function returning non-const reference");

      static_assert(is_coroutine || ptr_const_mismatch || ref_const_mismatch || is_illegal_type || matching_ret_type || void_signature,
                    "RETURN value is not convertible to the return type of the function");
      static_assert(is_coroutine || is_first_return,
                    "Multiple RETURN does not make sense");
      static_assert(is_coroutine || !Parent::throws || Parent::upper_call_limit == 0,
                    "THROW and RETURN does not make sense");
      static_assert(is_coroutine || Parent::upper_call_limit > 0,
                    "RETURN for forbidden call does not make sense");

      constexpr bool valid = !is_coroutine && !is_illegal_type && matching_ret_type && is_first_return && !Parent::throws && Parent::upper_call_limit > 0;
      using tag = std::integral_constant<bool, valid>;
      m.matcher->set_return(tag{}, std::forward<H>(h));
      return {std::move(m).matcher};
    }
  };

  struct handle_throw
  {
    template <typename Matcher, typename modifier_tag, typename Parent, typename H>
    static
    call_modifier<Matcher, modifier_tag, throw_injector<Parent>>
    action(call_modifier<Matcher, modifier_tag, Parent>&& m,
           H&& h)
    {
      using signature = typename Parent::signature;
      constexpr bool is_coroutine      = trompeloeil::is_coroutine<return_of_t<signature>>::value;

      static_assert(!is_coroutine,
                    "Do not use THROW from a coroutine, use CO_THROW");
      static_assert(is_coroutine || !Parent::throws,
                    "Multiple THROW does not make sense");
      constexpr bool has_return = !std::is_same<typename Parent::return_type, void>::value;
      static_assert(!has_return,
                    "THROW and RETURN does not make sense");

      constexpr bool forbidden = Parent::upper_call_limit == 0U;

      static_assert(is_coroutine || !forbidden,
                    "THROW for forbidden call does not make sense");

      constexpr bool valid = !is_coroutine && !Parent::throws && !has_return;
      using tag = std::integral_constant<bool, valid>;
      auto handler = throw_handler_t<H, signature>(std::forward<H>(h));
      m.matcher->set_return(tag{}, std::move(handler));
      return {std::move(m).matcher};
    }
  };

  struct times
  {
    template <
      typename Matcher, typename modifier_tag, typename Parent,
      size_t L,
      size_t H,
      bool times_set = Parent::call_limit_set>
    static
    call_modifier<Matcher, modifier_tag, call_limit_injector<Parent, H>>
    action(call_modifier<Matcher, modifier_tag, Parent>&& m,
           multiplicity<L, H>)
    {
      static_assert(!times_set,
                    "Only one TIMES call limit is allowed, but it can express an interval");

      static_assert(H >= L,
                    "In TIMES the first value must not exceed the second");

      static_assert(H > 0 || !Parent::throws,
                    "THROW and TIMES(0) does not make sense");

      static_assert(H > 0 || std::is_same<typename Parent::return_type, void>::value,
                    "RETURN and TIMES(0) does not make sense");

      static_assert(H > 0 || !Parent::side_effects,
                    "SIDE_EFFECT and TIMES(0) does not make sense");

      static_assert(H > 0 || !Parent::sequence_set,
                    "IN_SEQUENCE and TIMES(0) does not make sense");

      m.matcher->sequences->set_limits(L, H);
      return {std::move(m).matcher};
    }
  };

  inline
  void
  report_unfulfilled(
    const char* reason,
    char const        *name,
    std::string const &values,
    size_t min_calls,
    size_t call_count,
    location           loc)
  {
    std::ostringstream os;
    os << reason
       << ":\nExpected " << name << " to be called ";
    if (min_calls == 1)
      os << "once";
    else
      os << min_calls << " times";
    os << ", actually ";
    switch (call_count)
    {
    case 0:
      os << "never called\n"; break;
    case 1:
      os << "called once\n"; break;
    default:
      os << "called " << call_count << " times\n";
    }
    os << values;
    send_report<specialized>(severity::nonfatal, loc, os.str());
  }

  inline
  void
  report_forbidden_call(
    char const *name,
    location loc,
    std::string const& values)
  {
    std::ostringstream os;
    os << "Match of forbidden call of " << name
       << " at " << loc << '\n' << values;
    send_report<specialized>(severity::fatal, loc, os.str());
  }

  template <typename Sig>
  struct matcher_info
  {
    using signature = Sig;
    using co_return_type = void;
    using return_type = void;
    static size_t const upper_call_limit = 1;
    static bool const throws = false;
    static bool const call_limit_set = false;
    static bool const sequence_set = false;
    static bool const side_effects = false;
  };


  template <typename Sig, typename Value>
  struct call_matcher : public call_matcher_base<Sig>, expectation
  {
    using call_matcher_base<Sig>::name;
    using call_matcher_base<Sig>::loc;

#if TROMPELOEIL_GCC && TROMPELOEIL_GCC_VERSION >= 70000 && TROMPELOEIL_GCC_VERSION < 80000
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic push
#endif
    template <typename ... U>
    call_matcher(
      char const *file,
      unsigned long line,
      char const *call_string,
      U &&... u)
    : call_matcher_base<Sig>(location{file, line}, call_string)
    , val(std::forward<U>(u)...)
    {}
#if TROMPELOEIL_GCC && TROMPELOEIL_GCC_VERSION >= 70000 && TROMPELOEIL_GCC_VERSION < 80000
#pragma GCC diagnostic pop
#endif

    call_matcher(call_matcher &&r) = delete;

    ~call_matcher() override
    {
      auto lock = get_lock();
      if (is_unfulfilled())
      {
        report_missed("Unfulfilled expectation");
      }
      this->unlink();
    }

    bool
    is_satisfied()
      const
      noexcept
      override
    {
      auto lock = get_lock();
      return sequences->is_satisfied();
    }

    bool
    is_saturated()
      const
      noexcept
      override
    {
      auto lock = get_lock();
      return sequences->is_saturated();
    }
    bool
    is_unfulfilled()
    const
    noexcept
    {
      return !reported && this->is_linked() && !sequences->is_satisfied();
    }

    void
    mock_destroyed()
    override
    {
      if (is_unfulfilled())
      {
        report_missed("Pending expectation on destroyed mock object");
      }
    }

    call_matcher*
    hook_last(
      call_matcher_list<Sig> &list)
    noexcept
    {
      list.push_front(this);
      return this;
    }

    bool
    matches(
      call_params_type_t<Sig> const& params)
    const
    override
    {
      return match_parameters(val, params) && match_conditions(params);
    }

    bool
    match_conditions(
      call_params_type_t<Sig> const & params)
    const
    {
      // std::all_of() is almost always preferable. The only reason
      // for using a hand rolled loop is because it cuts compilation
      // times quite noticeably (almost 10% with g++5.1)
      for (auto& c : conditions)
      {
        if (!c.check(params)) return false;
      }
      return true;
    }

    unsigned
    sequence_cost()
      const
      noexcept
      override
    {
      return sequences->order();
    }

    return_of_t<Sig>
    return_value(
      trace_agent& agent,
      call_params_type_t<Sig>& params)
    override
    {
      if (!return_handler_obj) return default_return<return_of_t<Sig>>();
      return return_handler_obj->call(agent, params);
    }

    void
    run_actions(
      call_params_type_t<Sig>& params,
      call_matcher_list<Sig> &saturated_list)
    override
    {
      if (sequences->is_forbidden())
      {
        reported = true;
        report_forbidden_call(name, loc, params_string(params));
      }
      auto lock = get_lock();
      {
        if (!sequences->can_be_called())
        {
          sequences->validate(severity::fatal, name, loc);
        }
        sequences->increment_call();
        if (sequences->is_satisfied())
        {
          sequences->retire_predecessors();
        }
        if (sequences->is_saturated())
        {
          sequences->retire();
          this->unlink();
          saturated_list.push_back(this);
        }
      }
      for (auto& a : actions) a.action(params);
    }

    std::ostream&
    report_signature(
      std::ostream& os)
    const override
    {
      return os << name << " at " << loc;
    }

    std::ostream&
    report_mismatch(
      std::ostream& os,
      call_params_type_t<Sig> const & params)
    override
    {
      reported = true;
      report_signature(os);
      if (match_parameters(val, params))
      {
        for (auto& cond : conditions)
        {
          if (!cond.check(params))
          {
            os << "\n  Failed WITH(" << cond.name() << ')';
          }
        }
      }
      else
      {
        os << '\n';
        ::trompeloeil::print_mismatch(os, val, params);
      }
      return os;
    }

    void
    report_missed(
      char const *reason)
    noexcept
    {
      reported = true;
      report_unfulfilled(
        reason,
        name,
        params_string(val),
        sequences->get_min_calls(),
        sequences->get_calls(),
        loc);
    }

    template <typename C>
    void
    add_condition(
      char const *str,
      C&& c)
    {
      auto cond = new condition<Sig, C>(str, std::forward<C>(c));
      conditions.push_back(cond);
    }

    template <typename S>
    void
    add_side_effect(
      S&& s)
    {
      auto effect = new side_effect<Sig, S>(std::forward<S>(s));
      actions.push_back(effect);
    }

    template <typename ... T>
    void
    set_sequence(
      T&& ... t)
    {
      using handler = sequence_handler<sizeof...(T)>;
      auto seq = detail::make_unique<handler>(*sequences,
                                              name,
                                              loc,
                                              std::forward<T>(t)...);
      sequences = std::move(seq);
    }
    template <typename T>
    inline
    void
    set_return(
      std::true_type,
      T&& h)
    {
      using basic_t = typename std::remove_reference<T>::type;
      using handler = return_handler_t<Sig, basic_t>;
      return_handler_obj.reset(new handler(std::forward<T>(h)));
    }

    template <typename T>            // Never called. Used to limit errmsg
    static                           // with RETURN of wrong type and after:
    void                             //   FORBIDDEN_CALL
    set_return(std::false_type, T&&t)//   RETURN
      noexcept;                      //   THROW


    condition_list<Sig>                    conditions;
    side_effect_list<Sig>                  actions;
    std::shared_ptr<yield_expr_list<Sig>>  yield_expressions;

    std::unique_ptr<return_handler<Sig>>   return_handler_obj;
    std::unique_ptr<sequence_handler_base> sequences = detail::make_unique<sequence_handler<0>>();
    Value                                  val;
    bool                                   reported = false;
  };

  /* Clang (all versions) does not like computing the return type R
   * before determining if the function overload is the best match.
   */
  template <
    int N,
    typename T,
    typename = detail::enable_if_t<N <= std::tuple_size<T>::value>,
    typename R = decltype(std::get<N-1>(std::declval<T>()).get())
  >
  constexpr
  TROMPELOEIL_DECLTYPE_AUTO
  arg(
    T* t,
    std::true_type)
  TROMPELOEIL_TRAILING_RETURN_TYPE(R)
  {
    return std::get<N-1>(*t).get();
  }

  template <int>
  constexpr
  illegal_argument
  arg(
    void const*,
    std::false_type)
  noexcept
  {
    return {};
  }

  template <
    int N,
    typename T,
    typename R = decltype(arg<N>(std::declval<T*>(),
                                 std::integral_constant<bool, (N <= std::tuple_size<T>::value)>{}))
  >
  TROMPELOEIL_DECLTYPE_AUTO
  mkarg(
    T& t)
  noexcept
  TROMPELOEIL_TRAILING_RETURN_TYPE(R)
  {
    return arg<N>(&t, std::integral_constant<bool, (N <= std::tuple_size<T>::value)>{});
  }

  template <typename Mock>
  struct call_validator_t
  {
    template <typename M, typename Tag, typename Info>
    auto
    make_expectation(
      std::true_type,
      call_modifier<M, Tag, Info>&& m)
    const
    noexcept
    TROMPELOEIL_TRAILING_RETURN_TYPE(std::unique_ptr<expectation>)
    {
      auto lock = get_lock();
      m.matcher->hook_last(obj.trompeloeil_matcher_list(static_cast<Tag*>(nullptr)));

      return std::move(m).matcher;
    }

    template <typename T>
    static                                           // Never called. Used to
    std::unique_ptr<expectation>                     // limit errmsg when RETURN
    make_expectation(std::false_type, T&&) noexcept; // is missing in non-void
                                                     // function

    template <typename M, typename Tag, typename Info>
    inline
    auto
    operator+(
      call_modifier<M, Tag, Info>&& t)
    const
    TROMPELOEIL_TRAILING_RETURN_TYPE(std::unique_ptr<expectation>)
    {
      using call = call_modifier<M, Tag, Info>;
      using sigret = return_of_t<typename call::signature>;
      using ret = typename call::return_type;
      using coret = typename call::co_return_type;
      constexpr bool is_coroutine = trompeloeil::is_coroutine<sigret>::value;
      constexpr bool retmatch = std::is_same<ret, sigret>::value;
      constexpr bool coretmatch = std::is_same<coret, sigret>::value;
      constexpr bool forbidden = call::upper_call_limit == 0;
      constexpr bool valid_return_type = retmatch || coretmatch || forbidden;
      constexpr bool handles_return = is_coroutine || valid_return_type || call::throws;
      constexpr bool handles_co_return = !is_coroutine || valid_return_type || call::throws;
      static_assert(handles_return, "RETURN missing for non-void function");
      static_assert(handles_co_return, "CO_RETURN missing for coroutine");
      auto tag = std::integral_constant<bool, handles_return || handles_co_return>{};
      return make_expectation(tag, std::move(t));
    }
    Mock& obj;
  };

  template <typename T,
            typename = detail::enable_if_t<std::is_lvalue_reference<T&&>::value>>
  inline
  T&&
  decay_return_type(
    T&& t)
  {
    return std::forward<T>(t);
  }

  template <typename T,
            typename = detail::enable_if_t<std::is_rvalue_reference<T&&>::value>>
  inline
  T
  decay_return_type(
    T&& t)
  {
    return std::forward<T>(t);
  }

  template <typename T, size_t N>
  inline
  T*
  decay_return_type(
    T (&t)[N])
  {
    return t;
  }

  inline
  void
  decay_return_type()
  {
  }


  template <bool movable, typename Sig>
  struct expectations
  {
    expectations() = default;
    expectations(expectations&&) = default;
    ~expectations() {
      active.decommission();
      saturated.decommission();
    }
    call_matcher_list<Sig> active{};
    call_matcher_list<Sig> saturated{};
  };

  template <typename Sig>
  struct expectations<false, Sig>
  {
    expectations() = default;
    expectations(expectations&&)
    noexcept
    {
      static_assert(std::is_same<Sig,void>::value,
        "By default, mock objects are not movable. "
        "To make a mock object movable, see: "
        "https://github.com/rollbear/trompeloeil/blob/master/docs/reference.md#movable_mock");
    }
    ~expectations() {
      active.decommission();
      saturated.decommission();
    }
    call_matcher_list<Sig> active{};
    call_matcher_list<Sig> saturated{};
  };

  template <typename Sig, typename ... P>
  return_of_t<Sig> mock_func(std::false_type, P&& ...);


  template <bool movable, typename Sig, typename ... P>
  return_of_t<Sig>
  mock_func(std::true_type,
            expectations<movable, Sig>& e,
            char const *func_name,
            char const *sig_name,
            P&& ... p)
  {
    auto lock = get_lock();

    call_params_type_t<void(P...)> param_value(std::forward<P>(p)...);

    auto i = find(e.active, param_value);
    if (!i)
    {
      report_mismatch(e.active,
                      e.saturated,
                      func_name + std::string(" with signature ") + sig_name,
                      param_value);
    }
    else{
        report_match(e.active);
    }
    trace_agent ta{i->loc, i->name, tracer_obj()};
    try
    {
      ta.trace_params(param_value);
      i->run_actions(param_value, e.saturated);
      return i->return_value(ta, param_value);
    }
    catch (...)
    {
      ta.trace_exception();
      throw;
    }
  }

  template <typename ... U>
  struct param_helper {
    using type = decltype(std::make_tuple(std::declval<U>()...));
  };

  template <typename ... U>
  using param_t = typename param_helper<U...>::type;

  template <typename sig, typename tag, typename... U>
  using modifier_t = call_modifier<call_matcher<sig, param_t<U...>>,
                                   tag,
                                   matcher_info<sig>>;

  template <typename T>
  struct mock_interface : public T
  {
    using trompeloeil_interface_name = T;
    using T::T;
 };

#if defined(TROMPELOEIL_USER_DEFINED_COMPILE_TIME_REPORTER)
 extern template struct reporter<specialized>;
#endif // TROMPELOEIL_USER_DEFINED_COMPILE_TIME_REPORTER
}

#define TROMPELOEIL_LINE_ID(name)                                        \
  TROMPELOEIL_CONCAT(trompeloeil_l_ ## name ## _, __LINE__)

#define TROMPELOEIL_COUNT_ID(name)                                       \
  TROMPELOEIL_CONCAT(trompeloeil_c_ ## name ## _, __COUNTER__)

#if TROMPELOEIL_MSVC
#define TROMPELOEIL_MAKE_MOCK0(name, sig, ...)                           \
  TROMPELOEIL_MAKE_MOCK_(name,,,0, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK1(name, sig, ...)                           \
  TROMPELOEIL_MAKE_MOCK_(name,,,1, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK2(name, sig, ...)                           \
  TROMPELOEIL_MAKE_MOCK_(name,,,2, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK3(name, sig, ...)                           \
  TROMPELOEIL_MAKE_MOCK_(name,,,3, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK4(name, sig, ...)                           \
  TROMPELOEIL_MAKE_MOCK_(name,,,4, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK5(name, sig, ...)                           \
  TROMPELOEIL_MAKE_MOCK_(name,,,5, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK6(name, sig, ...)                           \
  TROMPELOEIL_MAKE_MOCK_(name,,,6, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK7(name, sig, ...)                           \
  TROMPELOEIL_MAKE_MOCK_(name,,,7, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK8(name, sig, ...)                           \
  TROMPELOEIL_MAKE_MOCK_(name,,,8, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK9(name, sig, ...)                           \
  TROMPELOEIL_MAKE_MOCK_(name,,,9, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK10(name, sig, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,,,10, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK11(name, sig, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,,,11, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK12(name, sig, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,,,12, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK13(name, sig, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,,,13, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK14(name, sig, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,,,14, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK15(name, sig, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,,,15, sig, __VA_ARGS__,,)

#define TROMPELOEIL_MAKE_CONST_MOCK0(name, sig, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,const,,0, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK1(name, sig, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,const,,1, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK2(name, sig, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,const,,2, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK3(name, sig, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,const,,3, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK4(name, sig, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,const,,4, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK5(name, sig, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,const,,5, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK6(name, sig, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,const,,6, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK7(name, sig, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,const,,7, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK8(name, sig, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,const,,8, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK9(name, sig, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,const,,9, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK10(name, sig, ...)                    \
  TROMPELOEIL_MAKE_MOCK_(name,const,,10, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK11(name, sig, ...)                    \
  TROMPELOEIL_MAKE_MOCK_(name,const,,11, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK12(name, sig, ...)                    \
  TROMPELOEIL_MAKE_MOCK_(name,const,,12, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK13(name, sig, ...)                    \
  TROMPELOEIL_MAKE_MOCK_(name,const,,13, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK14(name, sig, ...)                    \
  TROMPELOEIL_MAKE_MOCK_(name,const,,14, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK15(name, sig, ...)                    \
  TROMPELOEIL_MAKE_MOCK_(name,const,,15, sig, __VA_ARGS__,,)

#ifdef STDMETHODCALLTYPE
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK0(name, sig, ...)                 \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,0, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK1(name, sig, ...)                 \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,1, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK2(name, sig, ...)                 \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,2, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK3(name, sig, ...)                 \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,3, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK4(name, sig, ...)                 \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,4, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK5(name, sig, ...)                 \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,5, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK6(name, sig, ...)                 \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,6, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK7(name, sig, ...)                 \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,7, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK8(name, sig, ...)                 \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,8, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK9(name, sig, ...)                 \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,9, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK10(name, sig, ...)                \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,10, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK11(name, sig, ...)                \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,11, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK12(name, sig, ...)                \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,12, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK13(name, sig, ...)                \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,13, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK14(name, sig, ...)                \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,14, sig, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK15(name, sig, ...)                \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,15, sig, __VA_ARGS__,,)
#endif

#else
// sane standards compliant preprocessor

#define TROMPELOEIL_MAKE_MOCK0(name, ...)                                \
  TROMPELOEIL_MAKE_MOCK_(name,,,0, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK1(name, ...)                                \
  TROMPELOEIL_MAKE_MOCK_(name,,,1, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK2(name, ...)                                \
  TROMPELOEIL_MAKE_MOCK_(name,,,2, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK3(name, ...)                                \
  TROMPELOEIL_MAKE_MOCK_(name,,,3, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK4(name, ...)                                \
  TROMPELOEIL_MAKE_MOCK_(name,,,4, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK5(name, ...)                                \
  TROMPELOEIL_MAKE_MOCK_(name,,,5, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK6(name, ...)                                \
  TROMPELOEIL_MAKE_MOCK_(name,,,6, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK7(name, ...)                                \
  TROMPELOEIL_MAKE_MOCK_(name,,,7, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK8(name, ...)                                \
  TROMPELOEIL_MAKE_MOCK_(name,,,8, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK9(name, ...)                                \
  TROMPELOEIL_MAKE_MOCK_(name,,,9, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK10(name, ...)                               \
  TROMPELOEIL_MAKE_MOCK_(name,,,10, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK11(name, ...)                               \
  TROMPELOEIL_MAKE_MOCK_(name,,,11, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK12(name, ...)                               \
  TROMPELOEIL_MAKE_MOCK_(name,,,12, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK13(name, ...)                               \
  TROMPELOEIL_MAKE_MOCK_(name,,,13, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK14(name, ...)                               \
  TROMPELOEIL_MAKE_MOCK_(name,,,14,__VA_ARGS__,,)
#define TROMPELOEIL_MAKE_MOCK15(name, ...)                               \
  TROMPELOEIL_MAKE_MOCK_(name,,,15, __VA_ARGS__,,)

#define TROMPELOEIL_MAKE_CONST_MOCK0(name, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,const,,0, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK1(name, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,const,,1, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK2(name, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,const,,2, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK3(name, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,const,,3, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK4(name, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,const,,4, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK5(name, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,const,,5, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK6(name, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,const,,6, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK7(name, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,const,,7, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK8(name, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,const,,8, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK9(name, ...)                          \
  TROMPELOEIL_MAKE_MOCK_(name,const,,9, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK10(name, ...)                         \
  TROMPELOEIL_MAKE_MOCK_(name,const,,10, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK11(name, ...)                         \
  TROMPELOEIL_MAKE_MOCK_(name,const,,11, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK12(name, ...)                         \
  TROMPELOEIL_MAKE_MOCK_(name,const,,12, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK13(name, ...)                         \
  TROMPELOEIL_MAKE_MOCK_(name,const,,13, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK14(name, ...)                         \
  TROMPELOEIL_MAKE_MOCK_(name,const,,14, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_CONST_MOCK15(name, ...)                         \
  TROMPELOEIL_MAKE_MOCK_(name,const,,15, __VA_ARGS__,,)

#ifdef STDMETHODCALLTYPE
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK0(name, ...)                      \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,0, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK1(name, ...)                      \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,1, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK2(name, ...)                      \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,2, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK3(name, ...)                      \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,3, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK4(name, ...)                      \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,4, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK5(name, ...)                      \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,5, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK6(name, ...)                      \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,6, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK7(name, ...)                      \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,7, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK8(name, ...)                      \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,8, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK9(name, ...)                      \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,9, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK10(name, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,10, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK11(name, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,11, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK12(name, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,12, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK13(name, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,13, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK14(name, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,14, __VA_ARGS__,,)
#define TROMPELOEIL_MAKE_STDMETHOD_MOCK15(name, ...)                     \
  TROMPELOEIL_MAKE_MOCK_(name,,STDMETHODCALLTYPE,15, __VA_ARGS__,,)
#endif

#endif

#define TROMPELOEIL_IMPLEMENT_MOCK0(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(0, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK1(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(1, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK2(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(2, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK3(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(3, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK4(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(4, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK5(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(5, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK6(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(6, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK7(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(7, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK8(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(8, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK9(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(9, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK10(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(10, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK11(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(11, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK12(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(12, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK13(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(13, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK14(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(14, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_MOCK15(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_MOCK_(15, __VA_ARGS__,override))

#define TROMPELOEIL_IMPLEMENT_CONST_MOCK0(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(0, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK1(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(1, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK2(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(2, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK3(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(3, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK4(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(4, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK5(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(5, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK6(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(6, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK7(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(7, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK8(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(8, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK9(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(9, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK10(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(10, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK11(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(11, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK12(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(12, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK13(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(13, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK14(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(14, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK15(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_CONST_MOCK_(15, __VA_ARGS__,override))


#define TROMPELOEIL_IMPLEMENT_MOCK_(num, name, ...) \
  TROMPELOEIL_MAKE_MOCK_(name,\
                         ,\
                         ,\
                         num,\
                         decltype(::trompeloeil::nonconst_member_signature(&trompeloeil_interface_name::name))::type,\
                         TROMPELOEIL_SEPARATE(__VA_ARGS__),)
#define TROMPELOEIL_IMPLEMENT_CONST_MOCK_(num, name, ...) \
  TROMPELOEIL_MAKE_MOCK_(name,\
                         const,\
                         ,\
                         num,\
                         decltype(::trompeloeil::const_member_signature(&trompeloeil_interface_name::name))::type,\
                         TROMPELOEIL_SEPARATE(__VA_ARGS__),)
#ifdef STDMETHODCALLTYPE
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK0(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(0, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK1(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(1, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK2(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(2, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK3(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(3, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK4(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(4, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK5(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(5, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK6(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(6, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK7(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(7, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK8(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(8, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK9(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(9, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK10(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(10, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK11(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(11, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK12(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(12, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK13(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(13, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK14(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(14, __VA_ARGS__,override))
#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK15(...) \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(15, __VA_ARGS__,override))

#define TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK_(num, name, ...) \
  TROMPELOEIL_MAKE_MOCK_(name,\
                         ,\
                         STDMETHODCALLTYPE,\
                         num,\
                         decltype(::trompeloeil::stdcall_member_signature(&trompeloeil_interface_name::name))::type,\
                         TROMPELOEIL_SEPARATE(__VA_ARGS__),)
#endif

#define TROMPELOEIL_MAKE_MOCK_(name, constness, callconv, num, sig, spec, ...) \
  private:                                                                     \
  using TROMPELOEIL_LINE_ID(cardinality_match) =                               \
    std::integral_constant<bool, num ==                                        \
      ::trompeloeil::param_list<TROMPELOEIL_REMOVE_PAREN(sig)>::size>;         \
  static_assert(TROMPELOEIL_LINE_ID(cardinality_match)::value,                 \
                "Function signature does not have " #num " parameters");       \
  using TROMPELOEIL_LINE_ID(matcher_list_t) =                                  \
  ::trompeloeil::call_matcher_list<TROMPELOEIL_REMOVE_PAREN(sig)>;             \
  using TROMPELOEIL_LINE_ID(expectation_list_t) =                              \
    ::trompeloeil::expectations<trompeloeil_movable_mock,                      \
                                TROMPELOEIL_REMOVE_PAREN(sig)>;                \
                                                                               \
  struct TROMPELOEIL_LINE_ID(tag_type_trompeloeil)                             \
  {                                                                            \
    const char* trompeloeil_expectation_file;                                  \
    unsigned long trompeloeil_expectation_line;                                \
    const char *trompeloeil_expectation_string;                                \
                                                                               \
    /* Work around parsing bug in VS 2015 when a "complex" */                  \
    /* decltype() appears in a trailing return type. */                        \
    /* Further, work around C2066 defect in VS 2017 15.7.1. */                 \
    using trompeloeil_sig_t = typename                                         \
    ::trompeloeil::identity_type<TROMPELOEIL_REMOVE_PAREN(sig)>::type;         \
                                                                               \
    using trompeloeil_call_params_type_t =                                     \
      ::trompeloeil::call_params_type_t<TROMPELOEIL_REMOVE_PAREN(sig)>;        \
                                                                               \
    using trompeloeil_return_of_t =                                            \
      ::trompeloeil::return_of_t<TROMPELOEIL_REMOVE_PAREN(sig)>;               \
                                                                               \
    template <typename ... trompeloeil_param_type>                             \
    auto name(                                                                 \
      trompeloeil_param_type&& ... trompeloeil_param)                          \
      -> ::trompeloeil::modifier_t<trompeloeil_sig_t,                          \
                                   TROMPELOEIL_LINE_ID(tag_type_trompeloeil),  \
                                   trompeloeil_param_type...>                  \
    {                                                                          \
      using matcher = ::trompeloeil::call_matcher<                             \
                          TROMPELOEIL_REMOVE_PAREN(sig),                       \
                          ::trompeloeil::param_t<trompeloeil_param_type...>>;  \
      return {                                                                 \
          ::trompeloeil::detail::make_unique<matcher>(                         \
                trompeloeil_expectation_file,                                  \
                trompeloeil_expectation_line,                                  \
                trompeloeil_expectation_string,                                \
                std::forward<trompeloeil_param_type>(trompeloeil_param)...     \
              )                                                                \
      };                                                                       \
    }                                                                          \
  };                                                                           \
                                                                               \
  public:                                                                      \
  TROMPELOEIL_LINE_ID(matcher_list_t)&                                         \
  trompeloeil_matcher_list(                                                    \
    TROMPELOEIL_LINE_ID(tag_type_trompeloeil)*)                                \
  constness                                                                    \
  noexcept                                                                     \
  {                                                                            \
    return TROMPELOEIL_LINE_ID(expectations).active;                           \
  }                                                                            \
                                                                               \
  ::trompeloeil::return_of_t<TROMPELOEIL_REMOVE_PAREN(sig)>                    \
  callconv                                                                     \
  name(TROMPELOEIL_PARAM_LIST(num, sig))                                       \
  constness                                                                    \
  spec                                                                         \
  {                                                                            \
    return ::trompeloeil::mock_func<trompeloeil_movable_mock, TROMPELOEIL_REMOVE_PAREN(sig)>( \
                                    TROMPELOEIL_LINE_ID(cardinality_match){},  \
                                    TROMPELOEIL_LINE_ID(expectations),         \
#name,                                     \
#sig                                       \
                                    TROMPELOEIL_PARAMS(num));                  \
  }                                                                            \
                                                                               \
  TROMPELOEIL_NOT_IMPLEMENTED(                                                 \
  auto                                                                         \
  trompeloeil_self_ ## name(TROMPELOEIL_PARAM_LIST(num, sig)) constness        \
  -> decltype(*this));                                                         \
                                                                               \
  TROMPELOEIL_NOT_IMPLEMENTED(TROMPELOEIL_LINE_ID(tag_type_trompeloeil)        \
  trompeloeil_tag_ ## name(TROMPELOEIL_PARAM_LIST(num, sig)) constness);       \
                                                                               \
  private:                                                                     \
  mutable                                                                      \
  TROMPELOEIL_LINE_ID(expectation_list_t) TROMPELOEIL_LINE_ID(expectations){}; \
                                                                               \
  public:                                                                      \
  /* An unused alias declaration to make trailing semicolon acceptable. */     \
  using TROMPELOEIL_LINE_ID(unused_alias) = void


#define TROMPELOEIL_LPAREN (

#define TROMPELOEIL_MORE_THAN_TWO_ARGS(...)                                    \
  TROMPELOEIL_IDENTITY(                                                        \
    TROMPELOEIL_ARG16(__VA_ARGS__,                                             \
                      T, T, T, T, T, T, T, T, T, T, T, T, T, F, F, F))


#define TROMPELOEIL_REQUIRE_CALL_V(...)                                        \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_REQUIRE_CALL_IMPL TROMPELOEIL_LPAREN        \
      TROMPELOEIL_MORE_THAN_TWO_ARGS(__VA_ARGS__), __VA_ARGS__))

// Dispatch to _F (0, 1, or 2 arguments) or _T (> 2 arguments) macro
#define TROMPELOEIL_REQUIRE_CALL_IMPL(N, ...)                                  \
    TROMPELOEIL_IDENTITY(                                                      \
      TROMPELOEIL_REQUIRE_CALL_ ## N TROMPELOEIL_LPAREN __VA_ARGS__))

// Accept only two arguments
#define TROMPELOEIL_REQUIRE_CALL_F(obj, func)                                  \
  auto TROMPELOEIL_COUNT_ID(call_obj) =                                        \
    TROMPELOEIL_REQUIRE_CALL_V_LAMBDA(obj, func, #obj, #func, .null_modifier())

// Accept three or more arguments.
#define TROMPELOEIL_REQUIRE_CALL_T(obj, func, ...)                             \
  auto TROMPELOEIL_COUNT_ID(call_obj) =                                        \
    TROMPELOEIL_REQUIRE_CALL_V_LAMBDA(obj, func, #obj, #func, __VA_ARGS__)

#if TROMPELOEIL_GCC
// This is highly unfortunate. Conversion warnings are desired here, but there
// are situations when the wildcard _ uses conversion when creating an
// expectation. It would be much better if the warning could be suppressed only
// for that type. See https://github.com/rollbear/trompeloeil/issues/293
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic push
#endif
#define TROMPELOEIL_REQUIRE_CALL_V_LAMBDA(obj, func, obj_s, func_s, ...)       \
  [&]                                                                          \
  {                                                                            \
    using trompeloeil_s_t = decltype((obj)                                     \
                              .TROMPELOEIL_CONCAT(trompeloeil_self_, func));   \
    using trompeloeil_e_t = decltype((obj)                                     \
                              .TROMPELOEIL_CONCAT(trompeloeil_tag_,func));     \
                                                                               \
    return TROMPELOEIL_REQUIRE_CALL_LAMBDA_OBJ(obj, func, obj_s, func_s)       \
      __VA_ARGS__                                                              \
      ;                                                                        \
  }()
#if TROMPELOEIL_GCC
#pragma GCC diagnostic pop
#endif
#define TROMPELOEIL_REQUIRE_CALL_LAMBDA_OBJ(obj, func, obj_s, func_s)          \
  ::trompeloeil::call_validator_t<trompeloeil_s_t>{(obj)} +                    \
      ::trompeloeil::detail::conditional_t<false,                              \
                                           decltype((obj).func),               \
                                           trompeloeil_e_t>                    \
    {__FILE__, static_cast<unsigned long>(__LINE__), obj_s "." func_s}.func

#define TROMPELOEIL_NAMED_REQUIRE_CALL_V(...)                                  \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_NAMED_REQUIRE_CALL_IMPL TROMPELOEIL_LPAREN  \
    TROMPELOEIL_MORE_THAN_TWO_ARGS(__VA_ARGS__), __VA_ARGS__))

// Dispatch to _F (0, 1, or 2 arguments) or _T (> 2 arguments) macro
#define TROMPELOEIL_NAMED_REQUIRE_CALL_IMPL(N, ...)                            \
  TROMPELOEIL_IDENTITY(                                                        \
    TROMPELOEIL_NAMED_REQUIRE_CALL_ ## N TROMPELOEIL_LPAREN __VA_ARGS__))

// Accept only two arguments
#define TROMPELOEIL_NAMED_REQUIRE_CALL_F(obj, func)                            \
  TROMPELOEIL_REQUIRE_CALL_V_LAMBDA(obj, func, #obj, #func, .null_modifier())

// Accept three or more arguments.
#define TROMPELOEIL_NAMED_REQUIRE_CALL_T(obj, func, ...)                       \
  TROMPELOEIL_REQUIRE_CALL_V_LAMBDA(obj, func, #obj, #func, __VA_ARGS__)


#define TROMPELOEIL_ALLOW_CALL_V(...)                                          \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_ALLOW_CALL_IMPL TROMPELOEIL_LPAREN          \
    TROMPELOEIL_MORE_THAN_TWO_ARGS(__VA_ARGS__), __VA_ARGS__))

// Dispatch to _F (0, 1, or 2 arguments) or _T (> 2 arguments) macro
#define TROMPELOEIL_ALLOW_CALL_IMPL(N, ...)                                    \
  TROMPELOEIL_IDENTITY(                                                        \
    TROMPELOEIL_ALLOW_CALL_ ## N TROMPELOEIL_LPAREN __VA_ARGS__))

// Accept only two arguments
#define TROMPELOEIL_ALLOW_CALL_F(obj, func)                                    \
  TROMPELOEIL_REQUIRE_CALL_T(obj, func, .TROMPELOEIL_INFINITY_TIMES())

// Accept three or more arguments.
#define TROMPELOEIL_ALLOW_CALL_T(obj, func, ...)                               \
  TROMPELOEIL_REQUIRE_CALL_T(obj,                                              \
                             func,                                             \
                             .TROMPELOEIL_INFINITY_TIMES() __VA_ARGS__)


#define TROMPELOEIL_NAMED_ALLOW_CALL_V(...)                                    \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_NAMED_ALLOW_CALL_IMPL TROMPELOEIL_LPAREN    \
    TROMPELOEIL_MORE_THAN_TWO_ARGS(__VA_ARGS__), __VA_ARGS__))

// Dispatch to _F (0, 1, or 2 arguments) or _T (> 2 arguments) macro
#define TROMPELOEIL_NAMED_ALLOW_CALL_IMPL(N, ...)                              \
  TROMPELOEIL_IDENTITY(                                                        \
    TROMPELOEIL_NAMED_ALLOW_CALL_ ## N TROMPELOEIL_LPAREN __VA_ARGS__))

// Accept only two arguments
#define TROMPELOEIL_NAMED_ALLOW_CALL_F(obj, func)                              \
  TROMPELOEIL_NAMED_REQUIRE_CALL_T(obj, func, .TROMPELOEIL_INFINITY_TIMES())

// Accept three or more arguments.
#define TROMPELOEIL_NAMED_ALLOW_CALL_T(obj, func, ...)                         \
  TROMPELOEIL_NAMED_REQUIRE_CALL_T(obj,                                        \
                                   func,                                       \
                                   .TROMPELOEIL_INFINITY_TIMES()               \
                                   __VA_ARGS__)


#define TROMPELOEIL_FORBID_CALL_V(...)                                         \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_FORBID_CALL_IMPL TROMPELOEIL_LPAREN         \
    TROMPELOEIL_MORE_THAN_TWO_ARGS(__VA_ARGS__), __VA_ARGS__))

// Dispatch to _F (0, 1, or 2 arguments) or _T (> 2 arguments) macro
#define TROMPELOEIL_FORBID_CALL_IMPL(N, ...)                                   \
  TROMPELOEIL_IDENTITY(                                                        \
    TROMPELOEIL_FORBID_CALL_ ## N TROMPELOEIL_LPAREN __VA_ARGS__))

// Accept only two arguments
#define TROMPELOEIL_FORBID_CALL_F(obj, func)                                   \
  TROMPELOEIL_REQUIRE_CALL_T(obj, func, .TROMPELOEIL_TIMES(0))

// Accept three or more arguments.
#define TROMPELOEIL_FORBID_CALL_T(obj, func, ...)                              \
  TROMPELOEIL_REQUIRE_CALL_T(obj,                                              \
                             func,                                             \
                             .TROMPELOEIL_TIMES(0) __VA_ARGS__)


#define TROMPELOEIL_NAMED_FORBID_CALL_V(...)                                   \
  TROMPELOEIL_IDENTITY(TROMPELOEIL_NAMED_FORBID_CALL_IMPL TROMPELOEIL_LPAREN   \
    TROMPELOEIL_MORE_THAN_TWO_ARGS(__VA_ARGS__), __VA_ARGS__))

// Dispatch to _F (0, 1, or 2 arguments) or _T (> 2 arguments) macro
#define TROMPELOEIL_NAMED_FORBID_CALL_IMPL(N, ...)                             \
  TROMPELOEIL_IDENTITY(                                                        \
    TROMPELOEIL_NAMED_FORBID_CALL_ ## N TROMPELOEIL_LPAREN __VA_ARGS__))

// Accept only two arguments
#define TROMPELOEIL_NAMED_FORBID_CALL_F(obj, func)                             \
  TROMPELOEIL_NAMED_REQUIRE_CALL_T(obj, func, .TROMPELOEIL_TIMES(0))

// Accept three or more arguments.
#define TROMPELOEIL_NAMED_FORBID_CALL_T(obj, func, ...)                        \
  TROMPELOEIL_NAMED_REQUIRE_CALL_T(obj,                                        \
                                   func,                                       \
                                   .TROMPELOEIL_TIMES(0) __VA_ARGS__)


#if (TROMPELOEIL_CPLUSPLUS > 201103L)

#define TROMPELOEIL_REQUIRE_CALL(obj, func)                                    \
  TROMPELOEIL_REQUIRE_CALL_(obj, func, #obj, #func)

#define TROMPELOEIL_REQUIRE_CALL_(obj, func, obj_s, func_s)                    \
  auto TROMPELOEIL_COUNT_ID(call_obj) = TROMPELOEIL_REQUIRE_CALL_OBJ(obj, func,\
                                                               obj_s, func_s)

#define TROMPELOEIL_NAMED_REQUIRE_CALL(obj, func)                              \
  TROMPELOEIL_NAMED_REQUIRE_CALL_(obj, func, #obj, #func)

#define TROMPELOEIL_NAMED_REQUIRE_CALL_(obj, func, obj_s, func_s)              \
  TROMPELOEIL_REQUIRE_CALL_OBJ(obj, func, obj_s, func_s)


#define TROMPELOEIL_REQUIRE_CALL_OBJ(obj, func, obj_s, func_s)                 \
  ::trompeloeil::call_validator_t<decltype((obj).TROMPELOEIL_CONCAT(trompeloeil_self_, func))>{(obj)} + \
    ::trompeloeil::detail::conditional_t<false,                                \
                       decltype((obj).func),                                   \
                       decltype((obj).TROMPELOEIL_CONCAT(trompeloeil_tag_,func))>\
    {__FILE__, static_cast<unsigned long>(__LINE__), obj_s "." func_s}.func


#define TROMPELOEIL_ALLOW_CALL(obj, func)                                      \
  TROMPELOEIL_ALLOW_CALL_(obj, func, #obj, #func)

#define TROMPELOEIL_ALLOW_CALL_(obj, func, obj_s, func_s)                      \
  TROMPELOEIL_REQUIRE_CALL_(obj, func, obj_s, func_s)                          \
    .TROMPELOEIL_INFINITY_TIMES()


#define TROMPELOEIL_NAMED_ALLOW_CALL(obj, func)                                \
  TROMPELOEIL_NAMED_ALLOW_CALL_(obj, func, #obj, #func)

#define TROMPELOEIL_NAMED_ALLOW_CALL_(obj, func, obj_s, func_s)                \
  TROMPELOEIL_NAMED_REQUIRE_CALL_(obj, func, obj_s, func_s)                    \
    .TROMPELOEIL_INFINITY_TIMES()


#define TROMPELOEIL_FORBID_CALL(obj, func)                                     \
  TROMPELOEIL_FORBID_CALL_(obj, func, #obj, #func)

#define TROMPELOEIL_FORBID_CALL_(obj, func, obj_s, func_s)                     \
  TROMPELOEIL_REQUIRE_CALL_(obj, func, obj_s, func_s)                          \
    .TROMPELOEIL_TIMES(0)


#define TROMPELOEIL_NAMED_FORBID_CALL(obj, func)                               \
  TROMPELOEIL_NAMED_FORBID_CALL_(obj, func, #obj, #func)

#define TROMPELOEIL_NAMED_FORBID_CALL_(obj, func, obj_s, func_s)               \
  TROMPELOEIL_NAMED_REQUIRE_CALL_(obj, func, obj_s, func_s)                    \
    .TROMPELOEIL_TIMES(0)

#endif /* (TROMPELOEIL_CPLUSPLUS > 201103L) */


#define TROMPELOEIL_WITH(...)    TROMPELOEIL_WITH_(=,#__VA_ARGS__, __VA_ARGS__)
#define TROMPELOEIL_LR_WITH(...) TROMPELOEIL_WITH_(&,#__VA_ARGS__, __VA_ARGS__)


#if (TROMPELOEIL_CPLUSPLUS != 201103L)

#define TROMPELOEIL_WITH_(capture, arg_s, ...)                                 \
  template action<trompeloeil::with>(arg_s,                                    \
                                     [capture](auto const& trompeloeil_x) {    \
    auto&& _1 = ::trompeloeil::mkarg<1>(trompeloeil_x);                        \
    auto&& _2 = ::trompeloeil::mkarg<2>(trompeloeil_x);                        \
    auto&& _3 = ::trompeloeil::mkarg<3>(trompeloeil_x);                        \
    auto&& _4 = ::trompeloeil::mkarg<4>(trompeloeil_x);                        \
    auto&& _5 = ::trompeloeil::mkarg<5>(trompeloeil_x);                        \
    auto&& _6 = ::trompeloeil::mkarg<6>(trompeloeil_x);                        \
    auto&& _7 = ::trompeloeil::mkarg<7>(trompeloeil_x);                        \
    auto&& _8 = ::trompeloeil::mkarg<8>(trompeloeil_x);                        \
    auto&& _9 = ::trompeloeil::mkarg<9>(trompeloeil_x);                        \
    auto&&_10 = ::trompeloeil::mkarg<10>(trompeloeil_x);                       \
    auto&&_11 = ::trompeloeil::mkarg<11>(trompeloeil_x);                       \
    auto&&_12 = ::trompeloeil::mkarg<12>(trompeloeil_x);                       \
    auto&&_13 = ::trompeloeil::mkarg<13>(trompeloeil_x);                       \
    auto&&_14 = ::trompeloeil::mkarg<14>(trompeloeil_x);                       \
    auto&&_15 = ::trompeloeil::mkarg<15>(trompeloeil_x);                       \
    ::trompeloeil::ignore(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15); \
    return __VA_ARGS__;                                                        \
  })

#endif /* !(TROMPELOEIL_CPLUSPLUS != 201103L) */


#define TROMPELOEIL_SIDE_EFFECT(...)    TROMPELOEIL_SIDE_EFFECT_(=, __VA_ARGS__)
#define TROMPELOEIL_LR_SIDE_EFFECT(...) TROMPELOEIL_SIDE_EFFECT_(&, __VA_ARGS__)


#if (TROMPELOEIL_CPLUSPLUS != 201103L)

#define TROMPELOEIL_SIDE_EFFECT_(capture, ...)                                 \
  template action<trompeloeil::sideeffect>([capture](auto& trompeloeil_x) {    \
    auto&& _1 = ::trompeloeil::mkarg<1>(trompeloeil_x);                        \
    auto&& _2 = ::trompeloeil::mkarg<2>(trompeloeil_x);                        \
    auto&& _3 = ::trompeloeil::mkarg<3>(trompeloeil_x);                        \
    auto&& _4 = ::trompeloeil::mkarg<4>(trompeloeil_x);                        \
    auto&& _5 = ::trompeloeil::mkarg<5>(trompeloeil_x);                        \
    auto&& _6 = ::trompeloeil::mkarg<6>(trompeloeil_x);                        \
    auto&& _7 = ::trompeloeil::mkarg<7>(trompeloeil_x);                        \
    auto&& _8 = ::trompeloeil::mkarg<8>(trompeloeil_x);                        \
    auto&& _9 = ::trompeloeil::mkarg<9>(trompeloeil_x);                        \
    auto&&_10 = ::trompeloeil::mkarg<10>(trompeloeil_x);                       \
    auto&&_11 = ::trompeloeil::mkarg<11>(trompeloeil_x);                       \
    auto&&_12 = ::trompeloeil::mkarg<12>(trompeloeil_x);                       \
    auto&&_13 = ::trompeloeil::mkarg<13>(trompeloeil_x);                       \
    auto&&_14 = ::trompeloeil::mkarg<14>(trompeloeil_x);                       \
    auto&&_15 = ::trompeloeil::mkarg<15>(trompeloeil_x);                       \
    ::trompeloeil::ignore(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15); \
    __VA_ARGS__;                                                               \
  })

#endif /* !(TROMPELOEIL_CPLUSPLUS == 201103L) */


#define TROMPELOEIL_RETURN(...)    TROMPELOEIL_RETURN_(=, __VA_ARGS__)
#define TROMPELOEIL_LR_RETURN(...) TROMPELOEIL_RETURN_(&, __VA_ARGS__)


#if (TROMPELOEIL_CPLUSPLUS != 201103L)

#define TROMPELOEIL_RETURN_(capture, ...)                                      \
  template action<trompeloeil::handle_return>([capture](auto& trompeloeil_x)   \
                                              -> decltype(auto) {              \
    auto&& _1 = ::trompeloeil::mkarg<1>(trompeloeil_x);                        \
    auto&& _2 = ::trompeloeil::mkarg<2>(trompeloeil_x);                        \
    auto&& _3 = ::trompeloeil::mkarg<3>(trompeloeil_x);                        \
    auto&& _4 = ::trompeloeil::mkarg<4>(trompeloeil_x);                        \
    auto&& _5 = ::trompeloeil::mkarg<5>(trompeloeil_x);                        \
    auto&& _6 = ::trompeloeil::mkarg<6>(trompeloeil_x);                        \
    auto&& _7 = ::trompeloeil::mkarg<7>(trompeloeil_x);                        \
    auto&& _8 = ::trompeloeil::mkarg<8>(trompeloeil_x);                        \
    auto&& _9 = ::trompeloeil::mkarg<9>(trompeloeil_x);                        \
    auto&&_10 = ::trompeloeil::mkarg<10>(trompeloeil_x);                       \
    auto&&_11 = ::trompeloeil::mkarg<11>(trompeloeil_x);                       \
    auto&&_12 = ::trompeloeil::mkarg<12>(trompeloeil_x);                       \
    auto&&_13 = ::trompeloeil::mkarg<13>(trompeloeil_x);                       \
    auto&&_14 = ::trompeloeil::mkarg<14>(trompeloeil_x);                       \
    auto&&_15 = ::trompeloeil::mkarg<15>(trompeloeil_x);                       \
    ::trompeloeil::ignore(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15); \
    return ::trompeloeil::decay_return_type(__VA_ARGS__);                      \
  })

#endif /* !(TROMPELOEIL_CPLUSPLUS != 201103L) */


#define TROMPELOEIL_THROW(...)    TROMPELOEIL_THROW_(=, __VA_ARGS__)
#define TROMPELOEIL_LR_THROW(...) TROMPELOEIL_THROW_(&, __VA_ARGS__)


#if (TROMPELOEIL_CPLUSPLUS != 201103L)

#define TROMPELOEIL_THROW_(capture, ...)                                       \
  template action<trompeloeil::handle_throw>([capture](auto& trompeloeil_x) {  \
    auto&& _1 = ::trompeloeil::mkarg<1>(trompeloeil_x);                        \
    auto&& _2 = ::trompeloeil::mkarg<2>(trompeloeil_x);                        \
    auto&& _3 = ::trompeloeil::mkarg<3>(trompeloeil_x);                        \
    auto&& _4 = ::trompeloeil::mkarg<4>(trompeloeil_x);                        \
    auto&& _5 = ::trompeloeil::mkarg<5>(trompeloeil_x);                        \
    auto&& _6 = ::trompeloeil::mkarg<6>(trompeloeil_x);                        \
    auto&& _7 = ::trompeloeil::mkarg<7>(trompeloeil_x);                        \
    auto&& _8 = ::trompeloeil::mkarg<8>(trompeloeil_x);                        \
    auto&& _9 = ::trompeloeil::mkarg<9>(trompeloeil_x);                        \
    auto&&_10 = ::trompeloeil::mkarg<10>(trompeloeil_x);                       \
    auto&&_11 = ::trompeloeil::mkarg<11>(trompeloeil_x);                       \
    auto&&_12 = ::trompeloeil::mkarg<12>(trompeloeil_x);                       \
    auto&&_13 = ::trompeloeil::mkarg<13>(trompeloeil_x);                       \
    auto&&_14 = ::trompeloeil::mkarg<14>(trompeloeil_x);                       \
    auto&&_15 = ::trompeloeil::mkarg<15>(trompeloeil_x);                       \
    ::trompeloeil::ignore(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15); \
    throw __VA_ARGS__;                                                         \
 })

#endif /* !(TROMPELOEIL_CPLUSPLUS == 201103L) */


#define TROMPELOEIL_TIMES(...) template action<trompeloeil::times>(::trompeloeil::multiplicity<__VA_ARGS__>{})
#define TROMPELOEIL_INFINITY_TIMES() TROMPELOEIL_TIMES(0, ~static_cast<size_t>(0))

#define TROMPELOEIL_AT_LEAST(num) num, ~static_cast<size_t>(0)
#define TROMPELOEIL_AT_MOST(num) 0, num


#ifndef TROMPELOEIL_LONG_MACROS
#define MAKE_MOCK0                TROMPELOEIL_MAKE_MOCK0
#define MAKE_MOCK1                TROMPELOEIL_MAKE_MOCK1
#define MAKE_MOCK2                TROMPELOEIL_MAKE_MOCK2
#define MAKE_MOCK3                TROMPELOEIL_MAKE_MOCK3
#define MAKE_MOCK4                TROMPELOEIL_MAKE_MOCK4
#define MAKE_MOCK5                TROMPELOEIL_MAKE_MOCK5
#define MAKE_MOCK6                TROMPELOEIL_MAKE_MOCK6
#define MAKE_MOCK7                TROMPELOEIL_MAKE_MOCK7
#define MAKE_MOCK8                TROMPELOEIL_MAKE_MOCK8
#define MAKE_MOCK9                TROMPELOEIL_MAKE_MOCK9
#define MAKE_MOCK10               TROMPELOEIL_MAKE_MOCK10
#define MAKE_MOCK11               TROMPELOEIL_MAKE_MOCK11
#define MAKE_MOCK12               TROMPELOEIL_MAKE_MOCK12
#define MAKE_MOCK13               TROMPELOEIL_MAKE_MOCK13
#define MAKE_MOCK14               TROMPELOEIL_MAKE_MOCK14
#define MAKE_MOCK15               TROMPELOEIL_MAKE_MOCK15

#define MAKE_CONST_MOCK0          TROMPELOEIL_MAKE_CONST_MOCK0
#define MAKE_CONST_MOCK1          TROMPELOEIL_MAKE_CONST_MOCK1
#define MAKE_CONST_MOCK2          TROMPELOEIL_MAKE_CONST_MOCK2
#define MAKE_CONST_MOCK3          TROMPELOEIL_MAKE_CONST_MOCK3
#define MAKE_CONST_MOCK4          TROMPELOEIL_MAKE_CONST_MOCK4
#define MAKE_CONST_MOCK5          TROMPELOEIL_MAKE_CONST_MOCK5
#define MAKE_CONST_MOCK6          TROMPELOEIL_MAKE_CONST_MOCK6
#define MAKE_CONST_MOCK7          TROMPELOEIL_MAKE_CONST_MOCK7
#define MAKE_CONST_MOCK8          TROMPELOEIL_MAKE_CONST_MOCK8
#define MAKE_CONST_MOCK9          TROMPELOEIL_MAKE_CONST_MOCK9
#define MAKE_CONST_MOCK10         TROMPELOEIL_MAKE_CONST_MOCK10
#define MAKE_CONST_MOCK11         TROMPELOEIL_MAKE_CONST_MOCK11
#define MAKE_CONST_MOCK12         TROMPELOEIL_MAKE_CONST_MOCK12
#define MAKE_CONST_MOCK13         TROMPELOEIL_MAKE_CONST_MOCK13
#define MAKE_CONST_MOCK14         TROMPELOEIL_MAKE_CONST_MOCK14
#define MAKE_CONST_MOCK15         TROMPELOEIL_MAKE_CONST_MOCK15

#define MAKE_STDMETHOD_MOCK0      TROMPELOEIL_MAKE_STDMETHOD_MOCK0
#define MAKE_STDMETHOD_MOCK1      TROMPELOEIL_MAKE_STDMETHOD_MOCK1
#define MAKE_STDMETHOD_MOCK2      TROMPELOEIL_MAKE_STDMETHOD_MOCK2
#define MAKE_STDMETHOD_MOCK3      TROMPELOEIL_MAKE_STDMETHOD_MOCK3
#define MAKE_STDMETHOD_MOCK4      TROMPELOEIL_MAKE_STDMETHOD_MOCK4
#define MAKE_STDMETHOD_MOCK5      TROMPELOEIL_MAKE_STDMETHOD_MOCK5
#define MAKE_STDMETHOD_MOCK6      TROMPELOEIL_MAKE_STDMETHOD_MOCK6
#define MAKE_STDMETHOD_MOCK7      TROMPELOEIL_MAKE_STDMETHOD_MOCK7
#define MAKE_STDMETHOD_MOCK8      TROMPELOEIL_MAKE_STDMETHOD_MOCK8
#define MAKE_STDMETHOD_MOCK9      TROMPELOEIL_MAKE_STDMETHOD_MOCK9
#define MAKE_STDMETHOD_MOCK10     TROMPELOEIL_MAKE_STDMETHOD_MOCK10
#define MAKE_STDMETHOD_MOCK11     TROMPELOEIL_MAKE_STDMETHOD_MOCK11
#define MAKE_STDMETHOD_MOCK12     TROMPELOEIL_MAKE_STDMETHOD_MOCK12
#define MAKE_STDMETHOD_MOCK13     TROMPELOEIL_MAKE_STDMETHOD_MOCK13
#define MAKE_STDMETHOD_MOCK14     TROMPELOEIL_MAKE_STDMETHOD_MOCK14
#define MAKE_STDMETHOD_MOCK15     TROMPELOEIL_MAKE_STDMETHOD_MOCK15

#define IMPLEMENT_MOCK0           TROMPELOEIL_IMPLEMENT_MOCK0
#define IMPLEMENT_MOCK1           TROMPELOEIL_IMPLEMENT_MOCK1
#define IMPLEMENT_MOCK2           TROMPELOEIL_IMPLEMENT_MOCK2
#define IMPLEMENT_MOCK3           TROMPELOEIL_IMPLEMENT_MOCK3
#define IMPLEMENT_MOCK4           TROMPELOEIL_IMPLEMENT_MOCK4
#define IMPLEMENT_MOCK5           TROMPELOEIL_IMPLEMENT_MOCK5
#define IMPLEMENT_MOCK6           TROMPELOEIL_IMPLEMENT_MOCK6
#define IMPLEMENT_MOCK7           TROMPELOEIL_IMPLEMENT_MOCK7
#define IMPLEMENT_MOCK8           TROMPELOEIL_IMPLEMENT_MOCK8
#define IMPLEMENT_MOCK9           TROMPELOEIL_IMPLEMENT_MOCK9
#define IMPLEMENT_MOCK10          TROMPELOEIL_IMPLEMENT_MOCK10
#define IMPLEMENT_MOCK11          TROMPELOEIL_IMPLEMENT_MOCK11
#define IMPLEMENT_MOCK12          TROMPELOEIL_IMPLEMENT_MOCK12
#define IMPLEMENT_MOCK13          TROMPELOEIL_IMPLEMENT_MOCK13
#define IMPLEMENT_MOCK14          TROMPELOEIL_IMPLEMENT_MOCK14
#define IMPLEMENT_MOCK15          TROMPELOEIL_IMPLEMENT_MOCK15

#define IMPLEMENT_CONST_MOCK0     TROMPELOEIL_IMPLEMENT_CONST_MOCK0
#define IMPLEMENT_CONST_MOCK1     TROMPELOEIL_IMPLEMENT_CONST_MOCK1
#define IMPLEMENT_CONST_MOCK2     TROMPELOEIL_IMPLEMENT_CONST_MOCK2
#define IMPLEMENT_CONST_MOCK3     TROMPELOEIL_IMPLEMENT_CONST_MOCK3
#define IMPLEMENT_CONST_MOCK4     TROMPELOEIL_IMPLEMENT_CONST_MOCK4
#define IMPLEMENT_CONST_MOCK5     TROMPELOEIL_IMPLEMENT_CONST_MOCK5
#define IMPLEMENT_CONST_MOCK6     TROMPELOEIL_IMPLEMENT_CONST_MOCK6
#define IMPLEMENT_CONST_MOCK7     TROMPELOEIL_IMPLEMENT_CONST_MOCK7
#define IMPLEMENT_CONST_MOCK8     TROMPELOEIL_IMPLEMENT_CONST_MOCK8
#define IMPLEMENT_CONST_MOCK9     TROMPELOEIL_IMPLEMENT_CONST_MOCK9
#define IMPLEMENT_CONST_MOCK10    TROMPELOEIL_IMPLEMENT_CONST_MOCK10
#define IMPLEMENT_CONST_MOCK11    TROMPELOEIL_IMPLEMENT_CONST_MOCK11
#define IMPLEMENT_CONST_MOCK12    TROMPELOEIL_IMPLEMENT_CONST_MOCK12
#define IMPLEMENT_CONST_MOCK13    TROMPELOEIL_IMPLEMENT_CONST_MOCK13
#define IMPLEMENT_CONST_MOCK14    TROMPELOEIL_IMPLEMENT_CONST_MOCK14
#define IMPLEMENT_CONST_MOCK15    TROMPELOEIL_IMPLEMENT_CONST_MOCK15

#define IMPLEMENT_STDMETHOD_MOCK0  TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK0
#define IMPLEMENT_STDMETHOD_MOCK1  TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK1
#define IMPLEMENT_STDMETHOD_MOCK2  TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK2
#define IMPLEMENT_STDMETHOD_MOCK3  TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK3
#define IMPLEMENT_STDMETHOD_MOCK4  TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK4
#define IMPLEMENT_STDMETHOD_MOCK5  TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK5
#define IMPLEMENT_STDMETHOD_MOCK6  TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK6
#define IMPLEMENT_STDMETHOD_MOCK7  TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK7
#define IMPLEMENT_STDMETHOD_MOCK8  TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK8
#define IMPLEMENT_STDMETHOD_MOCK9  TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK9
#define IMPLEMENT_STDMETHOD_MOCK10 TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK10
#define IMPLEMENT_STDMETHOD_MOCK11 TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK11
#define IMPLEMENT_STDMETHOD_MOCK12 TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK12
#define IMPLEMENT_STDMETHOD_MOCK13 TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK13
#define IMPLEMENT_STDMETHOD_MOCK14 TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK14
#define IMPLEMENT_STDMETHOD_MOCK15 TROMPELOEIL_IMPLEMENT_STDMETHOD_MOCK15

#define REQUIRE_CALL_V            TROMPELOEIL_REQUIRE_CALL_V
#define NAMED_REQUIRE_CALL_V      TROMPELOEIL_NAMED_REQUIRE_CALL_V
#define ALLOW_CALL_V              TROMPELOEIL_ALLOW_CALL_V
#define NAMED_ALLOW_CALL_V        TROMPELOEIL_NAMED_ALLOW_CALL_V
#define FORBID_CALL_V             TROMPELOEIL_FORBID_CALL_V
#define NAMED_FORBID_CALL_V       TROMPELOEIL_NAMED_FORBID_CALL_V

#if (TROMPELOEIL_CPLUSPLUS > 201103L)

#define REQUIRE_CALL              TROMPELOEIL_REQUIRE_CALL
#define NAMED_REQUIRE_CALL        TROMPELOEIL_NAMED_REQUIRE_CALL
#define ALLOW_CALL                TROMPELOEIL_ALLOW_CALL
#define NAMED_ALLOW_CALL          TROMPELOEIL_NAMED_ALLOW_CALL
#define FORBID_CALL               TROMPELOEIL_FORBID_CALL
#define NAMED_FORBID_CALL         TROMPELOEIL_NAMED_FORBID_CALL

#endif /* (TROMPELOEIL_CPLUSPLUS > 201103L) */

#define WITH                      TROMPELOEIL_WITH
#define LR_WITH                   TROMPELOEIL_LR_WITH
#define SIDE_EFFECT               TROMPELOEIL_SIDE_EFFECT
#define LR_SIDE_EFFECT            TROMPELOEIL_LR_SIDE_EFFECT
#define RETURN                    TROMPELOEIL_RETURN
#define LR_RETURN                 TROMPELOEIL_LR_RETURN
#define THROW                     TROMPELOEIL_THROW
#define LR_THROW                  TROMPELOEIL_LR_THROW
#define TIMES                     TROMPELOEIL_TIMES
#define AT_LEAST                  TROMPELOEIL_AT_LEAST
#define AT_MOST                   TROMPELOEIL_AT_MOST

#endif // TROMPELOEIL_LONG_MACROS

#endif // include guard
