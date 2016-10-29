/*
mapbox-gl-native copyright (c) 2014-2016 Mapbox.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
