//
//  mkTransform2d.h
//  MonkSVG-OSX
//
//  Created by Micah Pearlman on 4/3/11.
//  Copyright 2011 Zero Vision. All rights reserved.
//

namespace MonkSVG {
    // see: http://www.w3.org/TR/SVG/coords.html#TransformMatrixDefined
    struct Transform2d {
        union {
            struct {
                float a, c, e, b, d, f, ff0, ff1, ff2;
            };
            float m[9];
            float mm[3][3];
        };
        Transform2d() {
            // set to identity
            a = d = ff2 = 1.0f;
            c = e = b = f = ff0 = ff1 = 0;
        }
        
        void setTranslate( float x, float y ) {
            e = x; f = y;
        }
        
        void setScale( float sx, float sy ) {
            a = sx;
            d = sy;
        }
        
        void setRotation( float a ) {	// assume radians
            a = cosf( a ); c = -sinf( a );
            b = sinf( a ); d = cosf( a );
        }
        
        float* ptr() {
            return &a;
        }
        
        Transform2d( float* t ) {
            a = t[0]; c = t[1]; e = t[2];
            b = t[3]; d = t[4]; f = t[5];
            ff0 = t[6]; ff1 = t[7];  ff2 = t[8];
        }
        
        static void multiply( Transform2d& r, Transform2d& a, Transform2d& b ) {
            for ( int z = 0; z < 9; z++ )
                r.m[z] = 0;
            
            for( int i = 0; i < 3; i++ ) 
                for( int j = 0; j < 3; j++ ) 
                    for ( int k = 0; k < 3; k++ ) {
                        r.mm[i][j] += a.mm[i][k] * b.mm[k][j];
                    }
        }
    };

}