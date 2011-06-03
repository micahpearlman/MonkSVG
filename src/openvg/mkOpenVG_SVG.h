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
#include <list>
#include <cmath>
#include "mkSVG.h"
#include "mkTransform2d.h"


using namespace std;

namespace MonkSVG {
	class OpenVG_SVGHandler : public ISVGHandler {
		
	public:
		
		OpenVG_SVGHandler();
		
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
		virtual void onPathArc( float rx, float ry, float x_axis_rotation, int large_arc_flag, int sweep_flag, float x, float y );

		
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
		
		
		void pushTransform( Transform2d& t ) {
			Transform2d& current_tranform = _transform_stack.back();
			Transform2d top_transform;
			Transform2d::multiply( top_transform, t, current_tranform );
			_transform_stack.push_back( top_transform );
			vgLoadMatrix( top_transform.m );
		}
		
		void popTransform() {
			_transform_stack.pop_back();
			Transform2d& top = _transform_stack.back();
			vgLoadMatrix( top.m );
		}
		
		struct path_object_t {
			VGPath path;
			VGPaint fill;
			VGPaint stroke;
			VGfloat stroke_width;
			Transform2d transform;
			
			path_object_t() : path( 0 ), fill( 0 ), stroke( 0 ), stroke_width( 0 ) {
				
			}
		};
		
		struct group_t {
			group_t() : parent( 0 ){
				
			}
			Transform2d transform;
			group_t* parent;
			list<group_t> children;
			list<path_object_t> path_objects;
			path_object_t current_path;
		};
		
		group_t		_root_group;
		group_t*	_current_group;
		
		vector<Transform2d> _transform_stack;		
		
		enum mode {
			kGroupParseMode = 1,
			kPathParseMode = 2
		};
		
		mode _mode;
		
		VGPaint	_blackBackFill;		// if a path doesn't have a stroke or a fill then use this fill
		
	private:
		
		void draw_recursive( OpenVG_SVGHandler::group_t& group );
	};
	
}

#endif