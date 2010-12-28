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
#include <map>
#include <iterator>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
using namespace boost;

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
				
				if ( type == "g" ) {
					handle_path( sibbling->FirstChildElement() );
				}
				
				if ( type == "path" ) {
					handle_path( sibbling );
				}
			}
		}
	}
	
	
	float SVG::d_string_to_float( char *c, char** str ) {
		while ( isspace(*c) ) {
			c++;
			(*str)++;
		}
		while ( *c == ',' ) {
			c++;
			(*str)++;
		}
		
		return strtof( c, str );
	}
	
	uint32_t SVG::string_hex_color_to_uint( string& hexstring ) {
		uint32_t color = strtol( hexstring.c_str() + 1, 0, 16 );
		if ( hexstring.length() == 7 ) {	// fix up to rgba if the color is only rgb
			color = color << 8;
			color |= 0x000000ff;
		}
		
		return color;
	}	
	
	void SVG::handle_path( TiXmlElement* pathElement ) {
		
		_handler->onPathBegin();
		string d = pathElement->Attribute( "d" );
		parse_path_d( d );
		_handler->onPathEnd();
		
		string fill; 
		if ( pathElement->QueryStringAttribute( "fill", &fill ) == TIXML_SUCCESS ) {
			_handler->onPathFillColor( string_hex_color_to_uint( fill ) );
		}
		
		
		string stroke;
		if ( pathElement->QueryStringAttribute( "stroke", &stroke) == TIXML_SUCCESS ) {
			_handler->onPathStrokeColor( string_hex_color_to_uint( stroke ) );
		}
		
		string stroke_width;
		if ( pathElement->QueryStringAttribute( "stroke-width", &stroke_width) == TIXML_SUCCESS ) {
			float width = atof( stroke_width.c_str() );
			_handler->onPathStrokeWidth( width );
		}
		
		string style;
		if ( pathElement->QueryStringAttribute( "style", &style) == TIXML_SUCCESS ) {
			parse_path_style( style );
		}
		
	}
	
	void nextState( char** c, char* state ) {
		
		while ( isspace(**c) ) {
			(*c)++;
		}
		if ( isalpha( **c ) ) {
			*state = **c;
			(*c)++;
		}
	}
	
	void SVG::parse_path_d( string& d ) {
		char* c = const_cast<char*>( d.c_str() );
		char state = *c;
		nextState( &c, &state );
		while ( *c && state != 'z' ) {
			
			switch ( state ) {
				case 'm':
				case 'M':
				{
					//c++;
					float x = d_string_to_float( c, &c );
					float y = d_string_to_float( c, &c );
					_handler->onPathMoveTo( x, y );
					nextState(&c, &state);
				}
				break;
					
				case 'l':
				case 'L':
				{
					//c++;
					float x = d_string_to_float( c, &c );
					float y = d_string_to_float( c, &c );
					_handler->onPathLineTo( x, y );
					nextState(&c, &state);
					
				}
				break;
	
				case 'c':
				case 'C':
				{
					//c++;
					float x1 = d_string_to_float( c, &c );
					float y1 = d_string_to_float( c, &c );
					float x2 = d_string_to_float( c, &c );
					float y2 = d_string_to_float( c, &c );
					float x3 = d_string_to_float( c, &c );
					float y3 = d_string_to_float( c, &c );
					_handler->onPathCubic(x1, y1, x2, y2, x3, y3);
					nextState(&c, &state);
					
				}
				break;
				case 'z':
				case 'Z':
				{
					//c++;
					_handler->onPathEnd();
					nextState(&c, &state);
					
				}
				break;
					
				
				default:
					c++;
					break;
			}
		}
	}
	
	// semicolon-separated property declarations of the form "name : value" within the ‘style’ attribute
	void SVG::parse_path_style( string& ps ) {
		map< string, string > style_key_values;
		char_separator<char> values_seperator(";");
		char_separator<char> key_value_seperator(":");
		tokenizer<char_separator<char> > values_tokens( ps, values_seperator );
		BOOST_FOREACH( string values, values_tokens ) {
			tokenizer<char_separator<char> > key_value_tokens( values, key_value_seperator );
			tokenizer<char_separator<char> >::iterator k = key_value_tokens.begin();
			tokenizer<char_separator<char> >::iterator v = k;
			v++;
			//cout << *k << ":" << *v << endl;
			style_key_values[*k] = *v;
		}
		
		map<string, string>::iterator kv = style_key_values.find( string("fill") );
		if ( kv != style_key_values.end() ) {
			_handler->onPathFillColor( string_hex_color_to_uint( kv->second ) );
		}
		
		kv = style_key_values.find( "stroke" );
		if ( kv != style_key_values.end() ) {
			_handler->onPathStrokeColor( string_hex_color_to_uint( kv->second ) );
		}
		
		kv = style_key_values.find( "stroke-width" );
		if ( kv != style_key_values.end() ) {
			float width = atof( kv->second.c_str() );
			_handler->onPathStrokeWidth( width );
		}
		
		
	}

};