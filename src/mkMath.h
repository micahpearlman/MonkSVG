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
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace MonkVG {
    typedef glm::f32vec2 v2_t;
    typedef glm::f32vec3 v3_t;
    typedef glm::u16vec2 Coordinate;
    typedef glm::fmat3x3 Matrix33;

	static inline float radians (float degrees) {return (float)(degrees * (M_PI/180.0f));}
	static inline float degrees (float radians) {return (float)(radians * (180.0f/M_PI));}
    
	inline v2_t affineTransform(const Matrix33& m, const v2_t& v )	{
        return v2_t(v[0] * m[0][0] + v[1] * m[0][1] + m[0][2],
                    v[0] * m[1][0] + v[1] * m[1][1] + m[1][2]);
	}
    inline v2_t affineTransform(const Matrix33& m, const float v[2])	{
        return affineTransform(m, glm::make_vec2(v));
    }
}


#endif // __mkMath_h__
