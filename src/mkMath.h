/*
 *  mkMath.h
 *  MonkVG-Quartz
 *
 *  Created by Micah Pearlman on 3/11/09.
 *  Copyright 2009 Monk Games. All rights reserved.
 *
 */

#ifndef __mkMath_h__
#define __mkMath_h__

#include <cmath>
#define GLM_PRECISION_LOWP_FLOAT
#define GLM_PRECISION_LOWP_UINT
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/hash.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace MonkVG {
    typedef glm::f32vec2 v2_t;
    typedef glm::f32vec3 v3_t;
    typedef glm::i32vec2 Pos;
    typedef glm::u16vec2 GpuPos;
    typedef glm::fmat3x3 Matrix33;
    typedef glm::u8vec4 Color;

	static inline float radians (float degrees) {return (float)(degrees * (M_PI/180.0f));}
	static inline float degrees (float radians) {return (float)(radians * (180.0f/M_PI));}
    
	inline v2_t affineTransform(const Matrix33& m, const v2_t& v )	{
        return v2_t(v[0] * m[0][0] + v[1] * m[0][1] + m[0][2],
                    v[0] * m[1][0] + v[1] * m[1][1] + m[1][2]);
	}
    inline v2_t affineTransform(const Matrix33& m, const float v[2])	{
        return affineTransform(m, glm::make_vec2(v));
    }
    void translate(Matrix33& m, float x, float y)
    {
        const v2_t v(x,y);
        m = glm::translate(m, v);
    }
    void rotate(Matrix33& m, float angle)
    {
        m = glm::rotate(m, angle);
    }
    void scale(Matrix33& m, float x, float y)
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
