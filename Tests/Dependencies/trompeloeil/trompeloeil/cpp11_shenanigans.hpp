/*
 * Trompeloeil C++ mocking framework
 *
 * Copyright (C) Björn Fahller
 * Copyright (C) Andrew Paxie
 *
 *  Use, modification and distribution is subject to the
 *  Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy atl
 *  http://www.boost.org/LICENSE_1_0.txt)
 *
 * Project home: https://github.com/rollbear/trompeloeil
 */

#ifndef TROMPELOEIL_CPP11_SHENANIGANS_HPP
#define TROMPELOEIL_CPP11_SHENANIGANS_HPP

namespace trompeloeil {
namespace detail
{
template <typename T>
struct unwrap_type
{
  using type = T;
};
template <typename T>
struct unwrap_type<std::reference_wrapper<T>>
{
using type = T&;
};

/* Implement C++14 features using only C++11 entities. */

/* <memory> */

/* Implementation of make_unique is from
 *
 * Stephan T. Lavavej, "make_unique (Revision 1),"
 * ISO/IEC JTC1 SC22 WG21 N3656, 18 April 2013.
 * Available: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3656.htm
 * Accessed: 14 June 2017
 *
 * Renamed types to avoid the use of reserved identifiers.
 */
template <class T>
struct unique_if
{
  typedef std::unique_ptr<T> single_object;
};

template <class T>
struct unique_if<T[]>
{
  typedef std::unique_ptr<T[]> unknown_bound;
};

template <class T, size_t N>
struct unique_if<T[N]>
{
  typedef void known_bound;
};

template <class T, class... Args>
typename unique_if<T>::single_object
make_unique(Args&&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T>
typename unique_if<T>::unknown_bound
make_unique(size_t n)
{
  typedef typename std::remove_extent<T>::type U;
  return std::unique_ptr<T>(new U[n]());
}

template <class T, class... Args>
typename unique_if<T>::known_bound
make_unique(Args&&...) = delete;

/* <type_traits> */

/* The implementation of these is from
 *
 * Walter E. Brown, "TransformationTraits Redux, v2,"
 * ISO/IEC JTC1 SC22 WG21 N3655, 18 April 2013.
 * Available: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3655.pdf
 * Accessed: 2 November 2017
 *
 * Minor changes to capitalize template parameter `bool B` has been made.
 *
 * See also:
 * http://en.cppreference.com/w/cpp/types/conditional
 * http://en.cppreference.com/w/cpp/types/decay
 * http://en.cppreference.com/w/cpp/types/enable_if
 * http://en.cppreference.com/w/cpp/types/remove_pointer
 * http://en.cppreference.com/w/cpp/types/remove_reference
 * Accessed: 17 May 2017
 */
template <bool B, typename T, typename F>
using conditional_t = typename std::conditional<B, T, F>::type;

template <typename T>
using decay_t = typename std::decay<T>::type;

template <bool B, typename T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

template <typename T>
using remove_pointer_t = typename std::remove_pointer<T>::type;

template <typename T>
using remove_reference_t = typename std::remove_reference<T>::type;

/* <utility> */

/* This implementation of exchange is from
 *
 * Jeffrey Yasskin, "exchange() utility function, revision 3,"
 * ISO/IEC JTC1 SC22 WG21 N3688, 19 April 2013.
 * Available: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3668.html
 * Accessed: 2 November 2017
 *
 * See also:
 * http://en.cppreference.com/w/cpp/utility/exchange
 * Accessed: 17 May 2017
 */
template<class T, class U = T>
inline
T
exchange(
  T& obj,
  U&& new_value)
{
  T old_value = std::move(obj);
  obj = std::forward<U>(new_value);
  return old_value;
}

/* integer_sequence and index_sequence implementations are from
 *
 * Jonathan Wakely, "Compile-time integer sequences,"
 * ISO/IEC JTC1 SC22 WG21 N3658, 18 April 2013.
 * Available: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3658.html
 * Accessed: 2 November 2017
 *
 * See also:
 * http://en.cppreference.com/w/cpp/utility/integer_sequence
 * Accessed: 17 May 2017
 */
template <typename T, T... I>
struct integer_sequence
{
  // Replaces a typedef used in the definition found in N3658.
  using value_type = T;

  static constexpr size_t size() noexcept
  {
    return sizeof...(I);
  }
};

template <size_t... I>
using index_sequence = integer_sequence<size_t, I...>;

/* This implementation of make_integer_sequence is from boost/mp11,
 *
 * Copyright 2015, 2017 Peter Dimov
 *
 * Distributed under the Boost Software License, Version 1.0.
 *
 * Implemented here:
 *
 * https://github.com/pdimov/mp11/blob/master/include/boost/
 *   integer_sequence.hpp
 * Accessed: 17 May 2017
 *
 * (now missing) and here:
 *
 * https://github.com/boostorg/mp11/blob/develop/include/boost/
 *   mp11/integer_sequence.hpp
 * Accessed: 13 August 2017
 */
namespace impl
{
// iseq_if_c
template <bool C, class T, class E>
struct iseq_if_c_impl;

template <class T, class E>
struct iseq_if_c_impl<true, T, E>
{
  using type = T;
};

template <class T, class E>
struct iseq_if_c_impl<false, T, E>
{
  using type = E;
};

template <bool C, class T, class E>
using iseq_if_c = typename iseq_if_c_impl<C, T, E>::type;

// iseq_identity
template <class T>
struct iseq_identity
{
  using type = T;
};

template <class S1, class S2>
struct append_integer_sequence;

template <class T, T... I, T... J>
struct append_integer_sequence<integer_sequence<T, I...>, integer_sequence<T, J...>>
{
  using type = integer_sequence<T, I..., ( J + sizeof...(I) )...>;
};

template <class T, T N>
struct make_integer_sequence_impl;

template <class T, T N>
struct make_integer_sequence_impl_
{
private:

  static_assert( N >= 0, "make_integer_sequence<T, N>: N must not be negative" );

  static T const M = N / 2;
  static T const R = N % 2;

  using S1 = typename make_integer_sequence_impl<T, M>::type;
  using S2 = typename append_integer_sequence<S1, S1>::type;
  using S3 = typename make_integer_sequence_impl<T, R>::type;
  using S4 = typename append_integer_sequence<S2, S3>::type;

public:

  using type = S4;
};

template <class T, T N>
struct make_integer_sequence_impl:
  iseq_if_c<N == 0,
    iseq_identity<integer_sequence<T>>,
    iseq_if_c<N == 1,
      iseq_identity<integer_sequence<T, 0>>,
      make_integer_sequence_impl_<T, N>>>
{
};
}

template<class T, T N>
using make_integer_sequence = typename impl::make_integer_sequence_impl<T, N>::type;

template <size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

template <typename... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

} /* namespace detail */

}

#define TROMPELOEIL_WITH_(capture, arg_s, ...)                                 \
  template action<trompeloeil::with>(                                          \
    arg_s,                                                                     \
    [capture]                                                                  \
    (typename trompeloeil_e_t::trompeloeil_call_params_type_t const& trompeloeil_x)\
    {                                                                          \
      auto&& _1 = ::trompeloeil::mkarg<1>(trompeloeil_x);                      \
      auto&& _2 = ::trompeloeil::mkarg<2>(trompeloeil_x);                      \
      auto&& _3 = ::trompeloeil::mkarg<3>(trompeloeil_x);                      \
      auto&& _4 = ::trompeloeil::mkarg<4>(trompeloeil_x);                      \
      auto&& _5 = ::trompeloeil::mkarg<5>(trompeloeil_x);                      \
      auto&& _6 = ::trompeloeil::mkarg<6>(trompeloeil_x);                      \
      auto&& _7 = ::trompeloeil::mkarg<7>(trompeloeil_x);                      \
      auto&& _8 = ::trompeloeil::mkarg<8>(trompeloeil_x);                      \
      auto&& _9 = ::trompeloeil::mkarg<9>(trompeloeil_x);                      \
      auto&&_10 = ::trompeloeil::mkarg<10>(trompeloeil_x);                     \
      auto&&_11 = ::trompeloeil::mkarg<11>(trompeloeil_x);                     \
      auto&&_12 = ::trompeloeil::mkarg<12>(trompeloeil_x);                     \
      auto&&_13 = ::trompeloeil::mkarg<13>(trompeloeil_x);                     \
      auto&&_14 = ::trompeloeil::mkarg<14>(trompeloeil_x);                     \
      auto&&_15 = ::trompeloeil::mkarg<15>(trompeloeil_x);                     \
      ::trompeloeil::ignore(                                                   \
        _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15);                   \
      return __VA_ARGS__;                                                      \
    })

#define TROMPELOEIL_SIDE_EFFECT_(capture, ...)                                 \
  template action<trompeloeil::sideeffect>(                                    \
    [capture]                                                                  \
    (typename trompeloeil_e_t::trompeloeil_call_params_type_t& trompeloeil_x)  \
    {                                                                          \
      auto&& _1 = ::trompeloeil::mkarg<1>(trompeloeil_x);                      \
      auto&& _2 = ::trompeloeil::mkarg<2>(trompeloeil_x);                      \
      auto&& _3 = ::trompeloeil::mkarg<3>(trompeloeil_x);                      \
      auto&& _4 = ::trompeloeil::mkarg<4>(trompeloeil_x);                      \
      auto&& _5 = ::trompeloeil::mkarg<5>(trompeloeil_x);                      \
      auto&& _6 = ::trompeloeil::mkarg<6>(trompeloeil_x);                      \
      auto&& _7 = ::trompeloeil::mkarg<7>(trompeloeil_x);                      \
      auto&& _8 = ::trompeloeil::mkarg<8>(trompeloeil_x);                      \
      auto&& _9 = ::trompeloeil::mkarg<9>(trompeloeil_x);                      \
      auto&&_10 = ::trompeloeil::mkarg<10>(trompeloeil_x);                     \
      auto&&_11 = ::trompeloeil::mkarg<11>(trompeloeil_x);                     \
      auto&&_12 = ::trompeloeil::mkarg<12>(trompeloeil_x);                     \
      auto&&_13 = ::trompeloeil::mkarg<13>(trompeloeil_x);                     \
      auto&&_14 = ::trompeloeil::mkarg<14>(trompeloeil_x);                     \
      auto&&_15 = ::trompeloeil::mkarg<15>(trompeloeil_x);                     \
      ::trompeloeil::ignore(                                                   \
        _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15);                   \
      __VA_ARGS__;                                                             \
    })

#define TROMPELOEIL_RETURN_(capture, ...)                                      \
  template action<trompeloeil::handle_return>(                                 \
    [capture]                                                                  \
    (typename trompeloeil_e_t::trompeloeil_call_params_type_t& trompeloeil_x)  \
      -> typename trompeloeil_e_t::trompeloeil_return_of_t                     \
    {                                                                          \
      auto&& _1 = ::trompeloeil::mkarg<1>(trompeloeil_x);                      \
      auto&& _2 = ::trompeloeil::mkarg<2>(trompeloeil_x);                      \
      auto&& _3 = ::trompeloeil::mkarg<3>(trompeloeil_x);                      \
      auto&& _4 = ::trompeloeil::mkarg<4>(trompeloeil_x);                      \
      auto&& _5 = ::trompeloeil::mkarg<5>(trompeloeil_x);                      \
      auto&& _6 = ::trompeloeil::mkarg<6>(trompeloeil_x);                      \
      auto&& _7 = ::trompeloeil::mkarg<7>(trompeloeil_x);                      \
      auto&& _8 = ::trompeloeil::mkarg<8>(trompeloeil_x);                      \
      auto&& _9 = ::trompeloeil::mkarg<9>(trompeloeil_x);                      \
      auto&&_10 = ::trompeloeil::mkarg<10>(trompeloeil_x);                     \
      auto&&_11 = ::trompeloeil::mkarg<11>(trompeloeil_x);                     \
      auto&&_12 = ::trompeloeil::mkarg<12>(trompeloeil_x);                     \
      auto&&_13 = ::trompeloeil::mkarg<13>(trompeloeil_x);                     \
      auto&&_14 = ::trompeloeil::mkarg<14>(trompeloeil_x);                     \
      auto&&_15 = ::trompeloeil::mkarg<15>(trompeloeil_x);                     \
      ::trompeloeil::ignore(                                                   \
        _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15);                   \
      return ::trompeloeil::decay_return_type(__VA_ARGS__);                    \
    })
#define TROMPELOEIL_THROW_(capture, ...)                                       \
  template action<trompeloeil::handle_throw>(                                  \
    [capture]                                                                  \
    (typename trompeloeil_e_t::trompeloeil_call_params_type_t& trompeloeil_x)  \
    {                                                                          \
      auto&& _1 = ::trompeloeil::mkarg<1>(trompeloeil_x);                      \
      auto&& _2 = ::trompeloeil::mkarg<2>(trompeloeil_x);                      \
      auto&& _3 = ::trompeloeil::mkarg<3>(trompeloeil_x);                      \
      auto&& _4 = ::trompeloeil::mkarg<4>(trompeloeil_x);                      \
      auto&& _5 = ::trompeloeil::mkarg<5>(trompeloeil_x);                      \
      auto&& _6 = ::trompeloeil::mkarg<6>(trompeloeil_x);                      \
      auto&& _7 = ::trompeloeil::mkarg<7>(trompeloeil_x);                      \
      auto&& _8 = ::trompeloeil::mkarg<8>(trompeloeil_x);                      \
      auto&& _9 = ::trompeloeil::mkarg<9>(trompeloeil_x);                      \
      auto&&_10 = ::trompeloeil::mkarg<10>(trompeloeil_x);                     \
      auto&&_11 = ::trompeloeil::mkarg<11>(trompeloeil_x);                     \
      auto&&_12 = ::trompeloeil::mkarg<12>(trompeloeil_x);                     \
      auto&&_13 = ::trompeloeil::mkarg<13>(trompeloeil_x);                     \
      auto&&_14 = ::trompeloeil::mkarg<14>(trompeloeil_x);                     \
      auto&&_15 = ::trompeloeil::mkarg<15>(trompeloeil_x);                     \
      ::trompeloeil::ignore(                                                   \
        _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15);                   \
      throw __VA_ARGS__;                                                       \
    })

#endif //TROMPELOEIL_CPP11_SHENANIGANS_HPP
