/*
mapbox-gl-native copyright (c) 2014-2016 Mapbox.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MBGL_UTIL_VEC
#define MBGL_UTIL_VEC

#include <limits>
#include <type_traits>
#include <cmath>
#include <cstdint>
#include <array>

namespace MonkVG {

template <typename T = double>
struct vec2 {
    struct null {};
    typedef T Type;

    T x, y;

    inline vec2() {}

    template<typename U = T, typename std::enable_if<std::numeric_limits<U>::has_quiet_NaN, int>::type = 0>
    inline vec2(null) : x(std::numeric_limits<T>::quiet_NaN()), y(std::numeric_limits<T>::quiet_NaN()) {}

    template<typename U = T, typename std::enable_if<!std::numeric_limits<U>::has_quiet_NaN, int>::type = 0>
    inline vec2(null) : x(std::numeric_limits<T>::min()), y(std::numeric_limits<T>::min()) {}

    inline vec2(const vec2& o) : x(o.x), y(o.y) {}

    template<typename U>
    inline vec2(const U& u) : x(u.x), y(u.y) {}

    inline vec2(T x_, T y_) : x(x_), y(y_) {}

    inline bool operator==(const vec2& rhs) const {
        return x == rhs.x && y == rhs.y;
    }

    template <typename O>
    inline typename std::enable_if<std::is_arithmetic<O>::value, vec2>::type
    operator*(O o) const {
        return {x * o, y * o};
    }

    template <typename O>
    inline typename std::enable_if<std::is_arithmetic<O>::value, vec2>::type &
    operator*=(O o) {
        x *= o;
        y *= o;
    }

    inline vec2<T> operator *(const std::array<float, 16>& matrix) {
        return { x * matrix[0] + y * matrix[4] + matrix[12], x * matrix[1] + y * matrix[5] + matrix[13] };
    }

    template <typename O>
    inline typename std::enable_if<std::is_arithmetic<O>::value, vec2>::type
    operator-(O o) const {
        return {x - o, y - o};
    }

    template <typename O>
    inline typename std::enable_if<!std::is_arithmetic<O>::value, vec2>::type
    operator-(const O &o) const {
        return vec2<T>(x - o.x, y - o.y);
    }

    template <typename O>
    inline typename std::enable_if<!std::is_arithmetic<O>::value, vec2>::type
    operator+(const O &o) const {
        return vec2<T>(x + o.x, y + o.y);
    }

    template <typename M>
    inline vec2 matMul(const M &m) const {
        return {m[0] * x + m[1] * y, m[2] * x + m[3] * y};
    }

    template<typename U = T, typename std::enable_if<std::numeric_limits<U>::has_quiet_NaN, int>::type = 0>
    inline operator bool() const {
        return !std::isnan(x) && !std::isnan(y);
    }

    template<typename U = T, typename std::enable_if<!std::numeric_limits<U>::has_quiet_NaN, int>::type = 0>
    inline operator bool() const {
        return x != std::numeric_limits<T>::min() && y != std::numeric_limits<T>::min();
    }
};

template <typename T = double>
struct vec3 {
    T x, y, z;

    inline vec3() {}
    inline vec3(const vec3& o) : x(o.x), y(o.y), z(o.z) {}
    inline vec3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}
    inline bool operator==(const vec3& rhs) const {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
};

typedef vec2<int16_t> Coordinate;

}

#endif
