/*
 *  svg.h
 *  MonkVG-Test-iOS-OpenGL
 *
 *  Created by Micah Pearlman on 8/12/10.
 *  Copyright 2010 Zero Vision. All rights reserved.
 *
 */

#ifndef __SVG_H__
#define __SVG_H__
#include <vg/openvg.h>
#include <vg/vgu.h>
#include <vector>
#include <cmath>
#include "mkSVG.h"


using namespace std;

namespace MonkSVG {
	class OpenVG_SVGHandler : public ISVGHandler {
		
	public:
		
		OpenVG_SVGHandler()
		:	ISVGHandler()
		,	_mode( kGroupParseMode )
		,	_current_group( &_root_group ) 
		{}
		
		void draw();
		
	private:
		
		// groups
		virtual void onGroupBegin();
		virtual void onGroupEnd();
		
		// paths
		virtual void onPathBegin();
		virtual void onPathEnd();
		virtual void onPathMoveTo( float x, float y );
		virtual void onPathLineTo( float x, float y );
		virtual void onPathCubic( float x1, float y1, float x2, float y2, float x3, float y3 );
		virtual void onPathFillColor( unsigned int color );
		virtual void onPathStrokeColor( unsigned int color );
		virtual void onPathStrokeWidth( float width );
		
		// transforms 
		virtual void onTransformTranslate( float x, float y );
		virtual void onTransformScale( float s );
		virtual void onTransformRotate( float r );
		virtual void onTransformMatrix( float a, float b, float c, float d, float e, float f );
		
		
		uint32_t openVGRelative() {
			if ( relative() ) {
				return VG_RELATIVE;
			}
			return VG_ABSOLUTE;
		}
	private:
		
		// see: http://www.w3.org/TR/SVG/coords.html#TransformMatrixDefined
		struct transform_abc_t {
			union {
				struct {
					float a, c, e, b, d, f, ff0, ff1, ff2;
				};
				float m[9];
				float mm[3][3];
			};
			transform_abc_t() {
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
			
			transform_abc_t( float* t ) {
				a = t[0]; c = t[1]; e = t[2];
				b = t[3]; d = t[4]; f = t[5];
				ff0 = t[6]; ff1 = t[7];  ff2 = t[8];
			}
			
			static void multiply( transform_abc_t& r, transform_abc_t& a, transform_abc_t& b ) {
				for ( int z = 0; z < 9; z++ )
					r.m[z] = 0;
				
				for( int i = 0; i < 3; i++ ) 
					for( int j = 0; j < 3; j++ ) 
						for ( int k = 0; k < 3; k++ ) {
							r.mm[i][j] += a.mm[i][k] * b.mm[k][j];
						}
			}
		};
		
		void pushTransform( transform_abc_t& t ) {
			transform_abc_t& current_tranform = _transform_stack.back();
			transform_abc_t top_transform;
			transform_abc_t::multiply( top_transform, t, current_tranform );
			_transform_stack.push_back( top_transform );
			vgLoadMatrix( top_transform.m );
		}
		
		void popTransform() {
			_transform_stack.pop_back();
			transform_abc_t& top = _transform_stack.back();
			vgLoadMatrix( top.m );
		}
		
		struct path_object_t {
			VGPath path;
			VGPaint fill;
			VGPaint stroke;
			VGfloat stroke_width;
			transform_abc_t transform;
			
			path_object_t() : path( 0 ), fill( 0 ), stroke( 0 ), stroke_width( 0 ) {
				
			}
		};
		
		struct group_t {
			group_t() : parent( 0 ){
				
			}
			transform_abc_t transform;
			group_t* parent;
			vector<group_t> children;
			vector<path_object_t> path_objects;
			path_object_t current_path;
		};
		
		group_t		_root_group;
		group_t*	_current_group;
		
		vector<transform_abc_t> _transform_stack;		
		
		enum mode {
			kGroupParseMode = 1,
			kPathParseMode = 2
		};
		
		mode _mode;
		
	private:
		
		void draw_recursive( OpenVG_SVGHandler::group_t& group );
	};
	
}

#endif