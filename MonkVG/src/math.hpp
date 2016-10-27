#ifndef MBGL_UTIL_MATH
#define MBGL_UTIL_MATH

#include <cmath>
#include <array>

#include "vec.hpp"

namespace MonkVG {
namespace util {

template <typename T>
inline T perp(const T& a) {
    return T(-a.y, a.x);
}

template <typename T, typename S1, typename S2>
inline T dist(const S1& a, const S2& b) {
    T dx = b.x - a.x;
    T dy = b.y - a.y;
    T c = std::sqrt(dx * dx + dy * dy);
    return c;
}

// Take the magnitude of vector a.
template <typename T = double, typename S>
inline T mag(const S& a) {
    return std::sqrt(a.x * a.x + a.y * a.y);
}

template <typename S>
inline S unit(const S& a) {
    auto magnitude = mag(a);
    if (magnitude == 0) {
        return a;
    }
    return a * (1 / magnitude);
}

}
}

#endif
