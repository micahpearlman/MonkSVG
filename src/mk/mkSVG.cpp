/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */

#include "mkSVG.h"
#include <tinyxml2/tinyxml2.h>
#include <StyleSheet/Document.h>
#include <map>
#include <iterator>
#include <regex>
#include <cstring>

#include "mkPath.h"
#include "mkMath.h"
#include "mkPaint.h"
#include "vgCompat.h"
#include <OpenGLES/ES2/gl.h>

#include <set>
#include <unordered_map>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include <Tess2/tess.h>

#include "SakaSVG.h"


namespace MonkVG {
    struct vertexData_t {
        MonkVG::Pos pos;
        MonkVG::Color color;
        
        vertexData_t(MonkVG::Pos::value_type x, MonkVG::Pos::value_type y, MonkVG::Color _color) :
        pos(x, y),
        color(_color)
        {
        }
        bool operator==(const vertexData_t& other) const
        {
            return pos == other.pos && color == other.color;
        }
    };
    struct __attribute__((packed)) gpuVertexData_t {
        MonkVG::GpuPos pos;
        MonkVG::Color color;
    };
}
template <> struct std::hash<MonkVG::vertexData_t>
{
    template <typename T> constexpr static
    T rotate_left(T val, size_t len)
    {
        return (val << len) | ((unsigned) val >> (-len & (sizeof(T) * CHAR_BIT - 1)));
    }
    
    std::size_t operator()(const MonkVG::vertexData_t& key) const
    {
        return
        std::hash<MonkVG::Pos>()(key.pos) ^
        rotate_left(std::hash<MonkVG::Color>()(key.color), 1);
    }
};

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
    ,   _batchMinX(INT_MAX)
    ,   _batchMinY(INT_MAX)
    ,   _batchMaxX(INT_MIN)
    ,   _batchMaxY(INT_MIN)
    ,   maxSizeX(0)
    ,   maxSizeY(0)
    ,   newMaxSizeX(0)
    ,   newMaxSizeY(0)
    ,   numDeletedId(0)
    ,	_mode( kGroupParseMode )
    ,	_current_group( &_root_group )
    ,	_blackBackFill( 0 )
    ,	_use_opacity( 1 )
    ,   _has_transparent_colors( false )

    ,	_stroke_line_width( 1.0f )
    ,	_stroke_paint( 0 )
    ,	_fill_paint( 0 )
    ,	_fill_rule( VG_EVEN_ODD )
    ,	_tessellationIterations( 4 )

    {
        _blackBackFill = createPaint();
        float fcolor[4] = { 0,0,0,1 };
        _blackBackFill->setPaintColor(fcolor);
        
        //_root_transform.setScale( 1, -1 );
        
    }
    
    MKSVGHandler::~MKSVGHandler() {
        delete _blackBackFill;
    }
    
    
    void MKSVGHandler::draw_recursive( group_t& group ) {
        
        // push the group matrix onto the stack
        pushTransform( group.transform ); setTransform( topTransform() );
        
        for ( list<path_object_t>::iterator it = group.path_objects.begin(); it != group.path_objects.end(); it++ ) {
            path_object_t& po = *it;
            uint32_t draw_params = 0;
            if ( po.fill ) {
                setFillPaint(po.fill);
                draw_params |= VG_FILL_PATH;
            }
            
            if ( po.stroke ) {
                setStrokePaint( po.stroke );
                setStrokeLineWidth(po.stroke_width);
                draw_params |= VG_STROKE_PATH;
            }
            
            if( draw_params == 0 ) {	// if no stroke or fill use the default black fill
                setFillPaint( _blackBackFill );
                draw_params |= VG_FILL_PATH;
            }
            
            // set the fill rule
            setFillRule(po.fill_rule);
            // trasnform
            pushTransform( po.transform );	setTransform( topTransform() );
            po.path->draw( draw_params );
            popTransform();	setTransform( topTransform() );
        }
        
        for ( list<group_t>::iterator it = group.children.begin(); it != group.children.end(); it++ ) {
            draw_recursive( *it );
        }
        
        popTransform();	setTransform( topTransform() );
    }
    
    void MKSVGHandler::optimize() {
        // clear out the transform stack
        _transform_stack.clear();
        
        Matrix33 m = getTransform();
        Matrix33 top = m * rootTransform();
        pushTransform( top );
        
        // SVG is origin at the top, left (openvg is origin at the bottom, left)
        // so need to flip
        //		Matrix33 flip;
        //		flip.setScale( 1, -1 );
        //		pushTransform( flip );
        
        draw_recursive( _root_group );
        
        setTransform( m );	// restore matrix
        _transform_stack.clear();
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
        _current_group->current_path->path = createPath();
        // inherit group settings
        _current_group->current_path->fill			= _current_group->fill;
        _current_group->current_path->stroke		= _current_group->stroke;
        _current_group->current_path->stroke_width	= _current_group->stroke_width;
        _current_group->current_path->fill_rule		= _current_group->fill_rule;
        
    }
    
    void MKSVGHandler::onPathEnd() {
        
        // onPathClose()
        
        _current_group->path_objects.push_back( *_current_group->current_path );
        // Do not delete what was copy-constructed, only delete the object itself 
        _current_group->current_path->path = nullptr;
        _current_group->current_path->fill = nullptr;
        _current_group->current_path->stroke = nullptr;
        delete _current_group->current_path;
        _current_group->current_path = nullptr;
    }
    
    void MKSVGHandler::onPathMoveTo( float x, float y ) {
        unsigned char seg = VG_MOVE_TO | openVGRelative();
        float data[2];
        
        data[0] = x; data[1] = y;
        _current_group->current_path->path->appendData( 1, &seg, data );
        
    }
    
    void MKSVGHandler::onPathClose(){
        unsigned char seg = VG_CLOSE_PATH;
        float data = 0.0f;
        _current_group->current_path->path->appendData( 1, &seg, &data );
        
    }
    void MKSVGHandler::onPathLineTo( float x, float y ) {
        unsigned char seg = VG_LINE_TO | openVGRelative();
        float data[2];
        
        data[0] = x; data[1] = y;
        _current_group->current_path->path->appendData( 1, &seg, data );
        
    }
    
    void MKSVGHandler::onPathHorizontalLine( float x ) {
        unsigned char seg = VG_HLINE_TO | openVGRelative();
        float data[1];
        data[0] = x;
        _current_group->current_path->path->appendData( 1, &seg, data );
        
    }
    void MKSVGHandler::onPathVerticalLine( float y ) {
        unsigned char seg = VG_VLINE_TO | openVGRelative();
        float data[1];
        data[0] = y;
        _current_group->current_path->path->appendData( 1, &seg, data );
        
    }
    
    void MKSVGHandler::onPathCubic( float x1, float y1, float x2, float y2, float x3, float y3 ) {
        unsigned char seg = VG_CUBIC_TO | openVGRelative();
        float data[6];
        
        data[0] = x1; data[1] = y1;
        data[2] = x2; data[3] = y2;
        data[4] = x3; data[5] = y3;
        _current_group->current_path->path->appendData( 1, &seg, data);
        
    }
    
    void MKSVGHandler::onPathSCubic( float x2, float y2, float x3, float y3 ) {
        unsigned char seg = VG_SCUBIC_TO | openVGRelative();
        float data[4];
        
        data[0] = x2; data[1] = y2;
        data[2] = x3; data[3] = y3;
        _current_group->current_path->path->appendData( 1, &seg, data);
        
    }
    
    void MKSVGHandler::onPathQuad( float x1, float y1, float x2, float y2) {
        unsigned char seg = VG_QUAD_TO | openVGRelative();
        float data[4];
        data[0] = x1; data[1] = y1;
        data[2] = x2; data[3] = y2;
        _current_group->current_path->path->appendData( 1, &seg, data);
    }
    
    void MKSVGHandler::onPathArc( float rx, float ry, float x_axis_rotation, int large_arc_flag, int sweep_flag, float x, float y ) {
        
        unsigned char seg = openVGRelative();
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
        float data[5];
        
        
        
        data[0] = rx;
        data[1] = ry;
        data[2] = x_axis_rotation;
        data[3] = x;
        data[4] = y;
        
        _current_group->current_path->path->appendData( 1, &seg, data);
        
    }
    
    void MKSVGHandler::onPathRect( float x, float y, float w, float h ) {
        static const unsigned char segments[5] = {
            VG_MOVE_TO | VG_ABSOLUTE,
            VG_HLINE_TO | VG_ABSOLUTE,
            VG_VLINE_TO | VG_ABSOLUTE,
            VG_HLINE_TO | VG_ABSOLUTE,
            VG_CLOSE_PATH};
        const float data[5] = {x, y, x + w, y + h, x};
        _current_group->current_path->path->appendData(5, segments, data);
    }
    
    
    void MKSVGHandler::onPathFillColor( unsigned int color ) {
        if( _mode == kGroupParseMode ) {
            _current_group->fill = createPaint();
            float fcolor[4] = { float( (color & 0xff000000) >> 24)/255.0f,
                float( (color & 0x00ff0000) >> 16)/255.0f,
                float( (color & 0x0000ff00) >> 8)/255.0f,
                _use_opacity };
            _current_group->fill->setPaintColor(fcolor);
            
        } else {
            _current_group->current_path->fill = createPaint();
            float fcolor[4] = { float( (color & 0xff000000) >> 24)/255.0f,
                float( (color & 0x00ff0000) >> 16)/255.0f,
                float( (color & 0x0000ff00) >> 8)/255.0f,
                _use_opacity  };
            _current_group->current_path->fill->setPaintColor(fcolor);
        }
    }
    
    void MKSVGHandler::onPathFillOpacity( float o ) {
        float fcolor[4];
        if( _mode == kGroupParseMode ) {
            if( _current_group->fill == 0 ) {	// if no fill create a black fill
                _current_group->fill = createPaint();
                float fcolor[4] = { 0,0,0,1 };
                _current_group->fill->setPaintColor(fcolor);
            }
            _current_group->fill->getPaintColor( &fcolor[0] );
            // set the opacity
            fcolor[3] = o;
            _current_group->fill->setPaintColor(fcolor);
            
        } else if( _mode == kPathParseMode ) {
            if( _current_group->current_path->fill == 0 ) {	// if no fill create a black fill
                _current_group->current_path->fill = createPaint();
                float fcolor[4] = { 0,0,0,1 };
                _current_group->current_path->fill->setPaintColor(fcolor);
            }
            
            _current_group->current_path->fill->getPaintColor( &fcolor[0] );
            // set the opacity
            fcolor[3] = o;
            _current_group->current_path->fill->setPaintColor(fcolor);
        } else if( _mode == kUseParseMode ) {
            _use_opacity = o;
            if( _current_group->fill == 0 ) {	// if no fill create a black fill
                _current_group->fill = createPaint();
                float fcolor[4] = { 0,0,0,1 };
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
            float fcolor[4] = { float( (color & 0xff000000) >> 24)/255.0f,
                float( (color & 0x00ff0000) >> 16)/255.0f,
                float( (color & 0x0000ff00) >> 8)/255.0f,
                _use_opacity };
            _current_group->stroke->setPaintColor(fcolor);
        } else {
            _current_group->current_path->stroke = createPaint();
            float fcolor[4] = { float( (color & 0xff000000) >> 24)/255.0f,
                float( (color & 0x00ff0000) >> 16)/255.0f,
                float( (color & 0x0000ff00) >> 8)/255.0f,
                _use_opacity };
            _current_group->current_path->stroke->setPaintColor(fcolor);
        }
    }
    void MKSVGHandler::onPathStrokeOpacity( float o ) {
        float fcolor[4];
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
            MonkVG::translate(_current_group->transform, x, y );
        } else if( kPathParseMode ) {	//
            
            MonkVG::translate(_current_group->current_path->transform, x, y );
        }
    }
    void MKSVGHandler::onTransformScale( float s ) {
        if( _mode == kGroupParseMode ) {
            MonkVG::scale(_current_group->transform, s, s );
        } else if( _mode == kPathParseMode ) { // kPathParseMode
            MonkVG::scale(_current_group->current_path->transform, s, s );
        } else if( _mode == kUseParseMode ) {
            MonkVG::scale(_use_transform, s, s);
        }
    }
    void MKSVGHandler::onTransformRotate( float r ) {
        if( _mode == kGroupParseMode ) {
            MonkVG::rotate(_current_group->transform, r );
        } else if( _mode == kPathParseMode ) {
            MonkVG::rotate(_current_group->current_path->transform, r );
        } else if( _mode == kUseParseMode ) {
            MonkVG::rotate(_use_transform, r );
        }
    }
    void MKSVGHandler::onTransformMatrix( float a, float b, float c, float d, float e, float f ) {
        Matrix33 t(a, b, c, d, e, f, 0.f, 0.f, 1.f);
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
        _use_transform = Matrix33();
        _use_opacity = 1.0;
    }
}


namespace MonkSVG {
    /// factories
    
    MKPath* MKSVGHandler::createPath( ) {
        
        MKPath *path = new MKPath(this);
        assert( path );
        return (MKPath*)path;
    }
    
    MKPaint* MKSVGHandler::createPaint() {
        MKPaint *paint = new MKPaint();
        assert( paint );
        return (MKPaint*)paint;
    }
    
    /// state
    void MKSVGHandler::setStrokePaint( MKPaint* paint ) {
        if ( paint != _stroke_paint ) {
            _stroke_paint = paint;
        }
    }
    
    void MKSVGHandler::setFillPaint( MKPaint* paint ) {
        if ( paint != _fill_paint ) {
            _fill_paint = paint;
        }
        
    }
    
    
    void MKSVGHandler::setIdentity() {
        _active_matrix = Matrix33();
    }
    
    const Matrix33& MKSVGHandler::getTransform( ) const {
        return _active_matrix;
    }
    
    void MKSVGHandler::setTransform( const Matrix33& t )  {
        _active_matrix = t;
    }
    
    
    void MKSVGHandler::multiply( const Matrix33& t ) {
        _active_matrix *= t;
    }
    
    void MKSVGHandler::scale( float sx, float sy ) {
        _active_matrix = glm::scale(_active_matrix, v2_t(sx, sy));
    }
    void MKSVGHandler::translate( float x, float y ) {
        _active_matrix = glm::translate(_active_matrix, v2_t(x, y));
    }
    float MKSVGHandler::angle()
    {
        return std::acosf((_active_matrix)[0][0]);
    }
    
    void MKSVGHandler::rotate( float angle ) {
        _active_matrix = glm::rotate(_active_matrix, angle);
    }
    
    void MKSVGHandler::rotate(float angle, float x, float y, float z) {
        
        translate(x, y);
        rotate(angle);
        
    }
    
    // Based on https://hal.archives-ouvertes.fr/inria-00072100/document
    static inline int32_t orientation(const int32_t a[2], const int32_t b[2], const int32_t c[2])
    {
        int32_t result = (a[0]-c[0]) * (b[1]-c[1]) - (a[1]-c[1]) * (b[0]-c[0]);
        return result;
    }
    static inline int intersectionTestVertex(const int32_t p1[2], const int32_t q1[2], const int32_t r1[2], const int32_t p2[2], const int32_t q2[2], const int32_t r2[2])
    {
        if (orientation(r2,p2,q1) >= 0)
            if (orientation(r2,q2,q1) <= 0)
                if (orientation(p1,p2,q1) > 0) return (orientation(p1,q2,q1) <= 0) ? 1 : false;
                else if (orientation(p1,p2,r1) < 0) return false;
                else return (orientation(q1,r1,p2) >= 0) ? 2 : false;
                else if (orientation(p1,q2,q1) > 0) return false;
                else if (orientation(r2,q2,r1) > 0) return false;
                else return (orientation(q1,r1,q2) >= 0) ? 3 : false;
                else if (orientation(r2,p2,r1) < 0) return false;
                else if (orientation(q1,r1,r2) >= 0) return (orientation(p1,p2,r1) >= 0) ? 4 : false;
                else if (orientation(q1,r1,q2) < 0) return false;
                else return (orientation(r2,r1,q2) >= 0) ? 5 : false;
    }
    static inline int intersectionTestEdge(const int32_t p1[2], const int32_t q1[2], const int32_t r1[2], const int32_t p2[2], const int32_t q2[2], const int32_t r2[2])
    {
        if (orientation(r2,p2,q1) >= 0)
            if (orientation(p1,p2,q1) >= 0) return (orientation(p1,q1,r2) >= 0) ? 1 : false;
            else if (orientation(q1,r1,p2) < 0) return false;
            else return (orientation(r1,p1,p2) >= 0) ? 2 : false;
            else if (orientation(r2,p2,r1) < 0) return false;
            else if (orientation(p1,p2,r1) < 0) return false;
            else if (orientation(p1,r1,r2) >= 0) return 3;
            else return (orientation(q1,r1,r2) >= 0) ? 4 : false;
    }
    static inline int intersection(const int32_t p1[2], const int32_t q1[2], const int32_t r1[2], const int32_t p2[2], const int32_t q2[2], const int32_t r2[2])
    {
        if ( orientation(p2,q2,p1) >= 0 )
            if ( orientation(q2,r2,p1) >= 0 )
                if ( orientation(r2,p2,p1) >= 0 ) return 1;
                else return intersectionTestEdge(p1,q1,r1,p2,q2,r2) << 1;
                else if ( orientation(r2,p2,p1) >= 0 ) return intersectionTestEdge(p1,q1,r1,r2,p2,q2) << 4;
                else return intersectionTestVertex(p1,q1,r1,p2,q2,r2) << 7;
                else if (orientation(q2,r2,p1) < 0) return intersectionTestVertex(p1,q1,r1,r2,p2,q2) << 10;
                else if (orientation(r2,p2,p1) >= 0) return intersectionTestEdge(p1,q1,r1,q2,r2,p2) << 13;
                else return intersectionTestVertex(p1,q1,r1,q2,r2,p2) << 16;
    };
    
    static const GLfloat precision = 0.01f;
    static const GLfloat precisionMult = 1.f/precision;
    std::map<int, int> stat;
    
    void MKSVGHandler::addTriangle(int32_t v[6], const MonkVG::Color& color)
    {
        int32_t* p = &v[0];
        int32_t* q = &v[2];
        int32_t* r = &v[4];
        
        // Remove null triangles
        if ((p[0] == q[0] && p[0] == r[0]) || (p[1] == q[1] && p[1] == r[1]) || (p[0] == q[0] && p[1] == q[1]) || (p[0] == r[0] && p[1] == r[1]) || (q[0] == r[0] && q[1] == r[1]))
        {
            return;
        }
        
        // Put leftmost first
        if (q[0] < p[0] && q[0] <= r[0])
        {
            std::swap(p,q); // q to the left
        }
        else if (r[0] < p[0] && r[0] <= q[0])
        {
            std::swap(p,r); // q to the left
        }
        
        // Make sure triangles are counterclockwise
        if (orientation(p, q, r) < 0)
        {
            std::swap(q,r);
        }
        
        // Find box
        const int32_t xmin = p[0];
        const int32_t xmax = std::max(q[0], r[0]);
        const int32_t ymin = std::min(p[1], std::min(q[1], r[1]));
        const int32_t ymax = std::max(p[1], std::max(q[1], r[1]));
        
        _batchMinX = std::min(_batchMinX, (GLfloat)xmin/precisionMult);
        _batchMaxX = std::max(_batchMaxX, (GLfloat)xmax/precisionMult);
        _batchMinY = std::min(_batchMinY, (GLfloat)ymin/precisionMult);
        _batchMaxY = std::max(_batchMaxY, (GLfloat)ymax/precisionMult);
        
        /*
         bool eraseIter;
         for (auto iter = trianglesByXMin.lower_bound(xmin - maxSizeX + 1); iter != trianglesByXMin.end();
         eraseIter ? iter = trianglesByXMin.erase(iter) : ++iter)
         {
         eraseIter = false;
         
         auto& t1(*iter->second);
         if (t1.max[1] <= ymin)
         {
         continue;
         }
         if (t1.min[1] >= ymax)
         {
         continue;
         }
         if (t1.max[0] <= xmin)
         {
         continue;
         }
         if (t1.min[0] >= xmax)
         {
         break;
         }
         
         // We got an optimization contender!
         int which = intersection(p, q, r, t1.p, t1.q, t1.r);
         
         auto addedStat = stat.emplace({which, 1});
         if (!addedStat.second)
         {
         ++addedStat.first->second;
         }
         if (which != 0)
         {
         // Optimized! TODO: just chop it!
         t1.id = -1;         // Please don't use it anymore (signal)
         eraseIter = true;
         ++numDeletedId;
         }
         }
         */
        
        // Add triangle to deque
        trianglesDb.push_back(triangle_t(
                                             (int)trianglesDb.size(), // id
                                             MonkVG::Pos(xmin, ymin), MonkVG::Pos(xmax, ymax), // min, max
                                             MonkVG::Pos(p[0], p[1]), MonkVG::Pos(q[0], q[1]), MonkVG::Pos(r[0], r[1]), // p,q,r
                                             color
                                             ));
        trianglesToAdd.push_back(&trianglesDb.back());
        newMaxSizeX = std::max(newMaxSizeX, xmax - xmin);
        newMaxSizeY = std::max(newMaxSizeY, ymax - ymin);
    }
    
    void MKSVGHandler::finalizeTriangleBatch()
    {
        for (auto triangleToAdd : trianglesToAdd)
        {
            trianglesByXMin.insert({triangleToAdd->min[0], triangleToAdd});
        }
        maxSizeX = newMaxSizeX;
        maxSizeY = newMaxSizeY;
        trianglesToAdd.clear();
    }
    
    void MKSVGHandler::addPathVertexData( GLfloat* fillVerts, size_t fillVertCnt, GLfloat* strokeVerts, size_t strokeVertCnt, GLbitfield paintModes ) {
        int32_t v[6];
        
        //printf("Adding %d fill %d stroke\n", (int)fillVertCnt, (int)strokeVertCnt);
        if ( paintModes & VG_FILL_PATH) {
            // get the paint color
            const float* fc = getFillPaint()->getPaintColor();
            
            const MonkVG::Color color(
                                      GLuint(fc[0] * 255.0f),
                                      GLuint(fc[1] * 255.0f),
                                      GLuint(fc[2] * 255.0f),
                                      GLuint(fc[3] * 255.0f));
            
            // get vertices and transform them
            for ( int i = 0; i < (int)fillVertCnt * 2 - 4; i+=6 ) {
                v2_t affine;
                affine = affineTransform(_active_matrix, &fillVerts[i + 0]);
                v[0] = static_cast<int32_t>(affine[0]*precisionMult);
                v[1] = static_cast<int32_t>(affine[1]*precisionMult);
                affine = affineTransform(_active_matrix, &fillVerts[i + 2]);
                v[2] = static_cast<int32_t>(affine[0]*precisionMult);
                v[3] = static_cast<int32_t>(affine[1]*precisionMult);
                affine = affineTransform(_active_matrix, &fillVerts[i + 4]);
                v[4] = static_cast<int32_t>(affine[0]*precisionMult);
                v[5] = static_cast<int32_t>(affine[1]*precisionMult);
                
                addTriangle(v, color);
            }
            finalizeTriangleBatch();
        }
        
        if ( paintModes & VG_STROKE_PATH) {
            // get the paint color
            const float* fc = getStrokePaint()->getPaintColor();
            
            const MonkVG::Color color(
                                      GLuint(fc[0] * 255.0f),
                                      GLuint(fc[1] * 255.0f),
                                      GLuint(fc[2] * 255.0f),
                                      GLuint(fc[3] * 255.0f));
            
            // get vertices and transform them
            int32_t* firstV = &v[0];    // Don't use a,b,c as these need to keep in order
            int32_t* secondV = &v[2];
            int32_t* thirdV = &v[4];
            
            int vertcnt = 0;
            for ( int i = 0; i < strokeVertCnt * 2; i+=2, vertcnt++ ) {
                v2_t affine = affineTransform(_active_matrix, &strokeVerts[i]) * precisionMult;
                
                // for stroke we need to convert from a strip to triangle
                switch ( vertcnt ) {
                    case 0:
                        firstV[0] = static_cast<int32_t>(affine[0]);
                        firstV[1] = static_cast<int32_t>(affine[1]);
                        break;
                    case 1:
                        secondV[0] = static_cast<int32_t>(affine[0]);
                        secondV[1] = static_cast<int32_t>(affine[1]);
                        break;
                    default:
                    {
                        thirdV[0] = static_cast<int32_t>(affine[0]);
                        thirdV[1] = static_cast<int32_t>(affine[1]);
                        
                        // Next will override what was in firstV.
                        std::swap(firstV, thirdV);
                        std::swap(firstV, secondV);
                        
                        addTriangle(v, color);
                        break;
                    }
                }
            }
            finalizeTriangleBatch();
        }
    }
    
    void MKSVGHandler::finalize(Saka::SVG* dest) {
        optimize();
        // Move triangles to vertexes
        int numVertices = (int)(trianglesDb.size() - numDeletedId) * 3;
        
        std::vector<GLuint> ebo;
        ebo.reserve(numVertices);
        
        std::vector<gpuVertexData_t> vbo;
        vbo.reserve(numVertices); // Note : numVertices is the maximum ever. It will ALWAYS be less than this.
        
        std::unordered_map<vertexData_t, size_t> vertexToId;
        
        GLfloat xSize = _batchMaxX - _batchMinX;
        GLfloat ySize = _batchMaxY - _batchMinY;
        
        auto addVertex = [&](int32_t x, int32_t y, MonkVG::Color color) -> GLuint
        {
            vertexData_t toAdd(x, y, color);
            auto found = vertexToId.find(toAdd);
            if (found == vertexToId.end())
            {
                auto id = vbo.size();
                GLushort xNorm = (GLushort)( ((GLfloat)x / precisionMult - _batchMinX) / xSize * 65535 );
                GLushort yNorm = 65535 - (GLushort)( ((GLfloat)y / precisionMult - _batchMinY) / ySize * 65535 );
                vbo.push_back({{xNorm, yNorm}, color});
                vertexToId.insert({toAdd, id});
                return (GLuint)id;
            }
            else
            {
                return (GLuint)found->second;
            }
        };
        
        for (auto iter : trianglesDb)
        {
            if (iter.id == -1)
            {
                continue;
            }
            ebo.push_back(addVertex(iter.p[0], iter.p[1], iter.color));
            ebo.push_back(addVertex(iter.q[0], iter.q[1], iter.color));
            ebo.push_back(addVertex(iter.r[0], iter.r[1], iter.color));
        }
        
        printf("numVertices = %d, numVbo = %d, numEbo = %d\n", (int)numVertices, (int)vbo.size(), (int)ebo.size());
        
        Saka::SharedVAO sVao = dest->vao = Saka::SharedVAO::make_shared();
        Saka::SharedVBO sVbo = Saka::SharedVBO::make_shared();
        Saka::SharedEBO sEbo = dest->ebo = Saka::SharedEBO::make_shared();
        Saka::GLMgr::instance().bind(sVao).bind(sVbo).bind(sEbo);
        sVbo->bufferData(vbo.size() * sizeof(gpuVertexData_t), &vbo[0], GL_STATIC_DRAW);
        sVbo->setNumAttribs(2);
        sVbo->addAttrib(2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(gpuVertexData_t), (GLvoid*)offsetof(gpuVertexData_t, pos));
        sVbo->addAttrib(4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(gpuVertexData_t), (GLvoid*)offsetof(gpuVertexData_t, color));
        sVao->addVbo(sVbo);
        sEbo->bufferData(ebo.size() * sizeof(GLuint), &ebo[0], GL_STATIC_DRAW, GL_TRIANGLES, (GLsizei)(ebo.size()), GL_UNSIGNED_INT);
        
        printf("\n");
        for (auto iter : stat)
        {
            printf("%5d : %d\n", iter.first, iter.second);
        }
    }
}

