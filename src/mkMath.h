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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace MonkVG {
	
	static inline float radians (float degrees) {return (float)(degrees * (M_PI/180.0f));}
	static inline float degrees (float radians) {return (float)(radians * (180.0f/M_PI));}
	
	//	[ sx	shx	tx
	//	 shy	sy	ty
	//	 0		0	1 ]
	//	sx and sy define scaling in the x and y directions, respectively;
	//	shx and shy define shearing in the x and y directions, respectively;
	//	tx and ty define translation in the x and y directions, respectively.
	
	class Matrix33 {
	public:
		
		union {
            struct {
                float	
				a, c, e,			// cos(a) -sin(a) tx
				b, d, f,			// sin(a) cos(a)  ty
				ff0, ff1, ff2;		// 0      0       1
            };
            float m[9];
            float mm[3][3];
        };
		
		Matrix33( float* t ) {
            a = t[0]; c = t[1]; e = t[2];
            b = t[3]; d = t[4]; f = t[5];
            ff0 = t[6]; ff1 = t[7];  ff2 = t[8];
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
		
		void translation( float t[2] ) const { t[0] = e; t[1] = f; }
		void setTranslation( float t[2] ) { e = t[0]; f = t[1]; }
		float angle() const { return acosf( a ); }
		void setAngle( float ang ) {
			setRotation( ang );
		}
		
		void lookAt( float la[2] ) const { la[0] = a; la[1] = b; }
		
        
        float* ptr() {
            return &a;
        }
        
        static void multiply( Matrix33& r, const Matrix33& a, const Matrix33& b ) {
            for ( int z = 0; z < 9; z++ )
                r.m[z] = 0;
            
            for( int i = 0; i < 3; i++ ) 
                for( int j = 0; j < 3; j++ ) 
                    for ( int k = 0; k < 3; k++ ) {
                        r.mm[i][j] += a.mm[i][k] * b.mm[k][j];
                    }
        }
		
		
		Matrix33() {
			setIdentity();
		}
		
		Matrix33( const Matrix33 &m ) {
			this->copy( m );
		}
		
		inline void set( int row, int col, float v ) {
			mm[row][col] = v; 
		}
		
		inline float get( int row, int col ) const {
			return mm[row][col];
		}
		
		inline void copy( const Matrix33& m_ ) {
			for( int i = 0; i < 9; i++ )
				m[i] = m_.m[i];
			
		}
		inline void copy( float* o ) {
			for( int i = 0; i < 9; i++ )
				m[i] = o[i];
		}
		inline void transpose( ) {
			Matrix33 tmp;
			for( int i = 0; i < 3; i++ )
				for( int k = 0; k < 3; k++ )
					tmp.set( i, k, get( k, i ) );
			copy( tmp );
		}
		inline void postMultiply( const Matrix33& m ) {
			Matrix33 tmp;
			Matrix33::multiply( tmp, *this, m );
			copy( tmp );
		}
		
		inline void preMultiply( const Matrix33& m ) {
			Matrix33 tmp;
			Matrix33::multiply( tmp, m, *this );
			copy( tmp );
		}
		
	};
	
	inline void affineTransform( float out[2], const Matrix33& m, const float v[2] )	{
		out[0] = v[0] * m.get(0,0) + v[1] * m.get(0,1) + m.get(0,2);
		out[1] = v[0] * m.get(1,0) + v[1] * m.get(1,1) + m.get(1,2);
	}
    inline void affineTransform( const Matrix33& m, float v[2] )	{
        const float in[2] = {v[0],v[1]};
        affineTransform(v, m, in);
    }
	
}


#endif // __mkMath_h__
