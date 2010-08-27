/*
 *  mkSVG.h
 *  MonkSVG
 *
 *  Created by Micah Pearlman on 8/2/10.
 *  Copyright 2010 Zero Vision. All rights reserved.
 *
 */

#ifndef __mkSVG_h__
#define __mkSVG_h__


#include <string>
#include <vector>

class TiXmlDocument;
class TiXmlElement;

namespace MonkSVG {
	using namespace std;
	
	class ISVGHandler {
	public:
		
		// transforms 
		virtual void onTransformTranslate( float x, float y ) {}
		virtual void onTransformScale( float s ) {}
		virtual void onTransformRotate( float r ) {}
		virtual void onTransformMatrix( float a, float b, float c, float d, float e, float f ) {}
		
		// paths 
		virtual void onPathBegin() {}
		virtual void onPathEnd() {}
		virtual void onPathMoveTo( float x, float y ) {}
		virtual void onPathLineTo( float x, float y ) {}
		virtual void onPathCubic( float x1, float y1, float x2, float y2, float x3, float y3 ) {}
		// fill
		virtual void onPathFillColor( unsigned int color ) {}
		virtual void onPathFillRule( string rule ) {}
		// stroke
		virtual void onPathStrokeColor( unsigned int color ) {}
		virtual void onPathStrokeWidth( float width ) {}
		
	};
	
	class SVG  {
	public:
		
		bool initialize( ISVGHandler* handler );
		bool read( string& data );

	private:
		
		ISVGHandler*	_handler;
		
	private:
		void recursive_parse( TiXmlDocument* doc, TiXmlElement* element );
		void parse_path( string& ps );

	};
}

#endif // __mkSVG_h__