// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

// Very hacky :)
#ifdef __APPLE__

#include <bit>
#include <concepts>

#include "common/apple_compat/condition_variable_any2.hpp"
#include "common/apple_compat/jthread.hpp"

#include <range/v3/all.hpp>

// use an external library range-v3 as std's one
namespace std::ranges {
using namespace ::ranges;
}

// adaptation of new features in c++20
namespace std {
constexpr unsigned long long min(const unsigned long long a, const unsigned long long b) {
    return (b < a) ? b : a;
}

template <std::unsigned_integral T>
/*    requires !std::custom_same_as<T, bool> && !std::custom_same_as<T, char> &&
             !std::custom_same_as<T, char8_t> && !std::custom_same_as<T, char16_t> &&
             !std::custom_same_as<T, char32_t> && !std::custom_same_as<T, wchar_t>*/
constexpr bool has_single_bit(T x) noexcept {
    return x != 0 && (x & (x - 1)) == 0;
}

using condition_variable_any_apple = std::condition_variable_any2;
} // namespace std

// fix sorting with constexpr funcs
// thanks to https://tristanbrindle.com/posts/a-more-useful-compile-time-quicksort
namespace constexprSort {
namespace cstd {
template <typename RAIt>
constexpr RAIt next(RAIt it, typename std::iterator_traits<RAIt>::difference_type n = 1) {
    return it + n;
}

template <typename RAIt>
constexpr auto distance(RAIt first, RAIt last) {
    return last - first;
}

template <class ForwardIt1, class ForwardIt2>
constexpr void iter_swap(ForwardIt1 a, ForwardIt2 b) {
    auto temp = std::move(*a);
    *a = std::move(*b);
    *b = std::move(temp);
}

template <class InputIt, class UnaryPredicate>
constexpr InputIt find_if_not(InputIt first, InputIt last, UnaryPredicate q) {
    for (; first != last; ++first) {
        if (!q(*first)) {
            return first;
        }
    }
    return last;
}

template <class ForwardIt, class UnaryPredicate>
constexpr ForwardIt partition(ForwardIt first, ForwardIt last, UnaryPredicate p) {
    first = cstd::find_if_not(first, last, p);
    if (first == last)
        return first;

    for (ForwardIt i = cstd::next(first); i != last; ++i) {
        if (p(*i)) {
            cstd::iter_swap(i, first);
            ++first;
        }
    }
    return first;
}

} // namespace cstd

template <class RAIt, class Compare = std::less<>>
constexpr void quick_sort(RAIt first, RAIt last, Compare cmp = Compare{}) {
    auto const N = cstd::distance(first, last);
    if (N <= 1)
        return;
    auto const pivot = *cstd::next(first, N / 2);
    auto const middle1 =
        cstd::partition(first, last, [=](auto const& elem) { return cmp(elem, pivot); });
    auto const middle2 =
        cstd::partition(middle1, last, [=](auto const& elem) { return !cmp(pivot, elem); });
    quick_sort(first, middle1, cmp); // assert(std::is_sorted(first, middle1, cmp));
    quick_sort(middle2, last, cmp);  // assert(std::is_sorted(middle2, last, cmp));
}

template <typename Range>
constexpr auto sort(Range&& range) {
    quick_sort(std::begin(range), std::end(range));
    return range;
}
} // namespace constexprSort
#endif