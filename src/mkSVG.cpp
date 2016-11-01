/*
 *  mkSVG.cpp
 *  MonkSVG
 *
 *  Created by Micah Pearlman on 8/2/10.
 *  Copyright 2010 Zero Vision. All rights reserved.
 *
 */

#include "mkSVG.h"
#include <tinyxml2/tinyxml2.h>
#include <StyleSheet/Document.h>
#include <map>
#include <iterator>
#include <regex>
#include <cstring>

#include "mkPath.h"
#include "mkBatch.h"
#include "mkMath.h"
#include "mkPaint.h"
#include <VG/vgext.h>
#include <OpenGLES/ES2/gl.h>
#include <glm/gtx/matrix_transform_2d.hpp>

namespace MonkSVG {
    MKSVGHandler::path_object_t::~path_object_t() {
        if (fill) delete fill;
        if (stroke) delete stroke;
        if (path) delete path;
    }

    StyleSheet::CssDocument _styleDocument;
	
	bool SVG::initialize( MKSVGHandler* handler ) {
		_handler = handler;
		
		return true;
	}
	
	bool SVG::read( const char* data ) {
        
		XMLDocument doc;
		doc.Parse( data );
        
        if (doc.Error()) {
            return false;
        }
		
		XMLElement* root = doc.FirstChildElement( "svg" )->ToElement();
		recursive_parse( root );        
        
        // get bounds information from the svg file, ignoring non-pixel values
        
        const char* inCString;
        string numberWithUnitString;
        regex numberWithUnitPattern( "^(-?\\d+)(px)?$" );
        
        _handler->_minX = 0.0f;
        if ( root->QueryStringAttribute( "x", &inCString ) == XML_SUCCESS ) {
            numberWithUnitString = inCString;
            match_results<string::const_iterator> matches;
            if ( regex_search( numberWithUnitString, matches, numberWithUnitPattern ) ) {
                _handler->_minX = ::atof( matches[1].str().c_str() );
            }
        }
        
        _handler->_minY = 0.0f;
        if ( root->QueryStringAttribute( "y", &inCString ) == XML_SUCCESS ) {
            numberWithUnitString = inCString;
            match_results<string::const_iterator> matches;
            if ( regex_search( numberWithUnitString, matches, numberWithUnitPattern ) ) {
                _handler->_minY = ::atof( matches[1].str().c_str() );
            }
        }
        
        _handler->_width = 0.0f;
        if ( root->QueryStringAttribute( "width", &inCString ) == XML_SUCCESS ) {
            numberWithUnitString = inCString;
            match_results<string::const_iterator> matches;
            if ( regex_search( numberWithUnitString, matches, numberWithUnitPattern ) ) {
                _handler->_width = ::atof( matches[1].str().c_str() );
            }
        }
        
        _handler->_height = 0.0f;
        if ( root->QueryStringAttribute( "height", &inCString ) == XML_SUCCESS ) {
            numberWithUnitString = inCString;
            match_results<string::const_iterator> matches;
            if ( regex_search( numberWithUnitString, matches, numberWithUnitPattern ) ) {
                _handler->_height = ::atof( matches[1].str().c_str() );
            }
        }
        
        
        if (_handler->_width == 0.0f && _handler->_height == 0.0f) {
            if ( root->QueryStringAttribute( "viewBox", &inCString) == XML_SUCCESS ) {
                numberWithUnitString = inCString;
                match_results<string::const_iterator> matches;
                regex numberWithUnitPatternVBox( "(\\d+)[ ](\\d+)[ ](\\d+)[ ](\\d+)" );
                if ( regex_search( numberWithUnitString, matches, numberWithUnitPatternVBox ) ) {
                    _handler->_minX = ::atof( matches[1].str().c_str() );
                    _handler->_minY = ::atof( matches[2].str().c_str() );
                    _handler->_width = ::atof( matches[3].str().c_str() );
                    _handler->_height  = ::atof( matches[4].str().c_str() );
                }
            }
            
        }
        
		return true;
		
	}
	
	bool SVG::read( string& data ) {
		return read( data.c_str() );
	}
	
	
	
	void SVG::recursive_parse( XMLElement* element ) {
		
		if ( element ) {
			for ( XMLElement* sibbling = element; sibbling != 0; sibbling = sibbling->NextSiblingElement() ) {
				handle_xml_element( sibbling );
			}
		}
		
		if ( element ) {
			XMLElement* child = element->FirstChildElement();
			// if we don't handle the element recursively go into it
			if( handle_xml_element( child ) == false ) {
				recursive_parse( child );
			}
		}

	}
	
	bool SVG::handle_xml_element( XMLElement* element ) {
		if( !element )
			return false;
		
		string type = element->Value();

        if (type == "style") {
            handle_stylesheet(element);
            return false;
        } else if ( type == "g" ) {
			handle_group( element );
			return true;
		} else if ( type == "path" ) {
			handle_path( element );
			return true;
		} else if ( type == "rect" ) {
			handle_rect( element );
			return true;
        } else if ( type == "line" ) {
            handle_line( element );
            return true;
        } else if ( type == "polyline" ) {
            handle_polyline( element );
            return true;
        } else if ( type == "polygon" ) {
			handle_polygon ( element );
			return true;
		} else if( type == "symbol" ) {
			const char* id;
			if ( element->QueryStringAttribute( "id", &id ) == XML_SUCCESS ) {
				_symbols[id] = (XMLElement*)element->ShallowClone(nullptr);
			}
			return true;
		} else if ( type == "use" ) {
			const char* href;
			if ( element->QueryStringAttribute( "xlink:href", &href ) == XML_SUCCESS ) {
				string id = href+1;	// skip the #
				_handler->onUseBegin();
				// handle transform and other parameters
				handle_general_parameter( element );
				recursive_parse( _symbols[id] );
				_handler->onUseEnd();
			}
			
			return true;
        } 
		return false;

	}
    
	void SVG::handle_stylesheet( XMLElement* pathElement ) {
        if (pathElement->GetText()) {
            _styleDocument = StyleSheet::CssDocument::parse(pathElement->GetText());
        }
    }
    
	void SVG::handle_group( XMLElement* pathElement ) {
		string id_;
//		if ( pathElement->QueryStringAttribute( "id", &id_) == TIXML_SUCCESS ) {
//			//_handler->onId( id_ );
//			cout << "group begin: " << id_ << endl;
//		}

		_handler->onGroupBegin();
		
		// handle transform and other parameters
		handle_general_parameter( pathElement );
		
		// go through all the children
		XMLElement* children = pathElement->FirstChildElement();
		for ( XMLElement* child = children; child != 0; child = child->NextSiblingElement() ) {
			string type = child->Value();
			handle_xml_element( child );
		}
		
		_handler->onGroupEnd();
		
//		if ( pathElement->QueryStringAttribute( "id", &id_) == XML_SUCCESS ) {
//			//_handler->onId( id_ );
//			cout << "group end: " << id_ << endl;
//		}


	}
	
	void SVG::handle_path( XMLElement* pathElement ) {
		
//		string id_;
//		if ( pathElement->QueryStringAttribute( "id", &id_) == XML_SUCCESS ) {
//			//_handler->onId( id_ );
//			cout << "path: " << id_ << endl;
//		}

		_handler->onPathBegin();
		const char* d;
		if ( pathElement->QueryStringAttribute( "d", &d ) == XML_SUCCESS ) {
			parse_path_d( d );
		}
		
		handle_general_parameter( pathElement );
		
		_handler->onPathEnd();		
	}
	
    void SVG::handle_line( XMLElement* pathElement ) {
        _handler->onPathBegin();
        
        double pos [4];
        if ( pathElement->QueryDoubleAttribute( "x1", &pos[0]) == XML_SUCCESS) {
        }
        if ( pathElement->QueryDoubleAttribute( "y1", &pos[1]) == XML_SUCCESS) {
        }
        if ( pathElement->QueryDoubleAttribute( "x2", &pos[2]) == XML_SUCCESS) {
        }
        if ( pathElement->QueryDoubleAttribute( "y2", &pos[3]) == XML_SUCCESS) {
        }
        _handler->setRelative(false);
        _handler->onPathMoveTo(pos[0], pos[1]);
        _handler->onPathLineTo(pos[2], pos[3]);
        
        handle_general_parameter( pathElement );

        _handler->onPathEnd();
    }
    
    void SVG::handle_polyline( XMLElement* pathElement ) {
        _handler->onPathBegin();
        const char* points;
        if ( pathElement->QueryStringAttribute( "points", &points ) == XML_SUCCESS ) {
            parse_polyline_points( points );
        }
        
        handle_general_parameter( pathElement );
        
        _handler->onPathEnd();

    }
    
	void SVG::handle_rect( XMLElement* pathElement ) {
		_handler->onPathBegin();
		
		
		float pos[2] = { 0, 0 };
		if ( pathElement->QueryFloatAttribute( "x", &pos[0] ) == XML_SUCCESS ) {
			//parse_path_d( d );
		}
		if ( pathElement->QueryFloatAttribute( "y", &pos[1] ) == XML_SUCCESS ) {
			//parse_path_d( d );
		}
		float sz[2] = { 0, 0 };
		if ( pathElement->QueryFloatAttribute( "width", &sz[0] ) == XML_SUCCESS ) {
			//parse_path_d( d );
		}
		if ( pathElement->QueryFloatAttribute( "height", &sz[1] ) == XML_SUCCESS ) {
			//parse_path_d( d );
		}
		_handler->onPathRect( pos[0], pos[1], sz[0], sz[1] );
		

		handle_general_parameter( pathElement );
		
		
		_handler->onPathEnd();		
		
	}

	void SVG::handle_polygon( XMLElement* pathElement ) {
		_handler->onPathBegin();
		const char* points;
		if ( pathElement->QueryStringAttribute( "points", &points ) == XML_SUCCESS ) {
			parse_points( points );
		}

		handle_general_parameter( pathElement );

		_handler->onPathEnd();
	}
	
	void SVG::handle_general_parameter( XMLElement* pathElement ) {
		const char* fill;
		if ( pathElement->QueryStringAttribute( "fill", &fill ) == XML_SUCCESS ) {
			_handler->onPathFillColor( string_hex_color_to_uint( fill ) );
		}
		
		
		const char* stroke;
		if ( pathElement->QueryStringAttribute( "stroke", &stroke) == XML_SUCCESS ) {
			_handler->onPathStrokeColor( string_hex_color_to_uint( stroke ) );
		}
		
		const char* stroke_width;
		if ( pathElement->QueryStringAttribute( "stroke-width", &stroke_width) == XML_SUCCESS ) {
			float width = atof( stroke_width );
			_handler->onPathStrokeWidth( width );
		}
		
		const char* style;
		if ( pathElement->QueryStringAttribute( "style", &style) == XML_SUCCESS ) {
			parse_path_style( style );
		}
		
        const char* class_;
        if ( pathElement->QueryStringAttribute( "class", &class_) == XML_SUCCESS) {
            parse_path_stylesheet(class_);
        }
        
		const char* transform;
		if ( pathElement->QueryStringAttribute( "transform", &transform) == XML_SUCCESS ) {
			parse_path_transform( transform );
		}
		const char* id_;
		if ( pathElement->QueryStringAttribute( "id", &id_) == XML_SUCCESS ) {
			_handler->onId( id_ );
		}
		
		const char* opacity;
		if ( pathElement->QueryStringAttribute( "opacity", &opacity) == XML_SUCCESS ) {
			float o = atof( opacity );
			_handler->onPathFillOpacity( o );
			// TODO: ??? stroke opacity???
		}
		if ( pathElement->QueryStringAttribute( "fill-opacity", &opacity) == XML_SUCCESS ) {
			float o = atof( opacity );
			_handler->onPathFillOpacity( o );
		}

		const char* fillrule;
		if ( pathElement->QueryStringAttribute( "fill-rule", &fillrule) == XML_SUCCESS ) {
			_handler->onPathFillRule( fillrule );		
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
	
	int SVG::d_string_to_int( char *c, char **str ) {
		while ( isspace(*c) ) {
			c++;
			(*str)++;
		}
		while ( *c == ',' ) {
			c++;
			(*str)++;
		}
		
		return (int)strtol( c, str, 10);
		
	}
	
	uint32_t SVG::string_hex_color_to_uint( const string& hexstring ) {
		uint32_t color = (uint32_t)strtol( hexstring.c_str() + 1, 0, 16 );
        
		if ( hexstring.length() == 7 ) {	// fix up to rgba if the color is only rgb
			color = color << 8;
			color |= 0x000000ff;
		}
		
		return color;
	}	
	
	
	void SVG::nextState( char** c, char* state ) {
		if ( **c == '\0') {
			*state = 'e';
			return;
		}
		
		while ( isspace(**c) ) {
			if ( **c == '\0') {
				*state = 'e';
				return;
			}
			(*c)++;
		}
		if ( isalpha( **c ) ) {
			*state = **c;
			(*c)++;
//			if ( **c == '\0') {
//				*state = 'e';
//				return;
//			}
			
			if ( islower(*state) ) {	// if lower case then relative coords (see SVG spec)
				_handler->setRelative( true );
			} else {
				_handler->setRelative( false );
			}
		}
		
		//cout << "state: " << *state << endl;
	}
	
	void SVG::parse_path_transform( const string& tr )	{
		size_t p = tr.find( "translate" );
		if ( p != string::npos ) {
			size_t left = tr.find( "(" );
			size_t right = tr.find( ")" );
			string values = tr.substr( left+1, right-1 );
			char* c = const_cast<char*>( values.c_str() );
			float x = d_string_to_float( c, &c );
			float y = d_string_to_float( c, &c );
			_handler->onTransformTranslate( x, y );
		} else if ( tr.find( "rotate" ) != string::npos ) {
			size_t left = tr.find( "(" );
			size_t right = tr.find( ")" );
			string values = tr.substr( left+1, right-1 );
			char* c = const_cast<char*>( values.c_str() );
			float a = d_string_to_float( c, &c );
			_handler->onTransformRotate( a );	// ??? radians or degrees ??
		} else if ( tr.find( "matrix" ) != string::npos ) {
			size_t left = tr.find( "(" );
			size_t right = tr.find( ")" );
			string values = tr.substr( left+1, right-1 );
			char* cc = const_cast<char*>( values.c_str() );
			float a = d_string_to_float( cc, &cc );
			float b = d_string_to_float( cc, &cc );
			float c = d_string_to_float( cc, &cc );
			float d = d_string_to_float( cc, &cc );
			float e = d_string_to_float( cc, &cc );
			float f = d_string_to_float( cc, &cc );
			_handler->onTransformMatrix( a, b, c, d, e, f );
		}
	}
	
	void SVG::parse_path_d( const string& d ) {
		char* c = const_cast<char*>( d.c_str() );
		char state = *c;
		nextState( &c, &state );
		while ( /**c &&*/ state != 'e' ) {
			
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
				
				case 'h':
				case 'H':
				{
					float x = d_string_to_float( c, &c );
					_handler->onPathHorizontalLine( x );
					nextState(&c, &state);

				}
				break;

				case 'v':
				case 'V':
				{
					float y = d_string_to_float( c, &c );
					_handler->onPathVerticalLine( y );
					nextState(&c, &state);
					
				}
				break;
					
				case 'c':
				case 'C':
				{
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

				case 's':
				case 'S':
				{
					float x2 = d_string_to_float( c, &c );
					float y2 = d_string_to_float( c, &c );
					float x3 = d_string_to_float( c, &c );
					float y3 = d_string_to_float( c, &c );
					_handler->onPathSCubic(x2, y2, x3, y3);
					nextState(&c, &state);
					
				}
				break;
					
				case 'a':
				case 'A':
				{
					float rx = d_string_to_float( c, &c );
					float ry = d_string_to_float( c, &c );
					float x_axis_rotation = d_string_to_float( c, &c );
					int large_arc_flag = d_string_to_int( c, &c );
					int sweep_flag = d_string_to_int( c, &c );
					float x = d_string_to_float( c, &c );;
					float y = d_string_to_float( c, &c );
					_handler->onPathArc( rx, ry, x_axis_rotation, large_arc_flag, sweep_flag, x, y );
					nextState(&c, &state);
					
				}
				break;
					
				case 'z':
				case 'Z':
				{
					//c++;
					_handler->onPathClose();
					nextState(&c, &state);
					
				}
				break;
                    
                case 'q':
                case 'Q':
                {
                    float x1 = d_string_to_float(c, &c);
                    float y1 = d_string_to_float(c, &c);
                    float x2 = d_string_to_float(c, &c);
                    float y2 = d_string_to_float(c, &c);
                    _handler->onPathQuad(x1, y1, x2, y2);
                    nextState(&c, &state);
                }
                break;
					
				default:
					// BUGBUG: can get stuck here!
					// TODO: figure out the next state if we don't handle a particular state or just dummy handle a state!
					c++;
					break;
			}
		}
	}
	
    void SVG::parse_path_stylesheet( string ps ) {

        if (_styleDocument.getElementCount()) {

            StyleSheet::CssElement theElement = _styleDocument.getElement(StyleSheet::CssSelector::CssClassSelector(ps));
            
            if (theElement.getPropertyCount()) {
                
                StyleSheet::CssProperty property = theElement.getProperties().getProperty("fill");
                std::string value = property.getValue();
                
                if ( !value.empty() ) {
                    if( value != "none" && !value.empty())
                        _handler->onPathFillColor( string_hex_color_to_uint( value ) );
                }
                
                property = theElement.getProperties().getProperty("fill-rule");
                value = property.getValue();
                if ( !value.empty() ) {
                    _handler->onPathFillRule( value );
                }
                
                property = theElement.getProperties().getProperty("fill-opacity");
                value = property.getValue();
                if ( !value.empty() ) {
                    float o = atof( value.c_str() );
                    _handler->onPathFillOpacity( o );
                }
                
                property = theElement.getProperties().getProperty("stroke");
                value = property.getValue();
                
                if ( !value.empty() ) {
                    if( value != "none" && !value.empty())
                        _handler->onPathStrokeColor( string_hex_color_to_uint( value ) );
                }
                
                property = theElement.getProperties().getProperty("stroke-width");
                value = property.getValue();
                if ( !value.empty() ) {
                    float width = atof( value.c_str() );
                    _handler->onPathStrokeWidth( width );
                }
                
/*                property = theElement.getProperties().getProperty( "stroke-linecap" );
                value = property.getValue();
                if ( value != "none" && !value.empty() ) {
                    _handler->onPathStrokeCapStyle( value );
                }

                property = theElement.getProperties().getProperty( "stroke-linejoin" );
                value = property.getValue();
                if ( value != "none" && !value.empty() ) {
                    _handler->onPathStrokeLineJoin( value );
                }
 
                property = theElement.getProperties().getProperty( "stroke-dasharray" );
                value = property.getValue();
                if ( value != "none" && !value.empty() ) {
                    vector<int> dasharray;
                    StyleSheet::tokenizer(value, ", \t", [&dasharray](const string& token)
                    {
                        dasharray.push_back(atoi(token.c_str()));
                    });
                    _handler->onPathStrokDashPattern((int *)dasharray.data());
                }
                
                property = theElement.getProperties().getProperty( "stroke-miterlimit" );
                value = property.getValue();
                if ( !value.empty()) {
                    float o = atof( value.c_str() );
                    _handler->onPathStrokeMiterLimit( o );
                }
*/
                property = theElement.getProperties().getProperty("stroke-opacity");
                value = property.getValue();
                if ( !value.empty() ) {
                    float o = atof( value.c_str() );
                    _handler->onPathStrokeOpacity( o );
                }
                
                property = theElement.getProperties().getProperty("opacity");
                value = property.getValue();
                if ( !value.empty() ) {
                    float o = atof( value.c_str() );
                    _handler->onPathFillOpacity( o );
                }
                
            }
            
        }
    }
    
	// semicolon-separated property declarations of the form "name : value" within the ‘style’ attribute
	void SVG::parse_path_style( const string& ps ) {
		map< string, string > style_key_values;

        char *initial_str = new char[ps.length() + 1];
        char *str = initial_str;
        strcpy(str, ps.c_str());
        const char* const style_separator = ";";
        const char keyval_separator = ':';
        char *values = strtok(str, style_separator);
        
        while (values != NULL) {
            char *value = strchr(values, keyval_separator);
            char *key = values;
            if (value != NULL) {
                *value = '\0';
                ++value;
                style_key_values[string(key)] = string(value);
            }
            
            values = strtok(NULL, style_separator);
        }
        
        delete [] initial_str;
		
		map<string, string>::iterator kv = style_key_values.find( string("fill") );
		if ( kv != style_key_values.end() ) {
			if( kv->second != "none" )
				_handler->onPathFillColor( string_hex_color_to_uint( kv->second ) );
		}

        kv = style_key_values.find( "fill-rule" );
        if ( kv != style_key_values.end() ) {
            _handler->onPathFillRule( kv->second );
        }
        
        kv = style_key_values.find( "fill-opacity" );
        if ( kv != style_key_values.end() ) {
            float o = atof( kv->second.c_str() );
            _handler->onPathFillOpacity( o );
        }
        

		kv = style_key_values.find( "stroke" );
		if ( kv != style_key_values.end() ) {
			if( kv->second != "none" )
				_handler->onPathStrokeColor( string_hex_color_to_uint( kv->second ) );
		}
		
		kv = style_key_values.find( "stroke-width" );
		if ( kv != style_key_values.end() ) {
			float width = atof( kv->second.c_str() );
			_handler->onPathStrokeWidth( width );
		}
		
/*        kv = style_key_values.find( "stroke-linejoin" );
        if ( kv != style_key_values.end() ) {
            if ( kv->second != "none" ) {
                _handler->onPathStrokeLineJoin( kv->second );
            }
        }
        
        kv = style_key_values.find( "stroke-linecap" );
        if ( kv != style_key_values.end() ) {
            if ( kv->second != "none" ) {
                _handler->onPathStrokeCapStyle( kv->second );
            }
        }
 
        kv = style_key_values.find( "stroke-dasharray" );
        if ( kv != style_key_values.end() ) {
            if ( kv->second != "none" ) {
                vector<int> dasharray;
                StyleSheet::tokenizer(kv->second, ", \t", [&dasharray](const string& token)
                                      {
                                          dasharray.push_back(atoi(token.c_str()));
                                      });
                _handler->onPathStrokDashPattern((int *)dasharray.data());
            }
        }
        
        kv = style_key_values.find( "stroke-miterlimit" );
        if ( kv != style_key_values.end() ) {
            float o = atof( kv->second.c_str() );
            _handler->onPathStrokeMiterLimit( o );
        }
*/
        kv = style_key_values.find( "stroke-opacity" );
        if ( kv != style_key_values.end() ) {
            float o = atof( kv->second.c_str() );
            _handler->onPathStrokeOpacity( o );
        }

		kv = style_key_values.find( "opacity" );
		if ( kv != style_key_values.end() ) {
			float o = atof( kv->second.c_str() );
			_handler->onPathFillOpacity( o );
			// ?? TODO: stroke Opacity???
		}
    }

    void SVG::parse_polyline_points( const string& points ) {
        float xy[2];
        int xy_offset = 0;  // 0:x, 1:y
        bool first = true;
        _handler->setRelative(false);
        StyleSheet::tokenizer(points, ", \t", [&xy, &xy_offset, &first, this](const string& token)
                              {
                                  xy[xy_offset++] = (float)atof(token.c_str());

                                  if (xy_offset == 2) {
                                      xy_offset = 0;
                                      if (first) {
                                          _handler->onPathMoveTo( xy[0], xy[1] );
                                          first = false;
                                      } else
                                          _handler->onPathLineTo( xy[0], xy[1] );
                                  }
                              });
        //_handler->onPathClose();
    }
    
	void SVG::parse_points( const string& points ) {
        float xy[2];
        int xy_offset = 0;  // 0:x, 1:y
        bool first = true;
        _handler->setRelative(false);
        char *str = new char[points.length() + 1];
        strcpy(str, points.c_str());
        char *p = strtok(str, ", \t");
        while (p != NULL) {
            xy[xy_offset++] = (float)atof(p);
            
            if (xy_offset == 2) {
                xy_offset = 0;
                if (first) {
                    _handler->onPathMoveTo( xy[0], xy[1] );
                    first = false;
                } else
                    _handler->onPathLineTo( xy[0], xy[1] );
            }
            
            p = strtok(NULL, ", \t");
        }
        
        delete [] str;
        _handler->onPathClose();
	}
    
    MKSVGHandler::MKSVGHandler()
    :	_minX( MAXFLOAT )
    ,	_minY( MAXFLOAT )
    ,	_width( -MAXFLOAT )
    ,	_height( -MAXFLOAT )
    ,	_mode( kGroupParseMode )
    ,	_current_group( &_root_group )
    ,	_blackBackFill( 0 )
    ,	_batch( 0 )
    ,	_use_opacity( 1 )
    ,   _has_transparent_colors( false )

    ,	_error( VG_NO_ERROR )
    ,	_stroke_line_width( 1.0f )
    ,	_stroke_paint( 0 )
    ,	_fill_paint( 0 )
    ,	_active_matrix( &_path_user_to_surface )
    ,	_fill_rule( VG_EVEN_ODD )
    ,	_renderingQuality( VG_RENDERING_QUALITY_BETTER )
    ,	_tessellationIterations( 16 )
    ,	_matrixMode( VG_MATRIX_PATH_USER_TO_SURFACE )
    ,	_currentBatch( 0 )

    {
        disableAutomaticCleanup();

        _blackBackFill = createPaint();
        VGfloat fcolor[4] = { 0,0,0,1 };
        _blackBackFill->setPaintColor(fcolor);
        _use_transform.setIdentity();
        
        //_root_transform.setScale( 1, -1 );
        
    }
    
    MKSVGHandler::~MKSVGHandler() {
        delete _blackBackFill;
        
        if( _batch ) delete _batch;
    }
    
    
    void MKSVGHandler::draw() {
        // clear out the transform stack
        _transform_stack.clear();
        
        float m[9];
        transform( m );
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
            setTransform( topTransform().m );
            loadGLMatrix();

            _batch->draw();
        } else {
            draw_recursive( _root_group );
        }
        
        setTransform( m );	// restore matrix
        loadGLMatrix();

        _transform_stack.clear();
    }
    
    void MKSVGHandler::draw_recursive( group_t& group ) {
        
        // push the group matrix onto the stack
        pushTransform( group.transform ); setTransform( topTransform().m );
        
        for ( list<path_object_t>::iterator it = group.path_objects.begin(); it != group.path_objects.end(); it++ ) {
            path_object_t& po = *it;
            uint32_t draw_params = 0;
            if ( po.fill ) {
                setFillPaint(po.fill);
                draw_params |= VG_FILL_PATH;
            }
            
            if ( po.stroke ) {
                setStrokePaint( po.stroke );
                set( VG_STROKE_LINE_WIDTH, po.stroke_width );
                draw_params |= VG_STROKE_PATH;
            }
            
            if( draw_params == 0 ) {	// if no stroke or fill use the default black fill
                setFillPaint( _blackBackFill );
                draw_params |= VG_FILL_PATH;
            }
            
            // set the fill rule
            set( VG_FILL_RULE, po.fill_rule );
            // trasnform
            pushTransform( po.transform );	setTransform( topTransform().m );
            po.path->draw( draw_params );
            popTransform();	setTransform( topTransform().m );
        }
        
        for ( list<group_t>::iterator it = group.children.begin(); it != group.children.end(); it++ ) {
            draw_recursive( *it );
        }
        
        popTransform();	setTransform( topTransform().m );
    }
    
    void MKSVGHandler::optimize() {
        
        if( _batch )
        {
            delete _batch;
            _batch = 0;
        }
        // use the monkvg batch extension to greatly optimize rendering.  don't need this for
        // other OpenVG implementations
        _batch = createBatch();
        
        startBatch( _batch ); { // draw
            
            // clear out the transform stack
            _transform_stack.clear();
            
            float m[9];
            transform( m );
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
            
            setTransform( m );	// restore matrix
            _transform_stack.clear();
            
            
        } endBatch( _batch );
        
    }
        
    void MKSVGHandler::onGroupBegin() {
        _mode = kGroupParseMode;
        _current_group->children.push_back( group_t() );
        group_t* top = &_current_group->children.back();
        top->parent = _current_group;
        _current_group = top;
        // copy any use transform
        _current_group->transform = _use_transform;
    }
    void MKSVGHandler::onGroupEnd() {
        _current_group = _current_group->parent;
    }
    
    
    void MKSVGHandler::onPathBegin() {
        _mode = kPathParseMode;
        _current_group->current_path = new path_object_t();
        _current_group->current_path->path = createPath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                                                          1,0,0,0, VG_PATH_CAPABILITY_ALL);
        // inherit group settings
        _current_group->current_path->fill			= _current_group->fill;
        _current_group->current_path->stroke		= _current_group->stroke;
        _current_group->current_path->stroke_width	= _current_group->stroke_width;
        _current_group->current_path->fill_rule		= _current_group->fill_rule;
        
    }
    
    void MKSVGHandler::onPathEnd() {
        
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
    
    void MKSVGHandler::onPathMoveTo( float x, float y ) {
        VGubyte seg = VG_MOVE_TO | openVGRelative();
        VGfloat data[2];
        
        data[0] = x; data[1] = y;
        _current_group->current_path->path->appendData( 1, &seg, data );
        
    }
    
    void MKSVGHandler::onPathClose(){
        VGubyte seg = VG_CLOSE_PATH;
        VGfloat data = 0.0f;
        _current_group->current_path->path->appendData( 1, &seg, &data );
        
    }
    void MKSVGHandler::onPathLineTo( float x, float y ) {
        VGubyte seg = VG_LINE_TO | openVGRelative();
        VGfloat data[2];
        
        data[0] = x; data[1] = y;
        _current_group->current_path->path->appendData( 1, &seg, data );
        
    }
    
    void MKSVGHandler::onPathHorizontalLine( float x ) {
        VGubyte seg = VG_HLINE_TO | openVGRelative();
        VGfloat data[1];
        data[0] = x;
        _current_group->current_path->path->appendData( 1, &seg, data );
        
    }
    void MKSVGHandler::onPathVerticalLine( float y ) {
        VGubyte seg = VG_VLINE_TO | openVGRelative();
        VGfloat data[1];
        data[0] = y;
        _current_group->current_path->path->appendData( 1, &seg, data );
        
    }
    
    void MKSVGHandler::onPathCubic( float x1, float y1, float x2, float y2, float x3, float y3 ) {
        VGubyte seg = VG_CUBIC_TO | openVGRelative();
        VGfloat data[6];
        
        data[0] = x1; data[1] = y1;
        data[2] = x2; data[3] = y2;
        data[4] = x3; data[5] = y3;
        _current_group->current_path->path->appendData( 1, &seg, data);
        
    }
    
    void MKSVGHandler::onPathSCubic( float x2, float y2, float x3, float y3 ) {
        VGubyte seg = VG_SCUBIC_TO | openVGRelative();
        VGfloat data[4];
        
        data[0] = x2; data[1] = y2;
        data[2] = x3; data[3] = y3;
        _current_group->current_path->path->appendData( 1, &seg, data);
        
    }
    
    void MKSVGHandler::onPathQuad( float x1, float y1, float x2, float y2) {
        VGubyte seg = VG_QUAD_TO | openVGRelative();
        VGfloat data[4];
        data[0] = x1; data[1] = y1;
        data[2] = x2; data[3] = y2;
        _current_group->current_path->path->appendData( 1, &seg, data);
    }
    
    void MKSVGHandler::onPathArc( float rx, float ry, float x_axis_rotation, int large_arc_flag, int sweep_flag, float x, float y ) {
        
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
        
        _current_group->current_path->path->appendData( 1, &seg, data);
        
    }
    
    void MKSVGHandler::onPathRect( float x, float y, float w, float h ) {
        _current_group->current_path->path->vguRect( x, y, w, h );
    }
    
    
    void MKSVGHandler::onPathFillColor( unsigned int color ) {
        if( _mode == kGroupParseMode ) {
            _current_group->fill = createPaint();
            VGfloat fcolor[4] = { VGfloat( (color & 0xff000000) >> 24)/255.0f,
                VGfloat( (color & 0x00ff0000) >> 16)/255.0f,
                VGfloat( (color & 0x0000ff00) >> 8)/255.0f,
                _use_opacity };
            _current_group->fill->setPaintColor(fcolor);
            
        } else {
            _current_group->current_path->fill = createPaint();
            VGfloat fcolor[4] = { VGfloat( (color & 0xff000000) >> 24)/255.0f,
                VGfloat( (color & 0x00ff0000) >> 16)/255.0f,
                VGfloat( (color & 0x0000ff00) >> 8)/255.0f,
                _use_opacity  };
            _current_group->current_path->fill->setPaintColor(fcolor);
        }
    }
    
    void MKSVGHandler::onPathFillOpacity( float o ) {
        VGfloat fcolor[4];
        if( _mode == kGroupParseMode ) {
            if( _current_group->fill == 0 ) {	// if no fill create a black fill
                _current_group->fill = createPaint();
                VGfloat fcolor[4] = { 0,0,0,1 };
                _current_group->fill->setPaintColor(fcolor);
            }
            _current_group->fill->getPaintColor( &fcolor[0] );
            // set the opacity
            fcolor[3] = o;
            _current_group->fill->setPaintColor(fcolor);
            
        } else if( _mode == kPathParseMode ) {
            if( _current_group->current_path->fill == 0 ) {	// if no fill create a black fill
                _current_group->current_path->fill = createPaint();
                VGfloat fcolor[4] = { 0,0,0,1 };
                _current_group->fill->setPaintColor(fcolor);
            }
            
            _current_group->current_path->fill->getPaintColor( &fcolor[0] );
            // set the opacity
            fcolor[3] = o;
            _current_group->current_path->fill->setPaintColor(fcolor);
        } else if( _mode == kUseParseMode ) {
            _use_opacity = o;
            if( _current_group->fill == 0 ) {	// if no fill create a black fill
                _current_group->fill = createPaint();
                VGfloat fcolor[4] = { 0,0,0,1 };
                _current_group->fill->setPaintColor(fcolor);
            }
            _current_group->fill->getPaintColor( &fcolor[0] );
            // set the opacity
            fcolor[3] = o;
            _current_group->fill->setPaintColor(fcolor);
            _use_opacity = o;
            
        }
        _has_transparent_colors = _has_transparent_colors || (o < 1.0f);
    }
    void MKSVGHandler::onPathStrokeColor( unsigned int color ) {
        if( _mode == kGroupParseMode ) {
            _current_group->stroke = createPaint();
            VGfloat fcolor[4] = { VGfloat( (color & 0xff000000) >> 24)/255.0f,
                VGfloat( (color & 0x00ff0000) >> 16)/255.0f,
                VGfloat( (color & 0x0000ff00) >> 8)/255.0f,
                _use_opacity };
            _current_group->stroke->setPaintColor(fcolor);
        } else {
            _current_group->current_path->stroke = createPaint();
            VGfloat fcolor[4] = { VGfloat( (color & 0xff000000) >> 24)/255.0f,
                VGfloat( (color & 0x00ff0000) >> 16)/255.0f,
                VGfloat( (color & 0x0000ff00) >> 8)/255.0f,
                _use_opacity };
            _current_group->current_path->stroke->setPaintColor(fcolor);
        }
    }
    void MKSVGHandler::onPathStrokeOpacity( float o ) {
        VGfloat fcolor[4];
        if( _mode == kGroupParseMode ) {
            _current_group->stroke->getPaintColor( &fcolor[0] );
            // set the opacity
            fcolor[3] = o;
            _current_group->stroke->setPaintColor(fcolor);
            
        } else {
            _current_group->current_path->stroke->getPaintColor( &fcolor[0] );
            // set the opacity
            fcolor[3] = o;
            _current_group->current_path->stroke->setPaintColor(fcolor);
        }
        _has_transparent_colors = _has_transparent_colors || (o < 1.0f);
    }
    
    void MKSVGHandler::onPathStrokeWidth( float width ) {
        if( _mode == kGroupParseMode ) {
            _current_group->stroke_width = width;
        } else {
            _current_group->current_path->stroke_width = width;
        }
    }
    
    void MKSVGHandler::onPathFillRule( const string& rule ) {
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
    
    void MKSVGHandler::onTransformTranslate( float x, float y ) {
        if( _mode == kGroupParseMode ) {
            _current_group->transform.setTranslate( x, y );
        } else if( kPathParseMode ) {	// 
            _current_group->current_path->transform.setTranslate( x, y );
        }
    }
    void MKSVGHandler::onTransformScale( float s ) {
        if( _mode == kGroupParseMode ) {
            _current_group->transform.setScale( s, s );
        } else if( _mode == kPathParseMode ) { // kPathParseMode
            _current_group->current_path->transform.setScale( s, s );
        } else if( _mode == kUseParseMode ) {
            _use_transform.setScale( s, s );
        }
    }
    void MKSVGHandler::onTransformRotate( float r ) {
        if( _mode == kGroupParseMode ) {
            _current_group->transform.setRotation( r );	// ?? radians or degrees ??
        } else if( _mode == kPathParseMode ) { // kPathParseMode
            _current_group->current_path->transform.setRotation( r );	// ?? radians or degrees ??
        } else if( _mode == kUseParseMode ) {
            _use_transform.setRotation( r );
        }
    }
    void MKSVGHandler::onTransformMatrix( float a, float b, float c, float d, float e, float f ) {
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
    
    void MKSVGHandler::onId( const std::string& id_ ) {
        if( _mode == kGroupParseMode ) {
            _current_group->id  = id_;
        } else if( _current_group->current_path ) { // kPathParseMode
            _current_group->current_path->id = id_;
        }
    }
    
    void MKSVGHandler::onUseBegin() {
        _mode = kUseParseMode;
        _use_opacity = 1.0;
    }
    void MKSVGHandler::onUseEnd() {
        _use_transform.setIdentity();
        _use_opacity = 1.0;
    }

    
    //// parameters ////
    void MKSVGHandler::set( VGuint type, VGfloat f ) {
        switch ( type ) {
            case VG_STROKE_LINE_WIDTH:
                setStrokeLineWidth( f );
                break;
            default:
                assert(false);
                break;
        }
    }
    
    void MKSVGHandler::set( VGuint type, VGint i ) {
        
        switch ( type ) {
            case VG_FILL_RULE:
                setFillRule( (VGFillRule)i );
                break;
            case VG_TESSELLATION_ITERATIONS_MNK:
                setTessellationIterations( i );
                break;
            default:
                assert(false);
                break;
        }
        
    }
    void MKSVGHandler::get( VGuint type, VGfloat &f ) const {
        switch ( type ) {
            case VG_STROKE_LINE_WIDTH:
                f = getStrokeLineWidth();
                break;
            default:
                break;
        }
    }
    void MKSVGHandler::get( VGuint type, VGint& i ) const {
        i = -1;
        
        switch ( type ) {
            case VG_FILL_RULE:
                i =  getFillRule( );
                break;
            case VG_TESSELLATION_ITERATIONS_MNK:
                i = getTessellationIterations( );
                break;
            case VG_SURFACE_WIDTH_MNK:
                i = getWidth();
                break;
            case VG_SURFACE_HEIGHT_MNK:
                i = getHeight();
                break;
                
            default:
                break;
        }
    }
}

#include "mkPath.h"
#include "mkPaint.h"
#include "mkBatch.h"


namespace MonkSVG {
    void MKSVGHandler::checkGLError() {
        
        int err = glGetError();
        
        
        const char* RVAL = "GL_UNKNOWN_ERROR";
        
        switch( err )
        {
            case GL_NO_ERROR:
                RVAL =  "GL_NO_ERROR";
                break;
            case GL_INVALID_ENUM:
                RVAL =  "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                RVAL =  "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                RVAL = "GL_INVALID_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                RVAL =  "GL_OUT_OF_MEMORY";
                break;
            default:
                break;
        }
        
        if( err != GL_NO_ERROR ) {
            printf("GL_ERROR: %s\n", RVAL );
            assert(false);
        }
    }
    
    bool MKSVGHandler::Initialize() {
        glGetIntegerv(GL_VIEWPORT, _viewport);
        resize();
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        
        return true;
    }
    
    void MKSVGHandler::resize() {
        // setup GL projection
        glViewport(0,0, _width, _height);
    }
    
    
    bool MKSVGHandler::Terminate() {
        _stroke_paint = NULL;
        _fill_paint = NULL;
        return true;
    }
    
    
    void MKSVGHandler::beginRender() {
    }
    void MKSVGHandler::endRender() {
    }
    
    
    /// factories
    
    MKPath* MKSVGHandler::createPath( VGint pathFormat, VGPathDatatype datatype, VGfloat scale, VGfloat bias, VGint segmentCapacityHint, VGint coordCapacityHint, VGbitfield capabilities ) {
        
        MKPath *path = new MKPath(this, pathFormat, datatype, scale, bias, segmentCapacityHint, coordCapacityHint, capabilities  &= VG_PATH_CAPABILITY_ALL);
        assert( path );
        return (MKPath*)path;
    }
    
    MKPaint* MKSVGHandler::createPaint() {
        MKPaint *paint = new MKPaint();
        assert( paint );
        return (MKPaint*)paint;
    }
    
    MKBatch* MKSVGHandler::createBatch() {
        MKBatch *batch = new MKBatch(this);
        assert( batch );
        return (MKBatch*)batch;
    }
    
    /// state
    void MKSVGHandler::setStrokePaint( MKPaint* paint ) {
        if ( paint != _stroke_paint ) {
            _stroke_paint = paint;
            MKPaint* glPaint = (MKPaint*)_stroke_paint;
            //glPaint->setGLState();
            if (glPaint)
                glPaint->setIsDirty( true );
        }
    }
    
    void MKSVGHandler::setFillPaint( MKPaint* paint ) {
        if ( paint != _fill_paint ) {
            _fill_paint = paint;
            MKPaint* glPaint = (MKPaint*)_fill_paint;
            //glPaint->setGLState();
            if (glPaint)
                glPaint->setIsDirty( true );
        }
        
    }
    
    
    void MKSVGHandler::stroke() {
        if ( _stroke_paint ) {
            MKPaint* glPaint = (MKPaint*)_stroke_paint;
            glPaint->setGLState();
            glPaint->setIsDirty( false );
            // set the fill paint to dirty
            if ( _fill_paint ) {
                glPaint = (MKPaint*)_fill_paint;
                glPaint->setIsDirty( true );
            }
        }
    }
    
    void MKSVGHandler::fill() {
        
        if ( _fill_paint && _fill_paint->getPaintType() == VG_PAINT_TYPE_COLOR ) {
            MKPaint* glPaint = (MKPaint*)_fill_paint;
            glPaint->setGLState();
            glPaint->setIsDirty( false );
            // set the stroke paint to dirty
            if ( _stroke_paint ) {
                glPaint = (MKPaint*)_stroke_paint;
                glPaint->setIsDirty( true );
            }
        }
    }
    
    void MKSVGHandler::startBatch( MKBatch* batch ) {
        assert( _currentBatch == 0 );	// can't have multiple batches going on at once
        _currentBatch = batch;
    }
    void MKSVGHandler::endBatch( MKBatch* batch ) {
        _currentBatch->finalize();
        _currentBatch = 0;
    }
    
    
    void MKSVGHandler::clear(VGint x, VGint y, VGint width, VGint height) {
        // TODO:
    }
    
    void MKSVGHandler::loadGLMatrix() {
        Matrix33& active = *getActiveMatrix();
// TODO send as uniform to shader         glLoadMatrixf( &mat44[0][0] );
    }
    
    
    
    void MKSVGHandler::setIdentity() {
        Matrix33* active = getActiveMatrix();
        *active = Matrix33();
    }
    
    void MKSVGHandler::transform( VGfloat* t ) {
        // a	b	0
        // c	d	0
        // tx	ty	1
        const Matrix33::value_type* active = glm::value_ptr(*getActiveMatrix());
        for( int i = 0; i < 9; i++ )
            t[i] = active[i];
        
    }
    
    void MKSVGHandler::setTransform( const VGfloat* t )  {
        //	OpenVG:
        //	sh	shx	tx
        //	shy	sy	ty
        //	0	0	1
        //
        // OpenGL
        // a	b	0
        // c	d	0
        // tx	ty	1
        
        Matrix33* active = getActiveMatrix();
        *active = glm::make_mat3(t);
    }
    
    
    void MKSVGHandler::multiply( const VGfloat* t ) {
        Matrix33 m = glm::make_mat3(t);
        Matrix33* active = getActiveMatrix();
        (*active) *= m;
    }
    
    void MKSVGHandler::scale( VGfloat sx, VGfloat sy ) {
        Matrix33* active = getActiveMatrix();
        *active = glm::scale(*active, v2_t(sx, sy));
    }
    void MKSVGHandler::translate( VGfloat x, VGfloat y ) {
        Matrix33* active = getActiveMatrix();
        *active = glm::translate(*active, v2_t(x, y));
    }
    float MKSVGHandler::angle()
    {
        Matrix33* active = getActiveMatrix();
        
        return std::acosf((*active)[0][0]);
    }
    
    void MKSVGHandler::rotate( VGfloat angle ) {
        Matrix33* active = getActiveMatrix();
        *active = glm::rotate(*active, angle);
    }
    
    void MKSVGHandler::rotate(VGfloat angle, VGfloat x, VGfloat y, VGfloat z) {
        
        translate(x, y);
        rotate(angle);
        
    }
}

SakaSVG::SakaSVG(float width, float height, const char* const svgFile)
{
    handler.setWidth( width );
    handler.setHeight( height );
    handler.Initialize();
    
    MonkSVG::SVG svg_parser;
    svg_parser.initialize( &handler );
    svg_parser.read( svgFile );
    
    handler.optimize();
}
SakaSVG::~SakaSVG()
{
    handler.Terminate();
}
void SakaSVG::resize(float width, float height)
{
    handler.setWidth(width);
    handler.setHeight(height);
    handler.resize();
}
void SakaSVG::draw()
{
    handler.draw();
}

