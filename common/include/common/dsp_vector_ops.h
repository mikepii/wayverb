#pragma once

#include "stl_wrappers.h"

#include <numeric>
#include <vector>
#include <cmath>

template <typename T>
inline float sum(const T& t) {
    return proc::accumulate(
            t, 0.0f, [](const auto& a, const auto& b) { return a + sum(b); });
}

template <>
inline float sum(const float& t) {
    return t;
}

template <typename T>
inline auto count(const T& t) {
    return proc::accumulate(
            t, 0u, [](const auto& a, const auto& b) { return a + count(b); });
}

template <typename T, typename Allocator>
inline auto count(const std::vector<T, Allocator>& coll) {
    return coll.size();
}

template <typename T>
inline auto mean(const T& t) {
    return sum(t) / count(t);
}

template <typename T>
inline float max_mag(const T& t) {
    return proc::accumulate(t, 0.0f, [](auto a, auto b) {
        using std::max;
        return max(a, max_mag(b));
    });
}

template <>
inline float max_mag(const float& t) {
    using std::fabs;
    return fabs(t);
}

/// Recursively divide by reference.
template <typename T>
inline void div(T& ret, float f) {
    for (auto& i : ret) {
        div(i, f);
    }
}

/// The base case of the div recursion.
template <>
inline void div(float& ret, float f) {
    ret /= f;
}

/// Recursively multiply by reference.
template <typename T>
inline void mul(T& ret, float f) {
    for (auto& i : ret) {
        mul(i, f);
    }
}

/// The base case of the mul recursion.
template <>
inline void mul(float& ret, float f) {
    ret *= f;
}

/// Find the largest absolute value in an arbitarily nested vector, then
/// divide every item in the vector by that value.
template <typename T>
inline void normalize(T& ret) {
    if (const auto mag{max_mag(ret)}) {
        mul(ret, 1.0 / mag);
    }
}

template <typename T>
inline void kernel_normalize(T& ret) {
    const auto sum{std::abs(proc::accumulate(ret, 0.0))};
    if (sum) {
        mul(ret,
            1.0 / sum - std::numeric_limits<typename T::value_type>::epsilon());
    }
}
