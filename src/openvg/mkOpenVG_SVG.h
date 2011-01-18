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
			float a, c, e, b, d, f, ff0, ff1, ff2;
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
		};
		
		
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
		
		//path_object_t _current_path;
		//vector<path_object_t> _path_objects;
		
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