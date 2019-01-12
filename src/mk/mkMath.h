/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */

#ifndef __mkMath_h__
#define __mkMath_h__

#include <cmath>
#define GLM_PRECISION_LOWP_FLOAT
#define GLM_PRECISION_LOWP_UINT
#include "glm/glm/glm/glm.hpp"
#include "glm/glm/glm/gtc/type_ptr.hpp"
#include "glm/glm/glm/gtx/matrix_transform_2d.hpp"
#include "glm/glm/glm/gtx/hash.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace MonkVG {
    template <typename T, typename U>
    constexpr std::size_t stdOffset(U T::*v) noexcept
    {
        T* ptr = nullptr;
        U* arg = &(ptr->*v);
        return reinterpret_cast<std::size_t>(arg) - reinterpret_cast<std::size_t>(ptr);
    }

    // See https://stackoverflow.com/a/17728525/1060383
    constexpr int64_t ipow(int64_t base, int exp, int64_t result = 1) {
        return exp < 1 ? result : ipow(base*base, exp/2, (exp % 2) ? result*base : result);
    }

    typedef glm::f32 value_t;
    typedef glm::f32vec2 v2_t;
    typedef glm::f32vec2 Pos;       // i32vec2
    typedef glm::f32vec2 GpuPos;    // u16vec2
    typedef glm::fmat3x3 Matrix33;
    typedef glm::u8vec4 Color;
    
    struct __attribute__((packed)) positionData_t {
        GpuPos position;
        
        enum
        {
            k_position = stdOffset(&positionData_t::position)
        };

        positionData_t() {}
        positionData_t(const decltype(position)& _position) : position(_position) {}
    };
    
    struct __attribute__((packed)) vertexData_t : public positionData_t {
        GpuPos quad;
        Color color;
        
        enum
        {
            k_quad = stdOffset(&vertexData_t::quad),
            k_color = stdOffset(&vertexData_t::color)
        };

        vertexData_t() {}
        vertexData_t(const decltype(position)& _position,
                     const decltype(color)& _color)
        : positionData_t(_position), quad(_position), color(_color)  {}
        
        vertexData_t(const decltype(position)& _position,
                     const decltype(quad)& _quad,
                     const decltype(color)& _color)
        : positionData_t(_position), quad(_quad), color(_color) {}

        bool operator <(const vertexData_t& other) const
        {
            return position.x < other.position.x ||
            (position.x == other.position.x && (position.y < other.position.y ||
            (position.y == other.position.y && (quad.x < other.quad.x ||
            (quad.x == other.quad.x && (quad.y < other.quad.y ||
            (quad.y == other.quad.y && (*(const uint32_t*)&color < *(const uint32_t*)&other.color))))))));
        }
    };

    struct vertexDataPtrLessThan
    {
        inline bool operator()(const MonkVG::vertexData_t* const& lhs, const MonkVG::vertexData_t* const& rhs) const
        {
            return *lhs < *rhs;
        }
    };
    

    using ebo_t = u_int32_t;
    
    using addResult_t = std::pair<positionData_t*, ebo_t>;
    
    
	static inline float radians (float degrees) {return (float)(degrees * (M_PI/180.0f));}
	static inline float degrees (float radians) {return (float)(radians * (180.0f/M_PI));}
    
	inline v2_t affineTransform(const Matrix33& m, const v2_t& v )	{
        return v2_t(v[0] * m[0][0] + v[1] * m[0][1] + m[0][2],
                    v[0] * m[1][0] + v[1] * m[1][1] + m[1][2]);
	}
    inline v2_t affineTransform(const Matrix33& m, const float v[2])	{
        return affineTransform(m, glm::make_vec2(v));
    }
    inline void translate(Matrix33& m, float x, float y)
    {
        const v2_t v(x,y);
        m = glm::translate(m, v);
    }
    inline void rotate(Matrix33& m, float angle)
    {
        m = glm::rotate(m, angle);
    }
    inline void scale(Matrix33& m, float x, float y)
    {
        const v2_t v(x,y);
        m = glm::scale(m, v);
    }
    template <typename T>
    inline T perp(const T& a) {
        return T(-a.y, a.x);
    }
    
    template <typename T>
    inline typename T::value_type mag(const T& v) {
        return glm::sqrt(glm::dot(v, v));
    }
}

#endif // __mkMath_h__
