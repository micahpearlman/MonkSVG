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
	};
	
}

#endif