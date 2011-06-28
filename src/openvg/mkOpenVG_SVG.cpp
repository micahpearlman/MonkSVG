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
	,	_blackBackFill( 0 )
	,	_batch( 0 )
	{
		_blackBackFill = vgCreatePaint();
		VGfloat fcolor[4] = { 0,0,0,1 };
		vgSetParameterfv( _blackBackFill, VG_PAINT_COLOR, 4, &fcolor[0]);
		_use_transform.setIdentity();

		//_root_transform.setScale( 1, -1 );
		
	}
	
	OpenVG_SVGHandler::~OpenVG_SVGHandler() {
		vgDestroyPaint( _blackBackFill );
		_blackBackFill = 0;
		
		if( _batch ) {
			vgDestroyBatchMNK( _batch );
			_batch = 0;
		}

	}
	
	
	void OpenVG_SVGHandler::draw() {
		
		// clear out the transform stack
		_transform_stack.clear();
		
		float m[9];
		vgGetMatrix( m );
		// assume the current openvg matrix is like the camera matrix and should always be applied first
		Transform2d top;
		Transform2d::multiply( top, Transform2d(m), rootTransform() );	// multiply by the root transform
		pushTransform( top );
		
		// SVG is origin at the top, left (openvg is origin at the bottom, left)
		// so need to flip
//		Transform2d flip;
//		flip.setScale( 1, -1 );
//		pushTransform( flip );
		
		if( _batch ) {
			vgDrawBatchMNK( _batch );
		} else {
			draw_recursive( _root_group );
		}
		
		vgLoadMatrix( m );	// restore matrix
		_transform_stack.clear();
	}
	
	void OpenVG_SVGHandler::draw_recursive( group_t& group ) {
		
		// push the group matrix onto the stack
		pushTransform( group.transform ); vgLoadMatrix( topTransform().m );
		
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
			
			// set the fill rule
			vgSeti( VG_FILL_RULE, po.fill_rule );
			// trasnform
			pushTransform( po.transform );	vgLoadMatrix( topTransform().m );
			vgDrawPath( po.path, draw_params );
			popTransform();	vgLoadMatrix( topTransform().m );
		}
		
		for ( list<group_t>::iterator it = group.children.begin(); it != group.children.end(); it++ ) {
			draw_recursive( *it );
		}
		
		popTransform();	vgLoadMatrix( topTransform().m );
	}
	
	void OpenVG_SVGHandler::optimize() {
		if( _batch ) {
			vgDestroyBatchMNK( _batch );
			_batch = 0;
		}
		// use the monkvg batch extension to greatly optimize rendering.  don't need this for
		// other OpenVG implementations
		_batch = vgCreateBatchMNK();
		
		vgBeginBatchMNK( _batch ); { // draw

			// clear out the transform stack
			_transform_stack.clear();
			
			float m[9];
			vgGetMatrix( m );
			// assume the current openvg matrix is like the camera matrix and should always be applied first
			Transform2d top;
			Transform2d::multiply( top, Transform2d(m), rootTransform() );	// multiply by the root transform
			pushTransform( top );
			
			// SVG is origin at the top, left (openvg is origin at the bottom, left)
			// so need to flip
			//		Transform2d flip;
			//		flip.setScale( 1, -1 );
			//		pushTransform( flip );
			
			draw_recursive( _root_group );
			
			vgLoadMatrix( m );	// restore matrix
			_transform_stack.clear();

			
		} vgEndBatchMNK( _batch );
		
	}
	
	void OpenVG_SVGHandler::onGroupBegin() {
		_mode = kGroupParseMode;
		_current_group->children.push_back( group_t() );
		group_t* top = &_current_group->children.back();
		top->parent = _current_group;
		_current_group = top;
		// copy any use transform
		_current_group->transform = _use_transform;
	}
	void OpenVG_SVGHandler::onGroupEnd() {
		_current_group = _current_group->parent;
	}

	
	void OpenVG_SVGHandler::onPathBegin() { 
		_mode = kPathParseMode;
		_current_group->current_path = new path_object_t();
		_current_group->current_path->path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
							 1,0,0,0, VG_PATH_CAPABILITY_ALL);
		// inherit group settings
		_current_group->current_path->fill			= _current_group->fill;
		_current_group->current_path->stroke		= _current_group->stroke;
		_current_group->current_path->stroke_width	= _current_group->stroke_width;
		_current_group->current_path->fill_rule		= _current_group->fill_rule;
		
	}
	
	void OpenVG_SVGHandler::onPathEnd() {  
		
		// onPathClose()
		
		_current_group->path_objects.push_back( *_current_group->current_path );
		
//		// build up the bounds
//		VGfloat minX, minY, width, height;
//		vgPathBounds( _current_group->current_path->path, &minX, &minY, &width, &height );
//		if ( minX < _minX ) {
//			_minX = minX;
//		}
//		if ( minY < _minY ) {
//			_minY = minY;
//		}
//		if ( width > _width ) {
//			_width = width;
//		}
//		if ( height > _height ) {
//			_height = height;
//		}
		
	}
	
	void OpenVG_SVGHandler::onPathMoveTo( float x, float y ) { 
		VGubyte seg = VG_MOVE_TO | openVGRelative();
		VGfloat data[2];
		
		data[0] = x; data[1] = y;
		vgAppendPathData( _current_group->current_path->path, 1, &seg, data );
		
	}
	
	void OpenVG_SVGHandler::onPathClose(){
		VGubyte seg = VG_CLOSE_PATH;
		VGfloat data = 0.0f;
		vgAppendPathData( _current_group->current_path->path, 1, &seg, &data );

	}
	void OpenVG_SVGHandler::onPathLineTo( float x, float y ) { 
		VGubyte seg = VG_LINE_TO | openVGRelative();
		VGfloat data[2];
		
		data[0] = x; data[1] = y;
		vgAppendPathData( _current_group->current_path->path, 1, &seg, data );
		
	}
	
	void OpenVG_SVGHandler::onPathHorizontalLine( float x ) {
		VGubyte seg = VG_HLINE_TO | openVGRelative();
		VGfloat data[1];
		data[0] = x; 
		vgAppendPathData( _current_group->current_path->path, 1, &seg, data );
		
	}
	void OpenVG_SVGHandler::onPathVerticalLine( float y ) {
		VGubyte seg = VG_VLINE_TO | openVGRelative();
		VGfloat data[1];
		data[0] = y; 
		vgAppendPathData( _current_group->current_path->path, 1, &seg, data );
		
	}

	void OpenVG_SVGHandler::onPathCubic( float x1, float y1, float x2, float y2, float x3, float y3 ) { 
		VGubyte seg = VG_CUBIC_TO | openVGRelative();
		VGfloat data[6];
		
		data[0] = x1; data[1] = y1;
		data[2] = x2; data[3] = y2;
		data[4] = x3; data[5] = y3;
		vgAppendPathData( _current_group->current_path->path, 1, &seg, data);
		
	}
	
	void OpenVG_SVGHandler::onPathSCubic( float x2, float y2, float x3, float y3 ) {
		VGubyte seg = VG_SCUBIC_TO | openVGRelative();
		VGfloat data[4];
		
		data[0] = x2; data[1] = y2;
		data[2] = x3; data[3] = y3;
		vgAppendPathData( _current_group->current_path->path, 1, &seg, data);
		
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
		
		vgAppendPathData( _current_group->current_path->path, 1, &seg, data);
		
	}
	
	void OpenVG_SVGHandler::onPathRect( float x, float y, float w, float h ) {
		vguRect( _current_group->current_path->path, x, y, w, h );
	}

	
	void OpenVG_SVGHandler::onPathFillColor( unsigned int color ) {
		if( _mode == kGroupParseMode ) {
			_current_group->fill = vgCreatePaint();
			VGfloat fcolor[4] = { VGfloat( (color & 0xff000000) >> 24)/255.0f, 
				VGfloat( (color & 0x00ff0000) >> 16)/255.0f, 
				VGfloat( (color & 0x0000ff00) >> 8)/255.0f, 
				1.0f };
			vgSetParameterfv( _current_group->fill, VG_PAINT_COLOR, 4, &fcolor[0]);

		} else {
			_current_group->current_path->fill = vgCreatePaint();
			VGfloat fcolor[4] = { VGfloat( (color & 0xff000000) >> 24)/255.0f, 
				VGfloat( (color & 0x00ff0000) >> 16)/255.0f, 
				VGfloat( (color & 0x0000ff00) >> 8)/255.0f, 
				1.0f  };
			vgSetParameterfv( _current_group->current_path->fill, VG_PAINT_COLOR, 4, &fcolor[0]);
		}
	}
	
	void OpenVG_SVGHandler::onPathFillOpacity( float o ) {
		VGfloat fcolor[4];
		if( _mode == kGroupParseMode ) {
			if( _current_group->fill == 0 ) {	// if no fill create a black fill
				_current_group->fill = vgCreatePaint();
				VGfloat fcolor[4] = { 0,0,0,1 };
				vgSetParameterfv( _current_group->fill, VG_PAINT_COLOR, 4, &fcolor[0]);
			}
			vgGetParameterfv( _current_group->fill, VG_PAINT_COLOR, 4, &fcolor[0] );
			// set the opacity
			fcolor[3] = o;
			vgSetParameterfv( _current_group->fill, VG_PAINT_COLOR, 4, &fcolor[0]);
			
		} else {
			if( _current_group->current_path->fill == 0 ) {	// if no fill create a black fill
				_current_group->current_path->fill = vgCreatePaint();
				VGfloat fcolor[4] = { 0,0,0,1 };
				vgSetParameterfv( _current_group->current_path->fill, VG_PAINT_COLOR, 4, &fcolor[0]);
			}

			vgGetParameterfv( _current_group->current_path->fill, VG_PAINT_COLOR, 4, &fcolor[0] );
			// set the opacity
			fcolor[3] = o;
			vgSetParameterfv( _current_group->current_path->fill, VG_PAINT_COLOR, 4, &fcolor[0]);
		}
	}
	void OpenVG_SVGHandler::onPathStrokeColor( unsigned int color ) {
		if( _mode == kGroupParseMode ) {
			_current_group->stroke = vgCreatePaint();
			VGfloat fcolor[4] = { VGfloat( (color & 0xff000000) >> 24)/255.0f, 
				VGfloat( (color & 0x00ff0000) >> 16)/255.0f, 
				VGfloat( (color & 0x0000ff00) >> 8)/255.0f, 
				1.0f };
			vgSetParameterfv( _current_group->stroke, VG_PAINT_COLOR, 4, &fcolor[0]);
		} else {
			_current_group->current_path->stroke = vgCreatePaint();
			VGfloat fcolor[4] = { VGfloat( (color & 0xff000000) >> 24)/255.0f, 
				VGfloat( (color & 0x00ff0000) >> 16)/255.0f, 
				VGfloat( (color & 0x0000ff00) >> 8)/255.0f, 
				1.0f };
			vgSetParameterfv( _current_group->current_path->stroke, VG_PAINT_COLOR, 4, &fcolor[0]);
		}
	}
	void OpenVG_SVGHandler::onPathStrokeOpacity( float o ) {
		VGfloat fcolor[4];
		if( _mode == kGroupParseMode ) {
			vgGetParameterfv( _current_group->stroke, VG_PAINT_COLOR, 4, &fcolor[0] );
			// set the opacity
			fcolor[3] = o;
			vgSetParameterfv( _current_group->stroke, VG_PAINT_COLOR, 4, &fcolor[0]);
			
		} else {
			vgGetParameterfv( _current_group->current_path->stroke, VG_PAINT_COLOR, 4, &fcolor[0] );
			// set the opacity
			fcolor[3] = o;
			vgSetParameterfv( _current_group->current_path->stroke, VG_PAINT_COLOR, 4, &fcolor[0]);
		}
		
	}

	void OpenVG_SVGHandler::onPathStrokeWidth( float width ) {
		if( _mode == kGroupParseMode ) {
			_current_group->stroke_width = width;
		} else {
			_current_group->current_path->stroke_width = width;
		}
	}
	
	void OpenVG_SVGHandler::onPathFillRule( const string& rule ) {
		if( _mode == kGroupParseMode ) {
			if( rule == "nonzero" ) {
				_current_group->fill_rule = VG_NON_ZERO;
			} else if( rule == "evenodd" ) {
				_current_group->fill_rule = VG_EVEN_ODD;
			}

		} else {
			if( rule == "nonzero" ) {
				_current_group->current_path->fill_rule = VG_NON_ZERO;
			} else if( rule == "evenodd" ) {
				_current_group->current_path->fill_rule = VG_EVEN_ODD;
			}
		}
	}
	
	void OpenVG_SVGHandler::onTransformTranslate( float x, float y ) {
		if( _mode == kGroupParseMode ) {
			_current_group->transform.setTranslate( x, y );
		} else if( kPathParseMode ) {	// 
			_current_group->current_path->transform.setTranslate( x, y );
		} else if( kUseParseMode ) {
			_use_transform.setTranslate( x, y );
		}
	}
	void OpenVG_SVGHandler::onTransformScale( float s ) {
		if( _mode == kGroupParseMode ) {
			_current_group->transform.setScale( s, s );
		} else if( _mode == kPathParseMode ) { // kPathParseMode
			_current_group->current_path->transform.setScale( s, s );
		} else if( _mode == kUseParseMode ) {
			_use_transform.setScale( s, s );
		}
	}
	void OpenVG_SVGHandler::onTransformRotate( float r ) {
		if( _mode == kGroupParseMode ) {
			_current_group->transform.setRotation( r );	// ?? radians or degrees ??
		} else if( _mode == kPathParseMode ) { // kPathParseMode
			_current_group->current_path->transform.setRotation( r );	// ?? radians or degrees ??
		} else if( _mode == kUseParseMode ) {
			_use_transform.setRotation( r );
		}
	}
	void OpenVG_SVGHandler::onTransformMatrix( float a, float b, float c, float d, float e, float f ) {
		Transform2d t;
		t.a = a; t.b = b; t.c = c; t.d = d; t.e = e; t.f = f;
		if( _mode == kGroupParseMode ) {
			_current_group->transform = t;
		} else if( _mode == kPathParseMode ) { // kPathParseMode
			_current_group->current_path->transform = t;
		} else if( _mode == kUseParseMode ) {
			_use_transform = t;//topTransform();
		}
	}
	
	void OpenVG_SVGHandler::onId( const std::string& id_ ) {
		if( _mode == kGroupParseMode ) {
			_current_group->id  = id_;
		} else { // kPathParseMode
			_current_group->current_path->id = id_;
		}
	}
	
	void OpenVG_SVGHandler::onUseBegin() {
		_mode = kUseParseMode;
	}
	void OpenVG_SVGHandler::onUseEnd() {
		_use_transform.setIdentity();
	}

	
}
