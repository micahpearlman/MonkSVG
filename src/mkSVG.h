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
#include <memory>
#include <list>
#include "mkMath.h"
#include "vgCompat.h"

namespace tinyxml2
{
    class XMLDocument;
    class XMLElement;
}

namespace MonkVG {
    class MKPaint;
    class MKBatch;
    class MKPath;
}

namespace MonkSVG {

    using namespace std;
    using namespace tinyxml2;
    using namespace MonkVG;
    
    class SVG;

    class MKSVGHandler {
	public:
        MKSVGHandler();
        ~MKSVGHandler();

        void draw();
        void optimize();
        
        const Matrix33& rootTransform() { return _root_transform; }
        void setRootTransform( const Matrix33& t ) { _root_transform = t; }
        
        const bool hasTransparentColors() { return _has_transparent_colors; }

        // groups
        void onGroupBegin();
        void onGroupEnd();
        
        // use
        void onUseBegin();
        void onUseEnd();
        
        
        // paths
        void onPathBegin();
        void onPathEnd();
        void onPathClose();
        void onPathMoveTo( float x, float y );
        void onPathLineTo( float x, float y );
        void onPathCubic( float x1, float y1, float x2, float y2, float x3, float y3 );
        void onPathSCubic( float x2, float y2, float x3, float y3 );
        void onPathHorizontalLine( float x );
        void onPathVerticalLine( float y );
        void onPathArc( float rx, float ry, float x_axis_rotation, int large_arc_flag, int sweep_flag, float x, float y );
        void onPathRect( float x, float y, float w, float h );
        
        void onPathQuad( float x1, float y1, float x2, float y2);
        
        
        // paint
        void onPathFillColor( unsigned int color );
        void onPathFillOpacity( float o );
        void onPathFillRule( const string& rule );
        
        
        // stroke
        void onPathStrokeColor( unsigned int color );
        void onPathStrokeWidth( float width );
        void onPathStrokeOpacity( float o );
        
        
        // transforms
        void onTransformTranslate( float x, float y );
        void onTransformScale( float s );
        void onTransformRotate( float r );
        void onTransformMatrix( float a, float b, float c, float d, float e, float f );
        
        // misc
        void onId( const std::string& id_ );
        
        uint32_t openVGRelative() {
            if ( relative() ) {
                return VG_RELATIVE;
            }
            return VG_ABSOLUTE;
        }
        
        
        void pushTransform( const Matrix33& t ) {
            if ( _transform_stack.size() == 0 ) {	// nothing on the stack so push the identity onto the stack
                _transform_stack.push_back( t );
            } else {
                const Matrix33& current_transform = topTransform();
                Matrix33 top_transform = current_transform * t;
                _transform_stack.push_back( top_transform );
            }
        }
        
        void popTransform() {
            _transform_stack.pop_back();
        }
        
        const Matrix33& topTransform() {
            return _transform_stack.back();
        }
		
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
		// bounds info
		float _minX;
		float _minY;
		float _width;
		float _height;	
		
		friend class SVG;
	private:
		bool _relative;
		
        struct path_object_t {
            MKPath*		path;
            MKPaint*		fill;
            VGFillRule	fill_rule;
            MKPaint*		stroke;
            float		stroke_width;
            Matrix33 transform;
            std::string id;
            
            path_object_t() : path( nullptr ), fill( nullptr ), stroke( nullptr ), stroke_width( -1 ), fill_rule( VG_NON_ZERO ) {
                
            }
            ~path_object_t();
        };
        
        struct group_t {
            group_t()
            :	parent( 0 )
            ,	current_path( 0 )
            ,	fill( 0 ), stroke( 0 ), stroke_width( -1 ), fill_rule( VG_NON_ZERO )
            {
                
            }
            Matrix33			transform;
            group_t*			parent;
            list<group_t>		children;
            list<path_object_t> path_objects;
            path_object_t*		current_path;
            std::string			id;
            
            MKPaint*		fill;
            VGFillRule	fill_rule;
            MKPaint*		stroke;
            float		stroke_width;
            
        };
        
        group_t		_root_group;
        group_t*	_current_group;
        
        
        vector<Matrix33>		_transform_stack;
        Matrix33				_root_transform;
        Matrix33				_use_transform;
        float					_use_opacity;
        
        enum mode {
            kGroupParseMode = 1,
            kPathParseMode = 2,
            kUseParseMode = 3
        };
        
        mode _mode;
        
        MKPaint*	_blackBackFill;		// if a path doesn't have a stroke or a fill then use this fill
        
        
        /// optimized batch monkvg batch object
        MKBatch*	_batch;
        
        // flag indicating if any of the fills or strokes in the image use transparent colors 
        // if there are no transparent colors in the image, blending can be disabled in open gl 
        // to improve rendering performance
        bool _has_transparent_colors;
        
        void draw_recursive( MKSVGHandler::group_t& group );
        
        
        //
        //
        // From MonkVG's mkContext
        // =======================
        //
        //
        
    public:
        //// Paints ////
        void setStrokePaint( MKPaint* paint );
        inline MKPaint* getStrokePaint() const {
            return _stroke_paint;
        }
        void setFillPaint( MKPaint* paint );
        inline MKPaint* getFillPaint() {
            return _fill_paint;
        }
        inline VGFillRule getFillRule() const {
            return _fill_rule;
        }
        inline void setFillRule( VGFillRule r ) {
            _fill_rule = r;
        }
        
        //// drawing context ////
        inline int getWidth() const {
            return _width;
        }
        inline void setWidth( int w ) {
            _width = w;
        }
        
        inline int getHeight() const {
            return _height;
        }
        inline void setHeight( int h ) {
            _height = h;
        }
        
        //// stroke parameters ////
        inline void setStrokeLineWidth( float w ) {
            _stroke_line_width = w;
        }
        inline float getStrokeLineWidth() const {
            return _stroke_line_width;
        }
        
        //// transforms ////
        inline int32_t getTessellationIterations() const { return _tessellationIterations; }
        inline void setTessellationIterations( int32_t i ) { _tessellationIterations = i; }
        
        MKBatch* currentBatch() { return _currentBatch; }
        
        Matrix33			_active_matrix;

    protected:
        
        // stroke properties
        float				_stroke_line_width;			// VG_STROKE_LINE_WIDTH
        
        // rendering quality
        int32_t				_tessellationIterations;
        
        // paints
        MKPaint*				_stroke_paint;
        MKPaint*				_fill_paint;
        VGFillRule			_fill_rule;
        
        // batch
        MKBatch*				_currentBatch;
        
    public:
        bool Initialize();
        bool Terminate();
        
        //// factories ////
        MKPaint* createPaint();
        MKPath* createPath();
        MKBatch* createBatch();
        
        //// platform specific execution of stroke and fill ////
        void stroke();
        void fill();
        
        //// platform specific execution of Masking and Clearing ////
        void clear(int x, int y, int width, int height);
        
        //// platform specific implementation of transform ////
        void setIdentity();
        const Matrix33& getTransform() const;
        void scale( float sx, float sy );
        void translate( float x, float y );
        float angle ();
        void rotate( float angle );
        void rotate( float angle , float x, float y, float z);
        void setTransform( const Matrix33& t ) ;
        void multiply( const Matrix33& t );
        void loadGLMatrix();
        
        void beginRender();
        void endRender();
        
        void resize();
        
        
        static void checkGLError();
        
        /// batch drawing
        void startBatch( MKBatch* batch );
        void endBatch( MKBatch* batch );
        
    private:
        
        // restore values to play nice with other apps
        int		_viewport[4];
        float	_projection[16];
        float	_modelview[16];
	};
	
	class SVG  {
	public:
		
		bool initialize( MKSVGHandler* handler );
		bool read( string& data );
		bool read( const char* data );

	private:
		
		MKSVGHandler*		_handler;
		
		// holds svg <symbols>
		map<string, XMLElement*>	_symbols;
		
	private:
		void recursive_parse( XMLElement* element );
		bool handle_xml_element( XMLElement* element );
		void handle_group( XMLElement* pathElement );
   		void handle_stylesheet( XMLElement* pathElement );
		void handle_line( XMLElement* pathElement );
        void handle_polyline( XMLElement* pathElement );
        void handle_path( XMLElement* pathElement );
		void handle_rect( XMLElement* pathElement );
		void handle_polygon( XMLElement* pathElement );
		void handle_general_parameter( XMLElement* pathElement );
		void parse_path_d( const string& ps );
		void parse_path_style( const string& ps );
		void parse_path_stylesheet( string ps );
        void parse_path_transform( const string& tr );
		void parse_points( const string& points );
        void parse_polyline_points( const string& points );
		uint32_t string_hex_color_to_uint( const string& hexstring );
		float d_string_to_float( char *c, char **str );
		int d_string_to_int( char *c, char **str );
		void nextState( char** c, char* state );
	};
}

class SakaSVG
{
public:
    MonkSVG::MKSVGHandler handler;
    
    SakaSVG(float width, float height, const char* const svgFile);
    ~SakaSVG();
    void resize(float width, float height);
    void draw();
};

#endif // __mkSVG_h__
