/*
 *  svg.cpp
 *  MonkVG--iOS-OpenGL
 *
 *  Created by Micah Pearlman on 8/12/10.
 *  Copyright 2010 Zero Vision. All rights reserved.
 *
 */

#include "mkOpenVG_SVG.h"

namespace MonkSVG {
	
	OpenVG_SVGHandler::OpenVG_SVGHandler()
	:	ISVGHandler()
	,	_mode( kGroupParseMode )
	,	_current_group( &_root_group ) 
	{
		_blackBackFill = vgCreatePaint();
		VGfloat fcolor[4] = { 0,0,0,1 };
		vgSetParameterfv( _blackBackFill, VG_PAINT_COLOR, 4, &fcolor[0]);

		
	}
	void OpenVG_SVGHandler::draw() {
		float m[9];
		vgGetMatrix( m );
		_transform_stack.push_back( Transform2d( m ) );	// set the current matrix to top of the matrix stack
		draw_recursive( _root_group );
		vgLoadMatrix( m );	// restore matrix
	}
	
	void OpenVG_SVGHandler::draw_recursive( group_t& group ) {
		
		// push the group matrix onto the stack
		pushTransform( group.transform );
		for ( list<path_object_t>::iterator it = group.path_objects.begin(); it != group.path_objects.end(); it++ ) {
			path_object_t& po = *it;
			uint32_t draw_params = 0;
			if ( po.fill ) {
				vgSetPaint( po.fill, VG_FILL_PATH );
				draw_params |= VG_FILL_PATH;
			}
			
			if ( po.stroke ) {
				vgSetPaint( po.stroke, VG_STROKE_PATH );
				vgSetf( VG_STROKE_LINE_WIDTH, po.stroke_width );
				draw_params |= VG_STROKE_PATH;
			}
			
			if( draw_params == 0 ) {	// if no stroke or fill use the default black fill
				vgSetPaint( _blackBackFill, VG_FILL_PATH );
				draw_params |= VG_FILL_PATH;
			}
			//vgMultMatrix( po.transform.ptr() );
			pushTransform( po.transform );
			vgDrawPath( po.path, draw_params );
			popTransform();
		}
		
		for ( list<group_t>::iterator it = group.children.begin(); it != group.children.end(); it++ ) {
			draw_recursive( *it );
		}
		
		popTransform();
	}
	
	void OpenVG_SVGHandler::onGroupBegin() {
		_mode = kGroupParseMode;
		_current_group->children.push_back( group_t() );
		group_t* top = &_current_group->children.back();
		top->parent = _current_group;
		_current_group = top;
	}
	void OpenVG_SVGHandler::onGroupEnd() {
		_current_group = _current_group->parent;
	}

	
	void OpenVG_SVGHandler::onPathBegin() { 
		_mode = kPathParseMode;
		_current_group->current_path = path_object_t();
		_current_group->current_path.path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
							 1,0,0,0, VG_PATH_CAPABILITY_ALL);
		
	}
	
	void OpenVG_SVGHandler::onPathEnd() {  
		VGubyte seg = VG_CLOSE_PATH;
		VGfloat data = 0.0f;
		vgAppendPathData( _current_group->current_path.path, 1, &seg, &data );
		
		
		
		_current_group->path_objects.push_back( _current_group->current_path );
		
		// build up the bounds
		VGfloat minX, minY, width, height;
		vgPathBounds( _current_group->current_path.path, &minX, &minY, &width, &height );
		if ( minX < _minX ) {
			_minX = minX;
		}
		if ( minY < _minY ) {
			_minY = minY;
		}
		if ( width > _width ) {
			_width = width;
		}
		if ( height > _height ) {
			_height = height;
		}
		
	}
	
	void OpenVG_SVGHandler::onPathMoveTo( float x, float y ) { 
		VGubyte seg = VG_MOVE_TO | openVGRelative();
		VGfloat data[2];
		
		data[0] = x; data[1] = y;
		vgAppendPathData( _current_group->current_path.path, 1, &seg, data );
		
	}
	void OpenVG_SVGHandler::onPathLineTo( float x, float y ) { 
		VGubyte seg = VG_LINE_TO | openVGRelative();
		VGfloat data[2];
		
		data[0] = x; data[1] = y;
		vgAppendPathData( _current_group->current_path.path, 1, &seg, data );
		
	}
	void OpenVG_SVGHandler::onPathCubic( float x1, float y1, float x2, float y2, float x3, float y3 ) { 
		VGubyte seg = VG_CUBIC_TO | openVGRelative();
		VGfloat data[6];
		
		data[0] = x1; data[1] = y1;
		data[2] = x2; data[3] = y2;
		data[4] = x3; data[5] = y3;
		vgAppendPathData( _current_group->current_path.path, 1, &seg, data);
		
	}
	
	void OpenVG_SVGHandler::onPathArc( float rx, float ry, float x_axis_rotation, int large_arc_flag, int sweep_flag, float x, float y ) {
		
		VGubyte seg = openVGRelative();
		if ( large_arc_flag ) {
			if (sweep_flag) {
				seg |= VG_LCCWARC_TO;
			} else {
				seg |= VG_LCWARC_TO;
			}
		} else {
			if (sweep_flag) {
				seg |= VG_SCCWARC_TO;
			} else {
				seg |= VG_SCWARC_TO;
			}
			
		}
		VGfloat data[5];
		
	
		
		data[0] = rx;
		data[1] = ry;
		data[2] = x_axis_rotation;
		data[3] = x;
		data[4] = y;
		
		vgAppendPathData( _current_group->current_path.path, 1, &seg, data);
		
	}

	
	void OpenVG_SVGHandler::onPathFillColor( unsigned int color ) {
		_current_group->current_path.fill = vgCreatePaint();
		VGfloat fcolor[4] = { VGfloat( (color & 0xff000000) >> 24)/255.0f, 
			VGfloat( (color & 0x00ff0000) >> 16)/255.0f, 
			VGfloat( (color & 0x0000ff00) >> 8)/255.0f, 
			1.0f /*VGfloat(color & 0x000000ff)/255.0f*/ };
		vgSetParameterfv( _current_group->current_path.fill, VG_PAINT_COLOR, 4, &fcolor[0]);
	}
	void OpenVG_SVGHandler::onPathStrokeColor( unsigned int color ) {
		_current_group->current_path.stroke = vgCreatePaint();
		VGfloat fcolor[4] = { VGfloat( (color & 0xff000000) >> 24)/255.0f, 
			VGfloat( (color & 0x00ff0000) >> 16)/255.0f, 
			VGfloat( (color & 0x0000ff00) >> 8)/255.0f, 
			1.0f /*VGfloat(color & 0x000000ff)/255.0f*/ };
		vgSetParameterfv( _current_group->current_path.stroke, VG_PAINT_COLOR, 4, &fcolor[0]);
	}
	void OpenVG_SVGHandler::onPathStrokeWidth( float width ) {
		_current_group->current_path.stroke_width = width;
	}
	
	void OpenVG_SVGHandler::onTransformTranslate( float x, float y ) {
		_current_group->current_path.transform.setTranslate( x, y );
	}
	void OpenVG_SVGHandler::onTransformScale( float s ) {
		_current_group->current_path.transform.setScale( s, s );
	}
	void OpenVG_SVGHandler::onTransformRotate( float r ) {
		_current_group->current_path.transform.setRotation( r );	// ?? radians or degrees ??
	}
	void OpenVG_SVGHandler::onTransformMatrix( float a, float b, float c, float d, float e, float f ) {
		Transform2d t;
		t.a = a; t.b = b; t.c = c; t.d = d; t.e = e; t.f = f;
		_current_group->current_path.transform = t;
	}
	
}
