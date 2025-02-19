// <chrono> -*- C++ -*-

// Copyright (C) 2008-2024 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file include/chrono
 *  This is a Standard C++ Library header.
 *  @ingroup chrono
 */

#ifndef _GLIBCXX_CHRONO
#define _GLIBCXX_CHRONO 1

#if __cplusplus < 201103L
#include <bits/c++0x_warning.h>
#else

#ifndef __glibcxx_chrono
#define __glibcxx_chrono
#endif
#include <bits/chrono.h>
#include <iterator>

#if __cplusplus >= 202002L
#include <bit>
#include <bits/stl_algo.h> // upper_bound
#include <bits/unique_ptr.h>
#endif

#define __glibcxx_want_chrono
#define __glibcxx_want_chrono_udls
#include <bits/version.h>

namespace std _GLIBCXX_VISIBILITY(default) {
_GLIBCXX_BEGIN_NAMESPACE_VERSION

/**
 * @defgroup chrono Time
 * @ingroup utilities
 *
 * Classes and functions for time.
 *
 * @since C++11
 */

/** @namespace std::chrono
 *  @brief ISO C++ 2011 namespace for date and time utilities
 *  @ingroup chrono
 */
namespace chrono {
#if __cplusplus >= 202002L
/// @addtogroup chrono
/// @{
struct local_t {};
template <typename _Duration> using local_time = time_point<local_t, _Duration>;
using local_seconds = local_time<seconds>;
using local_days = local_time<days>;

class utc_clock;
class tai_clock;
class gps_clock;

template <typename _Duration> using utc_time = time_point<utc_clock, _Duration>;
using utc_seconds = utc_time<seconds>;

template <typename _Duration> using tai_time = time_point<tai_clock, _Duration>;
using tai_seconds = tai_time<seconds>;

template <typename _Duration> using gps_time = time_point<gps_clock, _Duration>;
using gps_seconds = gps_time<seconds>;

template <> struct is_clock<utc_clock> : true_type {};
template <> struct is_clock<tai_clock> : true_type {};
template <> struct is_clock<gps_clock> : true_type {};

template <> inline constexpr bool is_clock_v<utc_clock> = true;
template <> inline constexpr bool is_clock_v<tai_clock> = true;
template <> inline constexpr bool is_clock_v<gps_clock> = true;

struct leap_second_info {
  bool is_leap_second;
  seconds elapsed;
};

namespace __detail {
inline leap_second_info __get_leap_second_info(sys_seconds __ss,
                                               bool __is_utc) {
  if (__ss < sys_seconds{}) [[unlikely]]
    return {};

  const seconds::rep __leaps[]{
      78796800,   // 1 Jul 1972
      94694400,   // 1 Jan 1973
      126230400,  // 1 Jan 1974
      157766400,  // 1 Jan 1975
      189302400,  // 1 Jan 1976
      220924800,  // 1 Jan 1977
      252460800,  // 1 Jan 1978
      283996800,  // 1 Jan 1979
      315532800,  // 1 Jan 1980
      362793600,  // 1 Jul 1981
      394329600,  // 1 Jul 1982
      425865600,  // 1 Jul 1983
      489024000,  // 1 Jul 1985
      567993600,  // 1 Jan 1988
      631152000,  // 1 Jan 1990
      662688000,  // 1 Jan 1991
      709948800,  // 1 Jul 1992
      741484800,  // 1 Jul 1993
      773020800,  // 1 Jul 1994
      820454400,  // 1 Jan 1996
      867715200,  // 1 Jul 1997
      915148800,  // 1 Jan 1999
      1136073600, // 1 Jan 2006
      1230768000, // 1 Jan 2009
      1341100800, // 1 Jul 2012
      1435708800, // 1 Jul 2015
      1483228800, // 1 Jan 2017
  };
  // The list above is known to be valid until (at least) 2024-12-28 00:00:00 UTC
  // and only contains positive leap seconds.

  {
    seconds::rep __s = __ss.time_since_epoch().count();
    const seconds::rep *__first = std::begin(__leaps);
    const seconds::rep *__last = std::end(__leaps);

    // Don't bother searching the list if we're after the last one.
    if (__s > (__last[-1] + (__last - __first) + 1))
      return {false, seconds(__last - __first)};

    auto __pos = std::upper_bound(__first, __last, __s);
    seconds __elapsed{__pos - __first};
    if (__is_utc) {
      // Convert utc_time to sys_time:
      __s -= __elapsed.count();
      // See if that sys_time is before (or during) previous leap sec:
      if (__pos != __first && __s < __pos[-1]) {
        if ((__s + 1) >= __pos[-1])
          return {true, __elapsed};
        --__elapsed;
      }
    }
    return {false, __elapsed};
  }
}
} // namespace __detail
template <typename _Duration>
leap_second_info get_leap_second_info(const utc_time<_Duration> &__ut) {
  auto __s = chrono::duration_cast<seconds>(__ut.time_since_epoch());
  return __detail::__get_leap_second_info(sys_seconds(__s), true);
}

/** A clock that measures Universal Coordinated Time (UTC).
 *
 * The epoch is 1970-01-01 00:00:00.
 *
 * @since C++20
 */
class utc_clock {
public:
  using rep = system_clock::rep;
  using period = system_clock::period;
  using duration = chrono::duration<rep, period>;
  using time_point = chrono::time_point<utc_clock>;
  static constexpr bool is_steady = false;

  [[nodiscard]]
  static time_point now() {
    return from_sys(system_clock::now());
  }

  template <typename _Duration>
  [[nodiscard]]
  static sys_time<common_type_t<_Duration, seconds>>
  to_sys(const utc_time<_Duration> &__t) {
    using _CDur = common_type_t<_Duration, seconds>;
    const auto __li = chrono::get_leap_second_info(__t);
    sys_time<_CDur> __s{__t.time_since_epoch() - __li.elapsed};
    if (__li.is_leap_second)
      __s = chrono::floor<seconds>(__s) + seconds{1} - _CDur{1};
    return __s;
  }

  template <typename _Duration>
  [[nodiscard]]
  static utc_time<common_type_t<_Duration, seconds>>
  from_sys(const sys_time<_Duration> &__t);
};

/** A clock that measures International Atomic Time.
 *
 * The epoch is 1958-01-01 00:00:00.
 *
 * @since C++20
 */
class tai_clock {
public:
  using rep = system_clock::rep;
  using period = system_clock::period;
  using duration = chrono::duration<rep, period>;
  using time_point = chrono::time_point<tai_clock>;
  static constexpr bool is_steady = false; // XXX true for CLOCK_TAI?

  // TODO move into lib, use CLOCK_TAI on linux, add extension point.
  [[nodiscard]]
  static time_point now() {
    return from_utc(utc_clock::now());
  }

  template <typename _Duration>
  [[nodiscard]]
  static utc_time<common_type_t<_Duration, seconds>>
  to_utc(const tai_time<_Duration> &__t) {
    using _CDur = common_type_t<_Duration, seconds>;
    return utc_time<_CDur>{__t.time_since_epoch()} - chrono::seconds(378691210);
  }

  template <typename _Duration>
  [[nodiscard]]
  static tai_time<common_type_t<_Duration, seconds>>
  from_utc(const utc_time<_Duration> &__t) {
    using _CDur = common_type_t<_Duration, seconds>;
    return tai_time<_CDur>{__t.time_since_epoch()} + chrono::seconds(378691210);
  }
};

/** A clock that measures GPS time.
 *
 * The epoch is 1980-01-06 00:00:00.
 *
 * @since C++20
 */
class gps_clock {
public:
  using rep = system_clock::rep;
  using period = system_clock::period;
  using duration = chrono::duration<rep, period>;
  using time_point = chrono::time_point<gps_clock>;
  static constexpr bool is_steady = false; // XXX

  // TODO move into lib, add extension point.
  [[nodiscard]]
  static time_point now() {
    return from_utc(utc_clock::now());
  }

  template <typename _Duration>
  [[nodiscard]]
  static utc_time<common_type_t<_Duration, seconds>>
  to_utc(const gps_time<_Duration> &__t) {
    using _CDur = common_type_t<_Duration, seconds>;
    return utc_time<_CDur>{__t.time_since_epoch()} + chrono::seconds(315964809);
  }

  template <typename _Duration>
  [[nodiscard]]
  static gps_time<common_type_t<_Duration, seconds>>
  from_utc(const utc_time<_Duration> &__t) {
    using _CDur = common_type_t<_Duration, seconds>;
    return gps_time<_CDur>{__t.time_since_epoch()} - chrono::seconds(315964809);
  }
};

template <typename _DestClock, typename _SourceClock>
struct clock_time_conversion {};

// Identity conversions

template <typename _Clock> struct clock_time_conversion<_Clock, _Clock> {
  template <typename _Duration>
  time_point<_Clock, _Duration>
  operator()(const time_point<_Clock, _Duration> &__t) const {
    return __t;
  }
};

template <> struct clock_time_conversion<system_clock, system_clock> {
  template <typename _Duration>
  sys_time<_Duration> operator()(const sys_time<_Duration> &__t) const {
    return __t;
  }
};

template <> struct clock_time_conversion<utc_clock, utc_clock> {
  template <typename _Duration>
  utc_time<_Duration> operator()(const utc_time<_Duration> &__t) const {
    return __t;
  }
};

// Conversions between system_clock and utc_clock

template <> struct clock_time_conversion<utc_clock, system_clock> {
  template <typename _Duration>
  utc_time<common_type_t<_Duration, seconds>>
  operator()(const sys_time<_Duration> &__t) const {
    return utc_clock::from_sys(__t);
  }
};

template <> struct clock_time_conversion<system_clock, utc_clock> {
  template <typename _Duration>
  sys_time<common_type_t<_Duration, seconds>>
  operator()(const utc_time<_Duration> &__t) const {
    return utc_clock::to_sys(__t);
  }
};

template <typename _Tp, typename _Clock>
inline constexpr bool __is_time_point_for_v = false;

template <typename _Clock, typename _Duration>
inline constexpr bool
    __is_time_point_for_v<time_point<_Clock, _Duration>, _Clock> = true;

// Conversions between system_clock and other clocks

template <typename _SourceClock>
struct clock_time_conversion<system_clock, _SourceClock> {
  template <typename _Duration, typename _Src = _SourceClock>
  auto operator()(const time_point<_SourceClock, _Duration> &__t) const
      -> decltype(_Src::to_sys(__t)) {
    using _Ret = decltype(_SourceClock::to_sys(__t));
    static_assert(__is_time_point_for_v<_Ret, system_clock>);
    return _SourceClock::to_sys(__t);
  }
};

template <typename _DestClock>
struct clock_time_conversion<_DestClock, system_clock> {
  template <typename _Duration, typename _Dest = _DestClock>
  auto operator()(const sys_time<_Duration> &__t) const
      -> decltype(_Dest::from_sys(__t)) {
    using _Ret = decltype(_DestClock::from_sys(__t));
    static_assert(__is_time_point_for_v<_Ret, _DestClock>);
    return _DestClock::from_sys(__t);
  }
};

// Conversions between utc_clock and other clocks

template <typename _SourceClock>
struct clock_time_conversion<utc_clock, _SourceClock> {
  template <typename _Duration, typename _Src = _SourceClock>
  auto operator()(const time_point<_SourceClock, _Duration> &__t) const
      -> decltype(_Src::to_utc(__t)) {
    using _Ret = decltype(_SourceClock::to_utc(__t));
    static_assert(__is_time_point_for_v<_Ret, utc_clock>);
    return _SourceClock::to_utc(__t);
  }
};

template <typename _DestClock>
struct clock_time_conversion<_DestClock, utc_clock> {
  template <typename _Duration, typename _Dest = _DestClock>
  auto operator()(const utc_time<_Duration> &__t) const
      -> decltype(_Dest::from_utc(__t)) {
    using _Ret = decltype(_DestClock::from_utc(__t));
    static_assert(__is_time_point_for_v<_Ret, _DestClock>);
    return _DestClock::from_utc(__t);
  }
};

/// @cond undocumented
namespace __detail {
template <typename _DestClock, typename _SourceClock, typename _Duration>
concept __clock_convs =
    requires(const time_point<_SourceClock, _Duration> &__t) {
      clock_time_conversion<_DestClock, _SourceClock>{}(__t);
    };

template <typename _DestClock, typename _SourceClock, typename _Duration>
concept __clock_convs_sys =
    requires(const time_point<_SourceClock, _Duration> &__t) {
      clock_time_conversion<_DestClock, system_clock>{}(
          clock_time_conversion<system_clock, _SourceClock>{}(__t));
    };

template <typename _DestClock, typename _SourceClock, typename _Duration>
concept __clock_convs_utc =
    requires(const time_point<_SourceClock, _Duration> &__t) {
      clock_time_conversion<_DestClock, utc_clock>{}(
          clock_time_conversion<utc_clock, _SourceClock>{}(__t));
    };

template <typename _DestClock, typename _SourceClock, typename _Duration>
concept __clock_convs_sys_utc =
    requires(const time_point<_SourceClock, _Duration> &__t) {
      clock_time_conversion<_DestClock, utc_clock>{}(
          clock_time_conversion<utc_clock, system_clock>{}(
              clock_time_conversion<system_clock, _SourceClock>{}(__t)));
    };

template <typename _DestClock, typename _SourceClock, typename _Duration>
concept __clock_convs_utc_sys =
    requires(const time_point<_SourceClock, _Duration> &__t) {
      clock_time_conversion<_DestClock, system_clock>{}(
          clock_time_conversion<system_clock, utc_clock>{}(
              clock_time_conversion<utc_clock, _SourceClock>{}(__t)));
    };

} // namespace __detail
/// @endcond

/// Convert a time point to a different clock.
template <typename _DestClock, typename _SourceClock, typename _Duration>
[[nodiscard]]
inline auto clock_cast(const time_point<_SourceClock, _Duration> &__t)
  requires __detail::__clock_convs<_DestClock, _SourceClock, _Duration> ||
           __detail::__clock_convs_sys<_DestClock, _SourceClock, _Duration> ||
           __detail::__clock_convs_utc<_DestClock, _SourceClock, _Duration> ||
           __detail::__clock_convs_sys_utc<_DestClock, _SourceClock,
                                           _Duration> ||
           __detail::__clock_convs_utc_sys<_DestClock, _SourceClock, _Duration>
{
  constexpr bool __direct =
      __detail::__clock_convs<_DestClock, _SourceClock, _Duration>;
  if constexpr (__direct) {
    return clock_time_conversion<_DestClock, _SourceClock>{}(__t);
  } else {
    constexpr bool __convert_via_sys_clock =
        __detail::__clock_convs_sys<_DestClock, _SourceClock, _Duration>;
    constexpr bool __convert_via_utc_clock =
        __detail::__clock_convs_utc<_DestClock, _SourceClock, _Duration>;
    if constexpr (__convert_via_sys_clock) {
      static_assert(!__convert_via_utc_clock,
                    "clock_cast requires a unique best conversion, but "
                    "conversion is possible via system_clock and also via"
                    "utc_clock");
      return clock_time_conversion<_DestClock, system_clock>{}(
          clock_time_conversion<system_clock, _SourceClock>{}(__t));
    } else if constexpr (__convert_via_utc_clock) {
      return clock_time_conversion<_DestClock, utc_clock>{}(
          clock_time_conversion<utc_clock, _SourceClock>{}(__t));
    } else {
      constexpr bool __convert_via_sys_and_utc_clocks =
          __detail::__clock_convs_sys_utc<_DestClock, _SourceClock, _Duration>;

      if constexpr (__convert_via_sys_and_utc_clocks) {
        constexpr bool __convert_via_utc_and_sys_clocks =
            __detail::__clock_convs_utc_sys<_DestClock, _SourceClock,
                                            _Duration>;
        static_assert(!__convert_via_utc_and_sys_clocks,
                      "clock_cast requires a unique best conversion, but "
                      "conversion is possible via system_clock followed by "
                      "utc_clock, and also via utc_clock followed by "
                      "system_clock");
        return clock_time_conversion<_DestClock, utc_clock>{}(
            clock_time_conversion<utc_clock, system_clock>{}(
                clock_time_conversion<system_clock, _SourceClock>{}(__t)));
      } else {
        return clock_time_conversion<_DestClock, system_clock>{}(
            clock_time_conversion<system_clock, utc_clock>{}(
                clock_time_conversion<utc_clock, _SourceClock>{}(__t)));
      }
    }
  }
}

// CALENDRICAL TYPES

// CLASS DECLARATIONS
class day;
class month;
class year;
class weekday;
class weekday_indexed;
class weekday_last;
class month_day;
class month_day_last;
class month_weekday;
class month_weekday_last;
class year_month;
class year_month_day;
class year_month_day_last;
class year_month_weekday;
class year_month_weekday_last;

struct last_spec {
  explicit last_spec() = default;

  friend constexpr month_day_last operator/(int __m, last_spec) noexcept;

  friend constexpr month_day_last operator/(last_spec, int __m) noexcept;
};

inline constexpr last_spec last{};

namespace __detail {
// Helper to __add_modulo and __sub_modulo.
template <unsigned __d, typename _Tp> consteval auto __modulo_offset() {
  using _Up = make_unsigned_t<_Tp>;
  auto constexpr __a = _Up(-1) - _Up(255 + __d - 2);
  auto constexpr __b = _Up(__d * (__a / __d) - 1);
  // Notice: b <= a - 1 <= _Up(-1) - (255 + d - 1) and b % d = d - 1.
  return _Up(-1) - __b; // >= 255 + d - 1
}

// Compute the remainder of the Euclidean division of __x + __y divided by
// __d without overflowing.  Typically, __x <= 255 + d - 1 is sum of
// weekday/month with a shift in [0, d - 1] and __y is a duration count.
template <unsigned __d, typename _Tp>
constexpr unsigned __add_modulo(unsigned __x, _Tp __y) {
  using _Up = make_unsigned_t<_Tp>;
  // For __y >= 0, _Up(__y) has the same mathematical value as __y and
  // this function simply returns (__x + _Up(__y)) % d.  Typically, this
  // doesn't overflow since the range of _Up contains many more positive
  // values than _Tp's.  For __y < 0, _Up(__y) has a mathematical value in
  // the upper-half range of _Up so that adding a positive value to it
  // might overflow.  Moreover, most likely, _Up(__y) != __y mod d.  To
  // fix both issues we subtract from _Up(__y) an __offset >=
  // 255 + d - 1 to make room for the addition to __x and shift the modulo
  // to the correct value.
  auto const __offset = __y >= 0 ? _Up(0) : __modulo_offset<__d, _Tp>();
  return (__x + _Up(__y) - __offset) % __d;
}

// Similar to __add_modulo but for __x - __y.
template <unsigned __d, typename _Tp>
constexpr unsigned __sub_modulo(unsigned __x, _Tp __y) {
  using _Up = make_unsigned_t<_Tp>;
  auto const __offset = __y <= 0 ? _Up(0) : __modulo_offset<__d, _Tp>();
  return (__x - _Up(__y) - __offset) % __d;
}

inline constexpr unsigned __days_per_month[12] = {31, 29, 31, 30, 31, 30,
                                                  31, 31, 30, 31, 30, 31};
} // namespace __detail

// DAY

class day {
private:
  unsigned char _M_d;

public:
  day() = default;

  explicit constexpr day(unsigned __d) noexcept : _M_d(__d) {}

  constexpr day &operator++() noexcept {
    ++_M_d;
    return *this;
  }

  constexpr day operator++(int) noexcept {
    auto __ret = *this;
    ++(*this);
    return __ret;
  }

  constexpr day &operator--() noexcept {
    --_M_d;
    return *this;
  }

  constexpr day operator--(int) noexcept {
    auto __ret = *this;
    --(*this);
    return __ret;
  }

  constexpr day &operator+=(const days &__d) noexcept {
    *this = *this + __d;
    return *this;
  }

  constexpr day &operator-=(const days &__d) noexcept {
    *this = *this - __d;
    return *this;
  }

  constexpr explicit operator unsigned() const noexcept { return _M_d; }

  constexpr bool ok() const noexcept { return 1 <= _M_d && _M_d <= 31; }

  friend constexpr bool operator==(const day &__x, const day &__y) noexcept {
    return unsigned{__x} == unsigned{__y};
  }

  friend constexpr strong_ordering operator<=>(const day &__x,
                                               const day &__y) noexcept {
    return unsigned{__x} <=> unsigned{__y};
  }

  friend constexpr day operator+(const day &__x, const days &__y) noexcept {
    return day(unsigned{__x} + __y.count());
  }

  friend constexpr day operator+(const days &__x, const day &__y) noexcept {
    return __y + __x;
  }

  friend constexpr day operator-(const day &__x, const days &__y) noexcept {
    return __x + -__y;
  }

  friend constexpr days operator-(const day &__x, const day &__y) noexcept {
    return days{int(unsigned{__x}) - int(unsigned{__y})};
  }

  friend constexpr month_day operator/(const month &__m,
                                       const day &__d) noexcept;

  friend constexpr month_day operator/(int __m, const day &__d) noexcept;

  friend constexpr month_day operator/(const day &__d,
                                       const month &__m) noexcept;

  friend constexpr month_day operator/(const day &__d, int __m) noexcept;

  friend constexpr year_month_day operator/(const year_month &__ym,
                                            const day &__d) noexcept;
};

// MONTH

class month {
private:
  unsigned char _M_m;

public:
  month() = default;

  explicit constexpr month(unsigned __m) noexcept : _M_m(__m) {}

  constexpr month &operator++() noexcept {
    *this += months{1};
    return *this;
  }

  constexpr month operator++(int) noexcept {
    auto __ret = *this;
    ++(*this);
    return __ret;
  }

  constexpr month &operator--() noexcept {
    *this -= months{1};
    return *this;
  }

  constexpr month operator--(int) noexcept {
    auto __ret = *this;
    --(*this);
    return __ret;
  }

  constexpr month &operator+=(const months &__m) noexcept {
    *this = *this + __m;
    return *this;
  }

  constexpr month &operator-=(const months &__m) noexcept {
    *this = *this - __m;
    return *this;
  }

  explicit constexpr operator unsigned() const noexcept { return _M_m; }

  constexpr bool ok() const noexcept { return 1 <= _M_m && _M_m <= 12; }

  friend constexpr bool operator==(const month &__x,
                                   const month &__y) noexcept {
    return unsigned{__x} == unsigned{__y};
  }

  friend constexpr strong_ordering operator<=>(const month &__x,
                                               const month &__y) noexcept {
    return unsigned{__x} <=> unsigned{__y};
  }

  friend constexpr month operator+(const month &__x,
                                   const months &__y) noexcept {
    // modulo(x + (y - 1), 12) = modulo(x + (y - 1) + 12, 12)
    //                         = modulo((x + 11) + y    , 12)
    return month{1 +
                 __detail::__add_modulo<12>(unsigned{__x} + 11, __y.count())};
  }

  friend constexpr month operator+(const months &__x,
                                   const month &__y) noexcept {
    return __y + __x;
  }

  friend constexpr month operator-(const month &__x,
                                   const months &__y) noexcept {
    // modulo(x + (-y - 1), 12) = modulo(x + (-y - 1) + 12, 12)
    //                          = modulo((x + 11) - y     , 12)
    return month{1 +
                 __detail::__sub_modulo<12>(unsigned{__x} + 11, __y.count())};
  }

  friend constexpr months operator-(const month &__x,
                                    const month &__y) noexcept {
    const auto __dm = int(unsigned(__x)) - int(unsigned(__y));
    return months{__dm < 0 ? 12 + __dm : __dm};
  }

  friend constexpr year_month operator/(const year &__y,
                                        const month &__m) noexcept;

  friend constexpr month_day operator/(const month &__m, int __d) noexcept;

  friend constexpr month_day_last operator/(const month &__m,
                                            last_spec) noexcept;

  friend constexpr month_day_last operator/(last_spec,
                                            const month &__m) noexcept;

  friend constexpr month_weekday
  operator/(const month &__m, const weekday_indexed &__wdi) noexcept;

  friend constexpr month_weekday operator/(const weekday_indexed &__wdi,
                                           const month &__m) noexcept;

  friend constexpr month_weekday_last
  operator/(const month &__m, const weekday_last &__wdl) noexcept;

  friend constexpr month_weekday_last operator/(const weekday_last &__wdl,
                                                const month &__m) noexcept;
};

inline constexpr month January{1};
inline constexpr month February{2};
inline constexpr month March{3};
inline constexpr month April{4};
inline constexpr month May{5};
inline constexpr month June{6};
inline constexpr month July{7};
inline constexpr month August{8};
inline constexpr month September{9};
inline constexpr month October{10};
inline constexpr month November{11};
inline constexpr month December{12};

// YEAR

class year {
private:
  short _M_y;

public:
  year() = default;

  explicit constexpr year(int __y) noexcept : _M_y{static_cast<short>(__y)} {}

  static constexpr year min() noexcept { return year{-32767}; }

  static constexpr year max() noexcept { return year{32767}; }

  constexpr year &operator++() noexcept {
    ++_M_y;
    return *this;
  }

  constexpr year operator++(int) noexcept {
    auto __ret = *this;
    ++(*this);
    return __ret;
  }

  constexpr year &operator--() noexcept {
    --_M_y;
    return *this;
  }

  constexpr year operator--(int) noexcept {
    auto __ret = *this;
    --(*this);
    return __ret;
  }

  constexpr year &operator+=(const years &__y) noexcept {
    *this = *this + __y;
    return *this;
  }

  constexpr year &operator-=(const years &__y) noexcept {
    *this = *this - __y;
    return *this;
  }

  constexpr year operator+() const noexcept { return *this; }

  constexpr year operator-() const noexcept { return year{-_M_y}; }

  constexpr bool is_leap() const noexcept {
    // Testing divisibility by 100 first gives better performance [1], i.e.,
    //     return _M_y % 100 == 0 ? _M_y % 400 == 0 : _M_y % 16 == 0;
    // Furthermore, if _M_y % 100 == 0, then _M_y % 400 == 0 is equivalent
    // to _M_y % 16 == 0, so we can simplify it to
    //     return _M_y % 100 == 0 ? _M_y % 16 == 0 : _M_y % 4 == 0.  // #1
    // Similarly, we can replace 100 with 25 (which is good since
    // _M_y % 25 == 0 requires one fewer instruction than _M_y % 100 == 0
    // [2]):
    //     return _M_y % 25 == 0 ? _M_y % 16 == 0 : _M_y % 4 == 0.  // #2
    // Indeed, first assume _M_y % 4 != 0.  Then _M_y % 16 != 0 and hence,
    // _M_y % 4 == 0 and _M_y % 16 == 0 are both false.  Therefore, #2
    // returns false as it should (regardless of _M_y % 25.) Now assume
    // _M_y % 4 == 0.  In this case, _M_y % 25 == 0 if, and only if,
    // _M_y % 100 == 0, that is, #1 and #2 are equivalent.  Finally, #2 is
    // equivalent to
    //     return (_M_y & (_M_y % 25 == 0 ? 15 : 3)) == 0.

    // References:
    // [1] https://github.com/cassioneri/calendar
    // [2] https://godbolt.org/z/55G8rn77e
    // [3] https://gcc.gnu.org/pipermail/libstdc++/2021-June/052815.html

    return (_M_y & (_M_y % 25 == 0 ? 15 : 3)) == 0;
  }

  explicit constexpr operator int() const noexcept { return _M_y; }

  constexpr bool ok() const noexcept {
    return min()._M_y <= _M_y && _M_y <= max()._M_y;
  }

  friend constexpr bool operator==(const year &__x, const year &__y) noexcept {
    return int{__x} == int{__y};
  }

  friend constexpr strong_ordering operator<=>(const year &__x,
                                               const year &__y) noexcept {
    return int{__x} <=> int{__y};
  }

  friend constexpr year operator+(const year &__x, const years &__y) noexcept {
    return year{int{__x} + static_cast<int>(__y.count())};
  }

  friend constexpr year operator+(const years &__x, const year &__y) noexcept {
    return __y + __x;
  }

  friend constexpr year operator-(const year &__x, const years &__y) noexcept {
    return __x + -__y;
  }

  friend constexpr years operator-(const year &__x, const year &__y) noexcept {
    return years{int{__x} - int{__y}};
  }

  friend constexpr year_month operator/(const year &__y, int __m) noexcept;

  friend constexpr year_month_day operator/(const year &__y,
                                            const month_day &__md) noexcept;

  friend constexpr year_month_day operator/(const month_day &__md,
                                            const year &__y) noexcept;

  friend constexpr year_month_day_last
  operator/(const year &__y, const month_day_last &__mdl) noexcept;

  friend constexpr year_month_day_last operator/(const month_day_last &__mdl,
                                                 const year &__y) noexcept;

  friend constexpr year_month_weekday
  operator/(const year &__y, const month_weekday &__mwd) noexcept;

  friend constexpr year_month_weekday operator/(const month_weekday &__mwd,
                                                const year &__y) noexcept;

  friend constexpr year_month_weekday_last
  operator/(const year &__y, const month_weekday_last &__mwdl) noexcept;

  friend constexpr year_month_weekday_last
  operator/(const month_weekday_last &__mwdl, const year &__y) noexcept;
};

// WEEKDAY

class weekday {
private:
  unsigned char _M_wd;

  static constexpr weekday _S_from_days(const days &__d) {
    return weekday{__detail::__add_modulo<7>(4, __d.count())};
  }

public:
  weekday() = default;

  explicit constexpr weekday(unsigned __wd) noexcept
      : _M_wd(__wd == 7 ? 0 : __wd) // __wd % 7 ?
  {}

  constexpr weekday(const sys_days &__dp) noexcept
      : weekday{_S_from_days(__dp.time_since_epoch())} {}

  explicit constexpr weekday(const local_days &__dp) noexcept
      : weekday{sys_days{__dp.time_since_epoch()}} {}

  constexpr weekday &operator++() noexcept {
    *this += days{1};
    return *this;
  }

  constexpr weekday operator++(int) noexcept {
    auto __ret = *this;
    ++(*this);
    return __ret;
  }

  constexpr weekday &operator--() noexcept {
    *this -= days{1};
    return *this;
  }

  constexpr weekday operator--(int) noexcept {
    auto __ret = *this;
    --(*this);
    return __ret;
  }

  constexpr weekday &operator+=(const days &__d) noexcept {
    *this = *this + __d;
    return *this;
  }

  constexpr weekday &operator-=(const days &__d) noexcept {
    *this = *this - __d;
    return *this;
  }

  constexpr unsigned c_encoding() const noexcept { return _M_wd; }

  constexpr unsigned iso_encoding() const noexcept {
    return _M_wd == 0u ? 7u : _M_wd;
  }

  constexpr bool ok() const noexcept { return _M_wd <= 6; }

  constexpr weekday_indexed operator[](unsigned __index) const noexcept;

  constexpr weekday_last operator[](last_spec) const noexcept;

  friend constexpr bool operator==(const weekday &__x,
                                   const weekday &__y) noexcept {
    return __x._M_wd == __y._M_wd;
  }

  friend constexpr weekday operator+(const weekday &__x,
                                     const days &__y) noexcept {
    return weekday{__detail::__add_modulo<7>(__x._M_wd, __y.count())};
  }

  friend constexpr weekday operator+(const days &__x,
                                     const weekday &__y) noexcept {
    return __y + __x;
  }

  friend constexpr weekday operator-(const weekday &__x,
                                     const days &__y) noexcept {
    return weekday{__detail::__sub_modulo<7>(__x._M_wd, __y.count())};
  }

  friend constexpr days operator-(const weekday &__x,
                                  const weekday &__y) noexcept {
    const auto __n = __x.c_encoding() - __y.c_encoding();
    return static_cast<int>(__n) >= 0 ? days{__n} : days{__n + 7};
  }
};

inline constexpr weekday Sunday{0};
inline constexpr weekday Monday{1};
inline constexpr weekday Tuesday{2};
inline constexpr weekday Wednesday{3};
inline constexpr weekday Thursday{4};
inline constexpr weekday Friday{5};
inline constexpr weekday Saturday{6};

// WEEKDAY_INDEXED

class weekday_indexed {
private:
  chrono::weekday _M_wd;
  unsigned char _M_index;

public:
  weekday_indexed() = default;

  constexpr weekday_indexed(const chrono::weekday &__wd,
                            unsigned __index) noexcept
      : _M_wd(__wd), _M_index(__index) {}

  constexpr chrono::weekday weekday() const noexcept { return _M_wd; }

  constexpr unsigned index() const noexcept { return _M_index; };

  constexpr bool ok() const noexcept {
    return _M_wd.ok() && 1 <= _M_index && _M_index <= 5;
  }

  friend constexpr bool operator==(const weekday_indexed &__x,
                                   const weekday_indexed &__y) noexcept {
    return __x.weekday() == __y.weekday() && __x.index() == __y.index();
  }

  friend constexpr month_weekday
  operator/(const month &__m, const weekday_indexed &__wdi) noexcept;

  friend constexpr month_weekday
  operator/(int __m, const weekday_indexed &__wdi) noexcept;

  friend constexpr month_weekday operator/(const weekday_indexed &__wdi,
                                           const month &__m) noexcept;

  friend constexpr month_weekday operator/(const weekday_indexed &__wdi,
                                           int __m) noexcept;

  friend constexpr year_month_weekday
  operator/(const year_month &__ym, const weekday_indexed &__wdi) noexcept;
};

constexpr weekday_indexed weekday::operator[](unsigned __index) const noexcept {
  return {*this, __index};
}

// WEEKDAY_LAST

class weekday_last {
private:
  chrono::weekday _M_wd;

public:
  explicit constexpr weekday_last(const chrono::weekday &__wd) noexcept
      : _M_wd{__wd} {}

  constexpr chrono::weekday weekday() const noexcept { return _M_wd; }

  constexpr bool ok() const noexcept { return _M_wd.ok(); }

  friend constexpr bool operator==(const weekday_last &__x,
                                   const weekday_last &__y) noexcept {
    return __x.weekday() == __y.weekday();
  }

  friend constexpr month_weekday_last
  operator/(int __m, const weekday_last &__wdl) noexcept;

  friend constexpr month_weekday_last operator/(const weekday_last &__wdl,
                                                int __m) noexcept;

  friend constexpr year_month_weekday_last
  operator/(const year_month &__ym, const weekday_last &__wdl) noexcept;
};

constexpr weekday_last weekday::operator[](last_spec) const noexcept {
  return weekday_last{*this};
}

// MONTH_DAY

class month_day {
private:
  chrono::month _M_m;
  chrono::day _M_d;

public:
  month_day() = default;

  constexpr month_day(const chrono::month &__m, const chrono::day &__d) noexcept
      : _M_m{__m}, _M_d{__d} {}

  constexpr chrono::month month() const noexcept { return _M_m; }

  constexpr chrono::day day() const noexcept { return _M_d; }

  constexpr bool ok() const noexcept {
    return _M_m.ok() && 1u <= unsigned(_M_d) &&
           unsigned(_M_d) <= __detail::__days_per_month[unsigned(_M_m) - 1];
  }

  friend constexpr bool operator==(const month_day &__x,
                                   const month_day &__y) noexcept {
    return __x.month() == __y.month() && __x.day() == __y.day();
  }

  friend constexpr strong_ordering
  operator<=>(const month_day &__x, const month_day &__y) noexcept = default;

  friend constexpr month_day operator/(const chrono::month &__m,
                                       const chrono::day &__d) noexcept {
    return {__m, __d};
  }

  friend constexpr month_day operator/(const chrono::month &__m,
                                       int __d) noexcept {
    return {__m, chrono::day(unsigned(__d))};
  }

  friend constexpr month_day operator/(int __m,
                                       const chrono::day &__d) noexcept {
    return {chrono::month(unsigned(__m)), __d};
  }

  friend constexpr month_day operator/(const chrono::day &__d,
                                       const chrono::month &__m) noexcept {
    return {__m, __d};
  }

  friend constexpr month_day operator/(const chrono::day &__d,
                                       int __m) noexcept {
    return {chrono::month(unsigned(__m)), __d};
  }

  friend constexpr year_month_day operator/(int __y,
                                            const month_day &__md) noexcept;

  friend constexpr year_month_day operator/(const month_day &__md,
                                            int __y) noexcept;
};

// MONTH_DAY_LAST

class month_day_last {
private:
  chrono::month _M_m;

public:
  explicit constexpr month_day_last(const chrono::month &__m) noexcept
      : _M_m{__m} {}

  constexpr chrono::month month() const noexcept { return _M_m; }

  constexpr bool ok() const noexcept { return _M_m.ok(); }

  friend constexpr bool operator==(const month_day_last &__x,
                                   const month_day_last &__y) noexcept {
    return __x.month() == __y.month();
  }

  friend constexpr strong_ordering
  operator<=>(const month_day_last &__x,
              const month_day_last &__y) noexcept = default;

  friend constexpr month_day_last operator/(const chrono::month &__m,
                                            last_spec) noexcept {
    return month_day_last{__m};
  }

  friend constexpr month_day_last operator/(int __m, last_spec) noexcept {
    return chrono::month(unsigned(__m)) / last;
  }

  friend constexpr month_day_last operator/(last_spec,
                                            const chrono::month &__m) noexcept {
    return __m / last;
  }

  friend constexpr month_day_last operator/(last_spec, int __m) noexcept {
    return __m / last;
  }

  friend constexpr year_month_day_last
  operator/(int __y, const month_day_last &__mdl) noexcept;

  friend constexpr year_month_day_last operator/(const month_day_last &__mdl,
                                                 int __y) noexcept;
};

// MONTH_WEEKDAY

class month_weekday {
private:
  chrono::month _M_m;
  chrono::weekday_indexed _M_wdi;

public:
  constexpr month_weekday(const chrono::month &__m,
                          const chrono::weekday_indexed &__wdi) noexcept
      : _M_m{__m}, _M_wdi{__wdi} {}

  constexpr chrono::month month() const noexcept { return _M_m; }

  constexpr chrono::weekday_indexed weekday_indexed() const noexcept {
    return _M_wdi;
  }

  constexpr bool ok() const noexcept { return _M_m.ok() && _M_wdi.ok(); }

  friend constexpr bool operator==(const month_weekday &__x,
                                   const month_weekday &__y) noexcept {
    return __x.month() == __y.month() &&
           __x.weekday_indexed() == __y.weekday_indexed();
  }

  friend constexpr month_weekday
  operator/(const chrono::month &__m,
            const chrono::weekday_indexed &__wdi) noexcept {
    return {__m, __wdi};
  }

  friend constexpr month_weekday
  operator/(int __m, const chrono::weekday_indexed &__wdi) noexcept {
    return chrono::month(unsigned(__m)) / __wdi;
  }

  friend constexpr month_weekday operator/(const chrono::weekday_indexed &__wdi,
                                           const chrono::month &__m) noexcept {
    return __m / __wdi;
  }

  friend constexpr month_weekday operator/(const chrono::weekday_indexed &__wdi,
                                           int __m) noexcept {
    return __m / __wdi;
  }

  friend constexpr year_month_weekday
  operator/(int __y, const month_weekday &__mwd) noexcept;

  friend constexpr year_month_weekday operator/(const month_weekday &__mwd,
                                                int __y) noexcept;
};

// MONTH_WEEKDAY_LAST

class month_weekday_last {
private:
  chrono::month _M_m;
  chrono::weekday_last _M_wdl;

public:
  constexpr month_weekday_last(const chrono::month &__m,
                               const chrono::weekday_last &__wdl) noexcept
      : _M_m{__m}, _M_wdl{__wdl} {}

  constexpr chrono::month month() const noexcept { return _M_m; }

  constexpr chrono::weekday_last weekday_last() const noexcept {
    return _M_wdl;
  }

  constexpr bool ok() const noexcept { return _M_m.ok() && _M_wdl.ok(); }

  friend constexpr bool operator==(const month_weekday_last &__x,
                                   const month_weekday_last &__y) noexcept {
    return __x.month() == __y.month() &&
           __x.weekday_last() == __y.weekday_last();
  }

  friend constexpr month_weekday_last
  operator/(const chrono::month &__m,
            const chrono::weekday_last &__wdl) noexcept {
    return {__m, __wdl};
  }

  friend constexpr month_weekday_last
  operator/(int __m, const chrono::weekday_last &__wdl) noexcept {
    return chrono::month(unsigned(__m)) / __wdl;
  }

  friend constexpr month_weekday_last
  operator/(const chrono::weekday_last &__wdl,
            const chrono::month &__m) noexcept {
    return __m / __wdl;
  }

  friend constexpr month_weekday_last
  operator/(const chrono::weekday_last &__wdl, int __m) noexcept {
    return chrono::month(unsigned(__m)) / __wdl;
  }

  friend constexpr year_month_weekday_last
  operator/(int __y, const month_weekday_last &__mwdl) noexcept;

  friend constexpr year_month_weekday_last
  operator/(const month_weekday_last &__mwdl, int __y) noexcept;
};

// YEAR_MONTH

namespace __detail {
// [time.cal.ym], [time.cal.ymd], etc constrain the 'months'-based
// addition/subtraction operator overloads like so:
//
//   Constraints: if the argument supplied by the caller for the months
//   parameter is convertible to years, its implicit conversion sequence
//   to years is worse than its implicit conversion sequence to months.
//
// We realize this constraint by templatizing the 'months'-based
// overloads (using a dummy defaulted template parameter), so that
// overload resolution doesn't select the 'months'-based overload unless
// the implicit conversion sequence to 'months' is better than that to
// 'years'.
using __months_years_conversion_disambiguator = void;
} // namespace __detail

class year_month {
private:
  chrono::year _M_y;
  chrono::month _M_m;

public:
  year_month() = default;

  constexpr year_month(const chrono::year &__y,
                       const chrono::month &__m) noexcept
      : _M_y{__y}, _M_m{__m} {}

  constexpr chrono::year year() const noexcept { return _M_y; }

  constexpr chrono::month month() const noexcept { return _M_m; }

  template <typename = __detail::__months_years_conversion_disambiguator>
  constexpr year_month &operator+=(const months &__dm) noexcept {
    *this = *this + __dm;
    return *this;
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  constexpr year_month &operator-=(const months &__dm) noexcept {
    *this = *this - __dm;
    return *this;
  }

  constexpr year_month &operator+=(const years &__dy) noexcept {
    *this = *this + __dy;
    return *this;
  }

  constexpr year_month &operator-=(const years &__dy) noexcept {
    *this = *this - __dy;
    return *this;
  }

  constexpr bool ok() const noexcept { return _M_y.ok() && _M_m.ok(); }

  friend constexpr bool operator==(const year_month &__x,
                                   const year_month &__y) noexcept {
    return __x.year() == __y.year() && __x.month() == __y.month();
  }

  friend constexpr strong_ordering
  operator<=>(const year_month &__x, const year_month &__y) noexcept = default;

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month operator+(const year_month &__ym,
                                        const months &__dm) noexcept {
    // TODO: Optimize?
    auto __m = __ym.month() + __dm;
    auto __i = int(unsigned(__ym.month())) - 1 + __dm.count();
    auto __y = (__i < 0 ? __ym.year() + years{(__i - 11) / 12}
                        : __ym.year() + years{__i / 12});
    return __y / __m;
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month operator+(const months &__dm,
                                        const year_month &__ym) noexcept {
    return __ym + __dm;
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month operator-(const year_month &__ym,
                                        const months &__dm) noexcept {
    return __ym + -__dm;
  }

  friend constexpr months operator-(const year_month &__x,
                                    const year_month &__y) noexcept {
    return (__x.year() - __y.year() +
            months{static_cast<int>(unsigned{__x.month()}) -
                   static_cast<int>(unsigned{__y.month()})});
  }

  friend constexpr year_month operator+(const year_month &__ym,
                                        const years &__dy) noexcept {
    return (__ym.year() + __dy) / __ym.month();
  }

  friend constexpr year_month operator+(const years &__dy,
                                        const year_month &__ym) noexcept {
    return __ym + __dy;
  }

  friend constexpr year_month operator-(const year_month &__ym,
                                        const years &__dy) noexcept {
    return __ym + -__dy;
  }

  friend constexpr year_month operator/(const chrono::year &__y,
                                        const chrono::month &__m) noexcept {
    return {__y, __m};
  }

  friend constexpr year_month operator/(const chrono::year &__y,
                                        int __m) noexcept {
    return {__y, chrono::month(unsigned(__m))};
  }

  friend constexpr year_month_day operator/(const year_month &__ym,
                                            int __d) noexcept;

  friend constexpr year_month_day_last operator/(const year_month &__ym,
                                                 last_spec) noexcept;
};

// YEAR_MONTH_DAY

class year_month_day {
private:
  chrono::year _M_y;
  chrono::month _M_m;
  chrono::day _M_d;

  static constexpr year_month_day _S_from_days(const days &__dp) noexcept;

  constexpr days _M_days_since_epoch() const noexcept;

public:
  year_month_day() = default;

  constexpr year_month_day(const chrono::year &__y, const chrono::month &__m,
                           const chrono::day &__d) noexcept
      : _M_y{__y}, _M_m{__m}, _M_d{__d} {}

  constexpr year_month_day(const year_month_day_last &__ymdl) noexcept;

  constexpr year_month_day(const sys_days &__dp) noexcept
      : year_month_day(_S_from_days(__dp.time_since_epoch())) {}

  explicit constexpr year_month_day(const local_days &__dp) noexcept
      : year_month_day(sys_days{__dp.time_since_epoch()}) {}

  template <typename = __detail::__months_years_conversion_disambiguator>
  constexpr year_month_day &operator+=(const months &__m) noexcept {
    *this = *this + __m;
    return *this;
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  constexpr year_month_day &operator-=(const months &__m) noexcept {
    *this = *this - __m;
    return *this;
  }

  constexpr year_month_day &operator+=(const years &__y) noexcept {
    *this = *this + __y;
    return *this;
  }

  constexpr year_month_day &operator-=(const years &__y) noexcept {
    *this = *this - __y;
    return *this;
  }

  constexpr chrono::year year() const noexcept { return _M_y; }

  constexpr chrono::month month() const noexcept { return _M_m; }

  constexpr chrono::day day() const noexcept { return _M_d; }

  constexpr operator sys_days() const noexcept {
    return sys_days{_M_days_since_epoch()};
  }

  explicit constexpr operator local_days() const noexcept {
    return local_days{sys_days{*this}.time_since_epoch()};
  }

  constexpr bool ok() const noexcept;

  friend constexpr bool operator==(const year_month_day &__x,
                                   const year_month_day &__y) noexcept {
    return __x.year() == __y.year() && __x.month() == __y.month() &&
           __x.day() == __y.day();
  }

  friend constexpr strong_ordering
  operator<=>(const year_month_day &__x,
              const year_month_day &__y) noexcept = default;

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month_day operator+(const year_month_day &__ymd,
                                            const months &__dm) noexcept {
    return (__ymd.year() / __ymd.month() + __dm) / __ymd.day();
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month_day
  operator+(const months &__dm, const year_month_day &__ymd) noexcept {
    return __ymd + __dm;
  }

  friend constexpr year_month_day operator+(const year_month_day &__ymd,
                                            const years &__dy) noexcept {
    return (__ymd.year() + __dy) / __ymd.month() / __ymd.day();
  }

  friend constexpr year_month_day
  operator+(const years &__dy, const year_month_day &__ymd) noexcept {
    return __ymd + __dy;
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month_day operator-(const year_month_day &__ymd,
                                            const months &__dm) noexcept {
    return __ymd + -__dm;
  }

  friend constexpr year_month_day operator-(const year_month_day &__ymd,
                                            const years &__dy) noexcept {
    return __ymd + -__dy;
  }

  friend constexpr year_month_day operator/(const year_month &__ym,
                                            const chrono::day &__d) noexcept {
    return {__ym.year(), __ym.month(), __d};
  }

  friend constexpr year_month_day operator/(const year_month &__ym,
                                            int __d) noexcept {
    return __ym / chrono::day{unsigned(__d)};
  }

  friend constexpr year_month_day operator/(const chrono::year &__y,
                                            const month_day &__md) noexcept {
    return __y / __md.month() / __md.day();
  }

  friend constexpr year_month_day operator/(int __y,
                                            const month_day &__md) noexcept {
    return chrono::year{__y} / __md;
  }

  friend constexpr year_month_day operator/(const month_day &__md,
                                            const chrono::year &__y) noexcept {
    return __y / __md;
  }

  friend constexpr year_month_day operator/(const month_day &__md,
                                            int __y) noexcept {
    return chrono::year(__y) / __md;
  }
};

// Construct from days since 1970/01/01.
// Proposition 6.3 of Neri and Schneider,
// "Euclidean Affine Functions and Applications to Calendar Algorithms".
// https://arxiv.org/abs/2102.06959
constexpr year_month_day
year_month_day::_S_from_days(const days &__dp) noexcept {
  constexpr auto __z2 = static_cast<uint32_t>(-1468000);
  constexpr auto __r2_e3 = static_cast<uint32_t>(536895458);

  const auto __r0 = static_cast<uint32_t>(__dp.count()) + __r2_e3;

  const auto __n1 = 4 * __r0 + 3;
  const auto __q1 = __n1 / 146097;
  const auto __r1 = __n1 % 146097 / 4;

  constexpr auto __p32 = static_cast<uint64_t>(1) << 32;
  const auto __n2 = 4 * __r1 + 3;
  const auto __u2 = static_cast<uint64_t>(2939745) * __n2;
  const auto __q2 = static_cast<uint32_t>(__u2 / __p32);
  const auto __r2 = static_cast<uint32_t>(__u2 % __p32) / 2939745 / 4;

  constexpr auto __p16 = static_cast<uint32_t>(1) << 16;
  const auto __n3 = 2141 * __r2 + 197913;
  const auto __q3 = __n3 / __p16;
  const auto __r3 = __n3 % __p16 / 2141;

  const auto __y0 = 100 * __q1 + __q2;
  const auto __m0 = __q3;
  const auto __d0 = __r3;

  const auto __j = __r2 >= 306;
  const auto __y1 = __y0 + __j;
  const auto __m1 = __j ? __m0 - 12 : __m0;
  const auto __d1 = __d0 + 1;

  return year_month_day{chrono::year{static_cast<int>(__y1 + __z2)},
                        chrono::month{__m1}, chrono::day{__d1}};
}

// Days since 1970/01/01.
// Proposition 6.2 of Neri and Schneider,
// "Euclidean Affine Functions and Applications to Calendar Algorithms".
// https://arxiv.org/abs/2102.06959
constexpr days year_month_day::_M_days_since_epoch() const noexcept {
  auto constexpr __z2 = static_cast<uint32_t>(-1468000);
  auto constexpr __r2_e3 = static_cast<uint32_t>(536895458);

  const auto __y1 = static_cast<uint32_t>(static_cast<int>(_M_y)) - __z2;
  const auto __m1 = static_cast<uint32_t>(static_cast<unsigned>(_M_m));
  const auto __d1 = static_cast<uint32_t>(static_cast<unsigned>(_M_d));

  const auto __j = static_cast<uint32_t>(__m1 < 3);
  const auto __y0 = __y1 - __j;
  const auto __m0 = __j ? __m1 + 12 : __m1;
  const auto __d0 = __d1 - 1;

  const auto __q1 = __y0 / 100;
  const auto __yc = 1461 * __y0 / 4 - __q1 + __q1 / 4;
  const auto __mc = (979 * __m0 - 2919) / 32;
  const auto __dc = __d0;

  return days{static_cast<int32_t>(__yc + __mc + __dc - __r2_e3)};
}

// YEAR_MONTH_DAY_LAST

class year_month_day_last {
private:
  chrono::year _M_y;
  chrono::month_day_last _M_mdl;

public:
  constexpr year_month_day_last(const chrono::year &__y,
                                const chrono::month_day_last &__mdl) noexcept
      : _M_y{__y}, _M_mdl{__mdl} {}

  template <typename = __detail::__months_years_conversion_disambiguator>
  constexpr year_month_day_last &operator+=(const months &__m) noexcept {
    *this = *this + __m;
    return *this;
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  constexpr year_month_day_last &operator-=(const months &__m) noexcept {
    *this = *this - __m;
    return *this;
  }

  constexpr year_month_day_last &operator+=(const years &__y) noexcept {
    *this = *this + __y;
    return *this;
  }

  constexpr year_month_day_last &operator-=(const years &__y) noexcept {
    *this = *this - __y;
    return *this;
  }

  constexpr chrono::year year() const noexcept { return _M_y; }

  constexpr chrono::month month() const noexcept { return _M_mdl.month(); }

  constexpr chrono::month_day_last month_day_last() const noexcept {
    return _M_mdl;
  }

  // Return A day representing the last day of this year, month pair.
  constexpr chrono::day day() const noexcept {
    const auto __m = static_cast<unsigned>(month());

    // The result is unspecified if __m < 1 or __m > 12.  Hence, assume
    // 1 <= __m <= 12.  For __m != 2, day() == 30 or day() == 31 or, in
    // other words, day () == 30 | b, where b is in {0, 1}.

    // If __m in {1, 3, 4, 5, 6, 7}, then b is 1 if, and only if, __m is
    // odd.  Hence, b = __m & 1 = (__m ^ 0) & 1.

    // If __m in {8, 9, 10, 11, 12}, then b is 1 if, and only if, __m is
    // even.  Hence, b = (__m ^ 1) & 1.

    // Therefore, b = (__m ^ c) & 1, where c = 0, if __m < 8, or c = 1 if
    // __m >= 8, that is, c = __m >> 3.

    // Since 30 = (11110)_2 and __m <= 31 = (11111)_2, the "& 1" in b's
    // calculation is unnecessary.

    // The performance of this implementation does not depend on look-up
    // tables being on the L1 cache.
    return chrono::day{__m != 2         ? (__m ^ (__m >> 3)) | 30
                       : _M_y.is_leap() ? 29
                                        : 28};
  }

  constexpr operator sys_days() const noexcept {
    return sys_days{year() / month() / day()};
  }

  explicit constexpr operator local_days() const noexcept {
    return local_days{sys_days{*this}.time_since_epoch()};
  }

  constexpr bool ok() const noexcept { return _M_y.ok() && _M_mdl.ok(); }

  friend constexpr bool operator==(const year_month_day_last &__x,
                                   const year_month_day_last &__y) noexcept {
    return __x.year() == __y.year() &&
           __x.month_day_last() == __y.month_day_last();
  }

  friend constexpr strong_ordering
  operator<=>(const year_month_day_last &__x,
              const year_month_day_last &__y) noexcept = default;

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month_day_last
  operator+(const year_month_day_last &__ymdl, const months &__dm) noexcept {
    return (__ymdl.year() / __ymdl.month() + __dm) / last;
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month_day_last
  operator+(const months &__dm, const year_month_day_last &__ymdl) noexcept {
    return __ymdl + __dm;
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month_day_last
  operator-(const year_month_day_last &__ymdl, const months &__dm) noexcept {
    return __ymdl + -__dm;
  }

  friend constexpr year_month_day_last
  operator+(const year_month_day_last &__ymdl, const years &__dy) noexcept {
    return {__ymdl.year() + __dy, __ymdl.month_day_last()};
  }

  friend constexpr year_month_day_last
  operator+(const years &__dy, const year_month_day_last &__ymdl) noexcept {
    return __ymdl + __dy;
  }

  friend constexpr year_month_day_last
  operator-(const year_month_day_last &__ymdl, const years &__dy) noexcept {
    return __ymdl + -__dy;
  }

  friend constexpr year_month_day_last operator/(const year_month &__ym,
                                                 last_spec) noexcept {
    return {__ym.year(), chrono::month_day_last{__ym.month()}};
  }

  friend constexpr year_month_day_last
  operator/(const chrono::year &__y,
            const chrono::month_day_last &__mdl) noexcept {
    return {__y, __mdl};
  }

  friend constexpr year_month_day_last
  operator/(int __y, const chrono::month_day_last &__mdl) noexcept {
    return chrono::year(__y) / __mdl;
  }

  friend constexpr year_month_day_last
  operator/(const chrono::month_day_last &__mdl,
            const chrono::year &__y) noexcept {
    return __y / __mdl;
  }

  friend constexpr year_month_day_last
  operator/(const chrono::month_day_last &__mdl, int __y) noexcept {
    return chrono::year(__y) / __mdl;
  }
};

// year_month_day ctor from year_month_day_last
constexpr year_month_day::year_month_day(
    const year_month_day_last &__ymdl) noexcept
    : _M_y{__ymdl.year()}, _M_m{__ymdl.month()}, _M_d{__ymdl.day()} {}

constexpr bool year_month_day::ok() const noexcept {
  if (!_M_y.ok() || !_M_m.ok())
    return false;
  return chrono::day{1} <= _M_d && _M_d <= (_M_y / _M_m / last).day();
}

// YEAR_MONTH_WEEKDAY

class year_month_weekday {
private:
  chrono::year _M_y;
  chrono::month _M_m;
  chrono::weekday_indexed _M_wdi;

  static constexpr year_month_weekday _S_from_sys_days(const sys_days &__dp) {
    year_month_day __ymd{__dp};
    chrono::weekday __wd{__dp};
    auto __index = __wd[(unsigned{__ymd.day()} - 1) / 7 + 1];
    return {__ymd.year(), __ymd.month(), __index};
  }

public:
  year_month_weekday() = default;

  constexpr year_month_weekday(const chrono::year &__y,
                               const chrono::month &__m,
                               const chrono::weekday_indexed &__wdi) noexcept
      : _M_y{__y}, _M_m{__m}, _M_wdi{__wdi} {}

  constexpr year_month_weekday(const sys_days &__dp) noexcept
      : year_month_weekday{_S_from_sys_days(__dp)} {}

  explicit constexpr year_month_weekday(const local_days &__dp) noexcept
      : year_month_weekday{sys_days{__dp.time_since_epoch()}} {}

  template <typename = __detail::__months_years_conversion_disambiguator>
  constexpr year_month_weekday &operator+=(const months &__m) noexcept {
    *this = *this + __m;
    return *this;
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  constexpr year_month_weekday &operator-=(const months &__m) noexcept {
    *this = *this - __m;
    return *this;
  }

  constexpr year_month_weekday &operator+=(const years &__y) noexcept {
    *this = *this + __y;
    return *this;
  }

  constexpr year_month_weekday &operator-=(const years &__y) noexcept {
    *this = *this - __y;
    return *this;
  }

  constexpr chrono::year year() const noexcept { return _M_y; }

  constexpr chrono::month month() const noexcept { return _M_m; }

  constexpr chrono::weekday weekday() const noexcept {
    return _M_wdi.weekday();
  }

  constexpr unsigned index() const noexcept { return _M_wdi.index(); }

  constexpr chrono::weekday_indexed weekday_indexed() const noexcept {
    return _M_wdi;
  }

  constexpr operator sys_days() const noexcept {
    auto __d = sys_days{year() / month() / 1};
    return __d + (weekday() - chrono::weekday(__d) +
                  days{(static_cast<int>(index()) - 1) * 7});
  }

  explicit constexpr operator local_days() const noexcept {
    return local_days{sys_days{*this}.time_since_epoch()};
  }

  constexpr bool ok() const noexcept {
    if (!_M_y.ok() || !_M_m.ok() || !_M_wdi.ok())
      return false;
    if (_M_wdi.index() <= 4)
      return true;
    days __d = (_M_wdi.weekday() - chrono::weekday{sys_days{_M_y / _M_m / 1}} +
                days((_M_wdi.index() - 1) * 7 + 1));
    __glibcxx_assert(__d.count() >= 1);
    return (unsigned)__d.count() <= (unsigned)(_M_y / _M_m / last).day();
  }

  friend constexpr bool operator==(const year_month_weekday &__x,
                                   const year_month_weekday &__y) noexcept {
    return __x.year() == __y.year() && __x.month() == __y.month() &&
           __x.weekday_indexed() == __y.weekday_indexed();
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month_weekday
  operator+(const year_month_weekday &__ymwd, const months &__dm) noexcept {
    return ((__ymwd.year() / __ymwd.month() + __dm) / __ymwd.weekday_indexed());
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month_weekday
  operator+(const months &__dm, const year_month_weekday &__ymwd) noexcept {
    return __ymwd + __dm;
  }

  friend constexpr year_month_weekday
  operator+(const year_month_weekday &__ymwd, const years &__dy) noexcept {
    return {__ymwd.year() + __dy, __ymwd.month(), __ymwd.weekday_indexed()};
  }

  friend constexpr year_month_weekday
  operator+(const years &__dy, const year_month_weekday &__ymwd) noexcept {
    return __ymwd + __dy;
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month_weekday
  operator-(const year_month_weekday &__ymwd, const months &__dm) noexcept {
    return __ymwd + -__dm;
  }

  friend constexpr year_month_weekday
  operator-(const year_month_weekday &__ymwd, const years &__dy) noexcept {
    return __ymwd + -__dy;
  }

  friend constexpr year_month_weekday
  operator/(const year_month &__ym,
            const chrono::weekday_indexed &__wdi) noexcept {
    return {__ym.year(), __ym.month(), __wdi};
  }

  friend constexpr year_month_weekday
  operator/(const chrono::year &__y, const month_weekday &__mwd) noexcept {
    return {__y, __mwd.month(), __mwd.weekday_indexed()};
  }

  friend constexpr year_month_weekday
  operator/(int __y, const month_weekday &__mwd) noexcept {
    return chrono::year(__y) / __mwd;
  }

  friend constexpr year_month_weekday
  operator/(const month_weekday &__mwd, const chrono::year &__y) noexcept {
    return __y / __mwd;
  }

  friend constexpr year_month_weekday operator/(const month_weekday &__mwd,
                                                int __y) noexcept {
    return chrono::year(__y) / __mwd;
  }
};

// YEAR_MONTH_WEEKDAY_LAST

class year_month_weekday_last {
private:
  chrono::year _M_y;
  chrono::month _M_m;
  chrono::weekday_last _M_wdl;

public:
  constexpr year_month_weekday_last(const chrono::year &__y,
                                    const chrono::month &__m,
                                    const chrono::weekday_last &__wdl) noexcept
      : _M_y{__y}, _M_m{__m}, _M_wdl{__wdl} {}

  template <typename = __detail::__months_years_conversion_disambiguator>
  constexpr year_month_weekday_last &operator+=(const months &__m) noexcept {
    *this = *this + __m;
    return *this;
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  constexpr year_month_weekday_last &operator-=(const months &__m) noexcept {
    *this = *this - __m;
    return *this;
  }

  constexpr year_month_weekday_last &operator+=(const years &__y) noexcept {
    *this = *this + __y;
    return *this;
  }

  constexpr year_month_weekday_last &operator-=(const years &__y) noexcept {
    *this = *this - __y;
    return *this;
  }

  constexpr chrono::year year() const noexcept { return _M_y; }

  constexpr chrono::month month() const noexcept { return _M_m; }

  constexpr chrono::weekday weekday() const noexcept {
    return _M_wdl.weekday();
  }

  constexpr chrono::weekday_last weekday_last() const noexcept {
    return _M_wdl;
  }

  constexpr operator sys_days() const noexcept {
    const auto __d = sys_days{_M_y / _M_m / last};
    return sys_days{
        (__d - (chrono::weekday{__d} - _M_wdl.weekday())).time_since_epoch()};
  }

  explicit constexpr operator local_days() const noexcept {
    return local_days{sys_days{*this}.time_since_epoch()};
  }

  constexpr bool ok() const noexcept {
    return _M_y.ok() && _M_m.ok() && _M_wdl.ok();
  }

  friend constexpr bool
  operator==(const year_month_weekday_last &__x,
             const year_month_weekday_last &__y) noexcept {
    return __x.year() == __y.year() && __x.month() == __y.month() &&
           __x.weekday_last() == __y.weekday_last();
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month_weekday_last
  operator+(const year_month_weekday_last &__ymwdl,
            const months &__dm) noexcept {
    return ((__ymwdl.year() / __ymwdl.month() + __dm) / __ymwdl.weekday_last());
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month_weekday_last
  operator+(const months &__dm,
            const year_month_weekday_last &__ymwdl) noexcept {
    return __ymwdl + __dm;
  }

  friend constexpr year_month_weekday_last
  operator+(const year_month_weekday_last &__ymwdl,
            const years &__dy) noexcept {
    return {__ymwdl.year() + __dy, __ymwdl.month(), __ymwdl.weekday_last()};
  }

  friend constexpr year_month_weekday_last
  operator+(const years &__dy,
            const year_month_weekday_last &__ymwdl) noexcept {
    return __ymwdl + __dy;
  }

  template <typename = __detail::__months_years_conversion_disambiguator>
  friend constexpr year_month_weekday_last
  operator-(const year_month_weekday_last &__ymwdl,
            const months &__dm) noexcept {
    return __ymwdl + -__dm;
  }

  friend constexpr year_month_weekday_last
  operator-(const year_month_weekday_last &__ymwdl,
            const years &__dy) noexcept {
    return __ymwdl + -__dy;
  }

  friend constexpr year_month_weekday_last
  operator/(const year_month &__ym,
            const chrono::weekday_last &__wdl) noexcept {
    return {__ym.year(), __ym.month(), __wdl};
  }

  friend constexpr year_month_weekday_last
  operator/(const chrono::year &__y,
            const chrono::month_weekday_last &__mwdl) noexcept {
    return {__y, __mwdl.month(), __mwdl.weekday_last()};
  }

  friend constexpr year_month_weekday_last
  operator/(int __y, const chrono::month_weekday_last &__mwdl) noexcept {
    return chrono::year(__y) / __mwdl;
  }

  friend constexpr year_month_weekday_last
  operator/(const chrono::month_weekday_last &__mwdl,
            const chrono::year &__y) noexcept {
    return __y / __mwdl;
  }

  friend constexpr year_month_weekday_last
  operator/(const chrono::month_weekday_last &__mwdl, int __y) noexcept {
    return chrono::year(__y) / __mwdl;
  }
};

/// @cond undocumented
namespace __detail {
consteval long long __pow10(unsigned __n) {
  long long __r = 1;
  while (__n-- > 0)
    __r *= 10;
  return __r;
}

template <typename _Duration> struct __utc_leap_second;
} // namespace __detail
/// @endcond

/** Utility for splitting a duration into hours, minutes, and seconds
 *
 * This is a convenience type that provides accessors for the constituent
 * parts (hours, minutes, seconds and subseconds) of a duration.
 *
 * @since C++20
 */
template <typename _Duration> class hh_mm_ss {
  static_assert(__is_duration<_Duration>::value);

private:
  static consteval int _S_fractional_width() {
    auto __den = _Duration::period::den;
    const int __multiplicity_2 = std::__countr_zero((uintmax_t)__den);
    __den >>= __multiplicity_2;
    int __multiplicity_5 = 0;
    while ((__den % 5) == 0) {
      ++__multiplicity_5;
      __den /= 5;
    }
    if (__den != 1)
      return 6;

    int __width = (__multiplicity_2 > __multiplicity_5 ? __multiplicity_2
                                                       : __multiplicity_5);
    if (__width > 18)
      __width = 18;
    return __width;
  }

  constexpr hh_mm_ss(_Duration __d, bool __is_neg)
      : _M_h(duration_cast<chrono::hours>(__d)),
        _M_m(duration_cast<chrono::minutes>(__d - hours())),
        _M_s(duration_cast<chrono::seconds>(__d - hours() - minutes())),
        _M_is_neg(__is_neg) {
    auto __ss = __d - hours() - minutes() - seconds();
    if constexpr (treat_as_floating_point_v<typename precision::rep>)
      _M_ss._M_r = __ss.count();
    else if constexpr (precision::period::den != 1)
      _M_ss._M_r = duration_cast<precision>(__ss).count();
  }

  static constexpr _Duration _S_abs(_Duration __d) {
    if constexpr (numeric_limits<typename _Duration::rep>::is_signed)
      return chrono::abs(__d);
    else
      return __d;
  }

public:
  static constexpr unsigned fractional_width = {_S_fractional_width()};

  using precision =
      duration<common_type_t<typename _Duration::rep, chrono::seconds::rep>,
               ratio<1, __detail::__pow10(fractional_width)>>;

  constexpr hh_mm_ss() noexcept = default;

  constexpr explicit hh_mm_ss(_Duration __d)
      : hh_mm_ss(_S_abs(__d), __d < _Duration::zero()) {}

  constexpr bool is_negative() const noexcept {
    if constexpr (!_S_is_unsigned)
      return _M_is_neg;
    else
      return false;
  }

  constexpr chrono::hours hours() const noexcept { return _M_h; }

  constexpr chrono::minutes minutes() const noexcept { return _M_m; }

  constexpr chrono::seconds seconds() const noexcept { return _M_s; }

  constexpr precision subseconds() const noexcept {
    return static_cast<precision>(_M_ss);
  }

  constexpr explicit operator precision() const noexcept {
    return to_duration();
  }

  constexpr precision to_duration() const noexcept {
    if constexpr (!_S_is_unsigned)
      if (_M_is_neg)
        return -(_M_h + _M_m + _M_s + subseconds());
    return _M_h + _M_m + _M_s + subseconds();
  }

private:
  static constexpr bool _S_is_unsigned =
      __and_v<is_integral<typename _Duration::rep>,
              is_unsigned<typename _Duration::rep>>;

  template <typename _Ratio>
  using __byte_duration = duration<unsigned char, _Ratio>;

  // The type of the _M_ss member that holds the subsecond precision.
  template <typename _Dur> struct __subseconds {
    typename _Dur::rep _M_r{};

    constexpr explicit operator _Dur() const noexcept { return _Dur(_M_r); }
  };

  // An empty class if this precision doesn't need subseconds.
  template <typename _Rep>
    requires(!treat_as_floating_point_v<_Rep>)
  struct __subseconds<duration<_Rep, ratio<1>>> {
    constexpr explicit operator duration<_Rep, ratio<1>>() const noexcept {
      return {};
    }
  };

  template <typename _Rep, typename _Period>
    requires(!treat_as_floating_point_v<_Rep>) &&
            ratio_less_v<_Period, ratio<1, 1>> &&
            ratio_greater_equal_v<_Period, ratio<1, 250>>
  struct __subseconds<duration<_Rep, _Period>> {
    unsigned char _M_r{};

    constexpr explicit operator duration<_Rep, _Period>() const noexcept {
      return duration<_Rep, _Period>(_M_r);
    }
  };

  template <typename _Rep, typename _Period>
    requires(!treat_as_floating_point_v<_Rep>) &&
            ratio_less_v<_Period, ratio<1, 250>> &&
            ratio_greater_equal_v<_Period, ratio<1, 4000000000>>
  struct __subseconds<duration<_Rep, _Period>> {
    uint_least32_t _M_r{};

    constexpr explicit operator duration<_Rep, _Period>() const noexcept {
      return duration<_Rep, _Period>(_M_r);
    }
  };

  chrono::hours _M_h{};
  __byte_duration<ratio<60>> _M_m{};
  __byte_duration<ratio<1>> _M_s{};
  bool _M_is_neg{};
  __subseconds<precision> _M_ss{};

  template <typename> friend struct __detail::__utc_leap_second;
};

/// @cond undocumented
namespace __detail {
// Represents a time that is within a leap second insertion.
template <typename _Duration> struct __utc_leap_second {
  explicit __utc_leap_second(const sys_time<_Duration> &__s)
      : _M_date(chrono::floor<days>(__s)), _M_time(__s - _M_date) {
    ++_M_time._M_s;
  }

  sys_days _M_date;
  hh_mm_ss<common_type_t<_Duration, days>> _M_time;
};
} // namespace __detail
/// @endcond

// 12/24 HOURS FUNCTIONS

constexpr bool is_am(const hours &__h) noexcept {
  return chrono::hours(0) <= __h && __h <= chrono::hours(11);
}

constexpr bool is_pm(const hours &__h) noexcept {
  return chrono::hours(12) <= __h && __h <= chrono::hours(23);
}

constexpr hours make12(const hours &__h) noexcept {
  if (__h == chrono::hours(0))
    return chrono::hours(12);
  else if (__h > chrono::hours(12))
    return __h - chrono::hours(12);
  return __h;
}

constexpr hours make24(const hours &__h, bool __is_pm) noexcept {
  if (!__is_pm) {
    if (__h == chrono::hours(12))
      return chrono::hours(0);
    else
      return __h;
  } else {
    if (__h == chrono::hours(12))
      return __h;
    else
      return __h + chrono::hours(12);
  }
}

/// @} group chrono
#endif // C++20
} // namespace chrono

#if __cplusplus >= 202002L
inline namespace literals {
inline namespace chrono_literals {
/// @addtogroup chrono
/// @{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuser-defined-literals"
/// Literal suffix for creating chrono::day objects.
/// @since C++20
constexpr chrono::day operator""d(unsigned long long __d) noexcept {
  return chrono::day{static_cast<unsigned>(__d)};
}

/// Literal suffix for creating chrono::year objects.
/// @since C++20
constexpr chrono::year operator""y(unsigned long long __y) noexcept {
  return chrono::year{static_cast<int>(__y)};
}
#pragma GCC diagnostic pop
/// @}
} // namespace chrono_literals
} // namespace literals
#endif // C++20

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std _GLIBCXX_VISIBILITY(default)

#endif // C++11

#endif //_GLIBCXX_CHRONO
