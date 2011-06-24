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
#include <map>
#include <cmath>
#include <boost/shared_ptr.hpp>


class TiXmlDocument;
class TiXmlElement;

namespace MonkSVG {
	using namespace std;
	
	class ISVGHandler {
	public:
		
		typedef boost::shared_ptr<ISVGHandler> SmartPtr;
		
		// transforms 
		virtual void onTransformTranslate( float x, float y ) {}
		virtual void onTransformScale( float s ) {}
		virtual void onTransformRotate( float r ) {}
		virtual void onTransformMatrix( float a, float b, float c, float d, float e, float f ) {}
		
		// groups
		virtual void onGroupBegin() {}
		virtual void onGroupEnd() {}
		virtual void onUseBegin() {}
		virtual void onUseEnd() {}
		
		virtual void onId( const std::string& id_ ) {}
		
		// paths 
		virtual void onPathBegin() {}
		virtual void onPathEnd() {}
		virtual void onPathMoveTo( float x, float y ) {}
		virtual void onPathClose(){}
		virtual void onPathLineTo( float x, float y ) {}
		virtual void onPathCubic( float x1, float y1, float x2, float y2, float x3, float y3 ) {}
		virtual void onPathSCubic( float x2, float y2, float x3, float y3 ) {}
		virtual void onPathArc( float rx, float ry, float x_axis_rotation, int large_arc_flag, int sweep_flag, float x, float y ) {}
		virtual void onPathRect( float x, float y, float w, float h ) {}
		virtual void onPathHorizontalLine( float x ) {}
		virtual void onPathVerticalLine( float y ) {}

		// fill
		virtual void onPathFillColor( unsigned int color ) {}
		virtual void onPathFillOpacity( float o ) {}
		virtual void onPathFillRule( const string& rule ) {}
		// stroke
		virtual void onPathStrokeColor( unsigned int color ) {}
		virtual void onPathStrokeOpacity( float o ) {}
		virtual void onPathStrokeWidth( float width ) {}
		
		void setRelative( bool r ) {
			_relative = r;
		}
		bool relative() const {
			return _relative;
		}
		
		// bounds
		float minX() {
			return _minX;
		}
		float minY() {
			return _minY;
		}
		float width() {
			return _width;
		}
		float height() {
			return _height;
		}
		
	protected:
		
		ISVGHandler() 
		:	_minX( MAXFLOAT )
		,	_minY( MAXFLOAT )
		,	_width( -MAXFLOAT )
		,	_height( -MAXFLOAT ) 
		{
			
		}
		// bounds info
		float _minX;
		float _minY;
		float _width;
		float _height;
		
	private:
		bool _relative;
		
	};
	
	class SVG  {
	public:
		
		bool initialize( ISVGHandler::SmartPtr handler );
		bool read( string& data );
		bool read( const char* data );

	private:
		
		ISVGHandler::SmartPtr		_handler;
		
		// holds svg <symbols>
		map<string, TiXmlElement*>	_symbols;
		
	private:
		void recursive_parse( TiXmlElement* element );
		bool handle_xml_element( TiXmlElement* element );
		void handle_group( TiXmlElement* pathElement ); 
		void handle_path( TiXmlElement* pathElement );
		void handle_rect( TiXmlElement* pathElement );
		void handle_general_parameter( TiXmlElement* pathElement );
		void parse_path_d( string& ps );
		void parse_path_style( string& ps );
		void parse_path_transform( string& tr );
		uint32_t string_hex_color_to_uint( string& hexstring );
		float d_string_to_float( char *c, char **str );
		int d_string_to_int( char *c, char **str );
		void nextState( char** c, char* state );

	};
}

#endif // __mkSVG_h__