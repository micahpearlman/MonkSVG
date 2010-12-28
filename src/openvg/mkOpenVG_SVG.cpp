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
	void OpenVG_SVGHandler::draw() {
		vector<VGPaint>::iterator fillpaintiter = _fill_list.begin();
		vector<VGPaint>::iterator strokepaintiter = _stroke_list.begin();
		vector<float>::iterator widthiter = _stroke_width.begin();
		int i = 0;
		for ( vector<VGPath>::iterator iter = _path_list.begin(); iter != _path_list.end(); iter++, fillpaintiter++, strokepaintiter++, widthiter++, i++ ) {
			vgSetPaint( *fillpaintiter, VG_FILL_PATH );
			vgSetPaint( *strokepaintiter, VG_STROKE_PATH );
			vgSetf( VG_STROKE_LINE_WIDTH, *widthiter );
			//			if ( i == 2) {
			vgDrawPath( *iter, VG_FILL_PATH | VG_STROKE_PATH );
			//			}
			
			
		}
		
	}
	
	void OpenVG_SVGHandler::onPathBegin() { 
		_path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
							 1,0,0,0, VG_PATH_CAPABILITY_ALL);
		
	}
	
	void OpenVG_SVGHandler::onPathEnd() {  
		VGubyte seg = VG_CLOSE_PATH;
		VGfloat data = 0.0f;
		vgAppendPathData( _path, 1, &seg, &data );
		_path_list.push_back( _path );
		
	}
	
	void OpenVG_SVGHandler::onPathMoveTo( float x, float y ) { 
		VGubyte seg = VG_MOVE_TO | VG_ABSOLUTE;
		VGfloat data[2];
		
		data[0] = x; data[1] = y;
		vgAppendPathData( _path, 1, &seg, data );
		
	}
	void OpenVG_SVGHandler::onPathLineTo( float x, float y ) { 
		VGubyte seg = VG_LINE_TO | VG_ABSOLUTE;
		VGfloat data[2];
		
		data[0] = x; data[1] = y;
		vgAppendPathData( _path, 1, &seg, data );
		
	}
	void OpenVG_SVGHandler::onPathCubic( float x1, float y1, float x2, float y2, float x3, float y3 ) { 
		VGubyte seg = VG_CUBIC_TO | VG_ABSOLUTE;
		VGfloat data[6];
		
		data[0] = x1; data[1] = y1;
		data[2] = x2; data[3] = y2;
		data[4] = x3; data[5] = y3;
		vgAppendPathData( _path, 1, &seg, data);
		
	}
	void OpenVG_SVGHandler::onPathFillColor( unsigned int color ) {
		_fill_paint = vgCreatePaint();
		VGfloat fcolor[4] = { VGfloat( (color & 0xff000000) >> 24)/255.0f, 
			VGfloat( (color & 0x00ff0000) >> 16)/255.0f, 
			VGfloat( (color & 0x0000ff00) >> 8)/255.0f, 
			1.0f /*VGfloat(color & 0x000000ff)/255.0f*/ };
		vgSetParameterfv(_fill_paint, VG_PAINT_COLOR, 4, &fcolor[0]);
		_fill_list.push_back( _fill_paint );
	}
	void OpenVG_SVGHandler::onPathStrokeColor( unsigned int color ) {
		VGPaint stroke_paint = vgCreatePaint();
		VGfloat fcolor[4] = { VGfloat( (color & 0xff000000) >> 24)/255.0f, 
			VGfloat( (color & 0x00ff0000) >> 16)/255.0f, 
			VGfloat( (color & 0x0000ff00) >> 8)/255.0f, 
			1.0f /*VGfloat(color & 0x000000ff)/255.0f*/ };
		vgSetParameterfv(stroke_paint, VG_PAINT_COLOR, 4, &fcolor[0]);
		_stroke_list.push_back( stroke_paint );
		
	}
	void OpenVG_SVGHandler::onPathStrokeWidth( float width ) {
		_stroke_width.push_back( width );
	}
}
