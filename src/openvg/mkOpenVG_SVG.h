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
		void draw();	
	private:
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
		
		VGPath _path;
		VGPaint _fill_paint;
		
		vector<VGPath>	_path_list;
		vector<VGPaint>	_fill_list;
		vector<VGPaint> _stroke_list;
		vector<float>	_stroke_width;
		
		// see: http://www.w3.org/TR/SVG/coords.html#TransformMatrixDefined
		struct transform_abc_t {
			float a, c, e, b, d, f, ff0, ff1, ff2;
			transform_abc_t() {
				a = d = ff2 = 1.0f;
				c, e, b, f, ff0, ff1 = 0;
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
		
		vector<transform_abc_t>	_transforms;
	};
	
}

#endif