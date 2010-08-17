/*
 *  mkSVG.cpp
 *  MonkSVG
 *
 *  Created by Micah Pearlman on 8/2/10.
 *  Copyright 2010 Zero Vision. All rights reserved.
 *
 */

#include "mkSVG.h"
#include "tinyxml.h"

namespace MonkSVG {
	
	
	bool SVG::initialize( ISVGHandler* handler ) {
		_handler = handler;
		
		return true;
	}
	
	bool SVG::read( string& data ) {
		TiXmlDocument doc;
		
		
		doc.Parse( data.c_str() );
		TiXmlElement* root = doc.FirstChild( "svg" )->ToElement();
		recursive_parse( &doc, root );
		
		return true;
	}
	
	void SVG::recursive_parse( TiXmlDocument* doc, TiXmlElement* element ) {
		if ( element ) {
			TiXmlElement* child = element->FirstChildElement();
			recursive_parse( doc, child );
		}
		
		if ( element ) {
			for ( TiXmlElement* sibbling = element; sibbling != 0; sibbling = sibbling->NextSiblingElement() ) {
				string type = sibbling->Value();
				
				if ( type == "path" ) {
					_handler->onPathBegin();
					string d = sibbling->Attribute( "d" );
					parse_path( d );
					_handler->onPathEnd();
					
					string fill = sibbling->Attribute( "fill" );
					unsigned int color = strtol( fill.c_str() + 1, 0, 16 );
					if ( fill.length() == 7 ) {
						color = color << 8;
						color |= 0x000000ff;
					}
					_handler->onPathFillColor( color );
				}
			}
		}
	}
	
	void SVG::parse_path( string& d ) {
		char* c = const_cast<char*>( d.c_str() );
		while ( *c ) {
			switch ( *c ) {
				case 'm':
				case 'M':
				{
					c++;
					float x = strtof( c, &c );
					float y = strtof( c, &c );
					_handler->onPathMoveTo( x, y );
				}
				break;
					
				case 'l':
				case 'L':
				{
					c++;
					float x = strtof( c, &c );
					float y = strtof( c, &c );
					_handler->onPathLineTo( x, y );
					
				}
				break;
	
				case 'c':
				case 'C':
				{
					c++;
					float x1 = strtof( c, &c );
					float y1 = strtof( c, &c );
					float x2 = strtof( c, &c );
					float y2 = strtof( c, &c );
					float x3 = strtof( c, &c );
					float y3 = strtof( c, &c );
					_handler->onPathCubic(x1, y1, x2, y2, x3, y3);
					
				}
				break;
				
				default:
					c++;
					break;
			}
		}
	}

};