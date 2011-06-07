//
//  mkTransform2d.h
//  MonkSVG-OSX
//
//  Created by Micah Pearlman on 4/3/11.
//  Copyright 2011 Zero Vision. All rights reserved.
//
#ifndef __mkTransform2d_h__
#define __mkTransform2d_h__

#include <cmath>
#include <iostream>
#include <iomanip>

namespace MonkSVG {
    // see: http://www.w3.org/TR/SVG/coords.html#TransformMatrixDefined
    struct Transform2d {
        union {
            struct {
                float	a, c, e,		// cos(a) -sin(a) tx
						b, d, f,		// sin(a) cos(a)  ty
						ff0, ff1, ff2;	// 0      0       1
            };
            float m[9];
            float mm[3][3];
        };
        Transform2d() {
			setIdentity();
        }
		
		void setIdentity() {
            // set to identity
            a = d = ff2 = 1.0f;
            c = e = b = f = ff0 = ff1 = 0;
		}

        
        void setTranslate( float x, float y ) {
            e = x; f = y;
        }
        
        void setScale( float sx, float sy ) {
            a = sx; d = sy;
        }
        
        void setRotation( float ang ) {	// assume radians
			float cs = cosf( ang );
			float ss = sinf( ang );
            a = cs; c = -ss;
            b = ss; d = cs;
        }
		
		void translation( float t[2] ) { t[0] = e; t[1] = f; }
		
        
        float* ptr() {
            return &a;
        }
        
		
        Transform2d( float* t ) {
            a = t[0]; c = t[1]; e = t[2];
            b = t[3]; d = t[4]; f = t[5];
            ff0 = t[6]; ff1 = t[7];  ff2 = t[8];
        }
        
        static void multiply( Transform2d& r, const Transform2d& a, const Transform2d& b ) {
            for ( int z = 0; z < 9; z++ )
                r.m[z] = 0;
            
            for( int i = 0; i < 3; i++ ) 
                for( int j = 0; j < 3; j++ ) 
                    for ( int k = 0; k < 3; k++ ) {
                        r.mm[i][j] += a.mm[i][k] * b.mm[k][j];
                    }
        }
		
		void print() {
			std::cout << ":: Transform2d ::" << std::endl;
			for( int i = 0; i < 3; i++ ) {
				for( int p = 0; p < 3; p++ ) {
					std::cout << std::setw(6) << std::setiosflags(std::ios::fixed) << std::setprecision(3) << mm[i][p];
				}
				std::cout << std::endl;
			}
		}
    };

}

#endif // __mkTransform2d_h__
