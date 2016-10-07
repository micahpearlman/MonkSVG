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

namespace MonkSVG {
	
    StyleSheet::CssDocument _styleDocument;
	
	bool SVG::initialize( ISVGHandler::SmartPtr handler ) {
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
                
                property = theElement.getProperties().getProperty( "stroke-linecap" );
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
		
        kv = style_key_values.find( "stroke-linejoin" );
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
};
