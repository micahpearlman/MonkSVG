/*
 *  mkPath.h
 *  MonkVG-XCode
 *
 *  Created by Micah Pearlman on 2/22/09.
 *  Copyright 2009 Monk Games. All rights reserved.
 *
 */

#ifndef __mkPath_h__
#define __mkPath_h__

#include "mkMath.h"

#include <vector>
#include "mkPaint.h"
#include <list>
#include <vector>
#include "vec.hpp"
#include "math.hpp"
#include <Tess2/tesselator.h>
#include <VG/vgu.h>
#include <OpenGLES/ES2/gl.h>


namespace MonkSVG {
    class MKSVGHandler;
}

namespace MonkVG {
	
	class MKPath {
	public:
        void clear( VGbitfield caps );

		inline VGint getFormat() const {
			return _format;
		}
		inline void setFormat( const VGint f ) {
			_format = f;
		}
		
		inline VGPathDatatype getDataType() const {
			return _datatype;
		}
		inline void setDataType( const VGPathDatatype d ) {
			_datatype = d;
		}
		
		inline VGfloat getScale() const {
			return _scale;
		}
		inline void setScale( const VGfloat s ) {
			_scale = s;
		}
		
		inline VGfloat getBias() const {
			return _bias;
		}
		inline void setBias( const VGfloat b ) {
			_bias = b;
		}
		
		inline VGint getNumSegments() const {
			return _numSegments;
		}
		inline void setNumSegments( const VGint ns ) {
			_numSegments = ns;
		}
		
		inline VGint getNumCoords() const {
			return _numCoords;
		}
		inline void setNumCoords( const VGint nc ) {
			_numCoords = nc;
		}
		
		inline VGbitfield getCapabilities( ) const {
			return _capabilities;
		}
		inline void setCapabilities( const VGbitfield c ) {
			_capabilities = c;
		}
		
		inline bool getIsDirty() {
			return _isFillDirty;
		}
		inline void setIsDirty( bool b ) {
			_isFillDirty = b;
			_isStrokeDirty = b;
		}
		
        inline VGfloat  getMiterlimit ()
        {
            return getJoinStyle() == VG_JOIN_BEVEL ? 1.05f : _stroke_miterlimit;
        }
        inline VGJoinStyle getJoinStyle ()
        {
            return  _joinStyle;
        }
        inline VGCapStyle  getCapStyle ()
        {
            return _capStyle;
        }
        
        inline void setMiterlimit (VGfloat m)
        {
            _stroke_miterlimit = m;
        }
        
        inline void setJoinStyle (VGJoinStyle s)
        {
            _joinStyle = s;
        }
        
        inline void setCapStyle (VGCapStyle c)
        {
            _capStyle = c;
        }

		// bounds
		inline VGfloat getMinX() {
			return _minX;
		}
		inline VGfloat getMinY() {
			return _minY;
		}
		inline VGfloat getWidth() {
			return _width;
		}
		inline VGfloat getHeight() {
			return _height;
		}
		
		//// internal data manipulators ////
        bool draw( VGbitfield paintModes );
		void appendData( const VGint numSegments, const VGubyte * pathSegments, const void * pathData ) ;
		int32_t segmentToNumCoordinates(VGPathSegment segment);
		void copy( const MKPath& src, const Matrix33& transform );
		void buildFillIfDirty();
		

        MKPath( MonkSVG::MKSVGHandler* h, VGint f, VGPathDatatype dt, VGfloat s, VGfloat b, VGint ns, VGint nc, VGbitfield cap )
		:   _handler(h)
        ,   _format( f )
		,	_datatype( dt )
		,	_scale( s )
		,	_bias( b )
		,	_numSegments( ns )
		,	_numCoords( nc )
		,	_capabilities( cap )
		,	_isFillDirty( true )
		,	_isStrokeDirty( true )
		,	_minX( VG_MAX_FLOAT )
		,	_minY( VG_MAX_FLOAT )
		,	_width( -VG_MAX_FLOAT )
		,	_height( -VG_MAX_FLOAT )
        ,	_fillTesseleator( 0 )
        ,	_strokeVBO(-1)
        ,	_fillVBO(-1)
		{
			switch (_datatype) {
				case VG_PATH_DATATYPE_F:
					_fcoords = new std::vector<float>( _numCoords );
					break;
				default:
					// error 
					break;
			}
			
		}
		
        ~MKPath();
        
        
    public:
        //
        // From mkVGU.cpp
        // ==============
        //
        void append(int numSegments, const VGubyte* segments, int numCoordinates, const VGfloat* coordinates);
        void vguLine(VGfloat x0, VGfloat y0, VGfloat x1, VGfloat y1);
        void vguPolygon(const VGfloat * points, VGint count, VGboolean closed);
        void vguRect(VGfloat x, VGfloat y, VGfloat width, VGfloat height);
        void vguEllipse(VGfloat cx, VGfloat cy, VGfloat width, VGfloat height);
        void vguRoundRect(VGfloat x, VGfloat y, VGfloat width, VGfloat height, VGfloat arcWidth, VGfloat arcHeight);
        void vguArc(VGfloat x, VGfloat y, VGfloat width, VGfloat height, VGfloat startAngle, VGfloat angleExtent, VGUArcType arcType);

		
	protected:

		VGint				_format;		// VG_PATH_FORMAT
		VGPathDatatype		_datatype;		// VG_PATH_DATATYPE
		VGfloat				_scale;			// VG_PATH_SCALE
		VGfloat				_bias;			// VG_PATH_BIAS
		VGint				_numSegments;	// VG_PATH_NUM_SEGMENTS
		VGint				_numCoords;		// VG_PATH_NUM_COORDS
		VGbitfield			_capabilities;
		
		// data
		std::vector< VGubyte >	_segments;
		std::vector< VGfloat >	*_fcoords;
		bool				_isFillDirty;
		bool				_isStrokeDirty;
		
		// bounds
		VGfloat				_minX;
		VGfloat				_minY;
		VGfloat				_height;
		VGfloat				_width;

        VGfloat     _stroke_miterlimit;
        VGJoinStyle _joinStyle;
        VGCapStyle  _capStyle;

    private:
        MonkSVG::MKSVGHandler* _handler;
        
        typedef vec2<GLfloat> v2_t;
        
        typedef vec3<float> v3_t;
        
        struct textured_vertex_t {
            GLfloat		v[2];
            GLfloat		uv[2];
        };
        
    private:
        
        TESStesselator*		_fillTesseleator;
        std::vector<GLfloat>		_vertices;
        std::vector<v2_t>		_strokeVertices;
        std::list<v3_t>			_tessVertices;
        GLenum				_primType;
        GLuint				_fillVBO;
        GLuint				_strokeVBO;
        int					_numberFillVertices;
        int					_numberStrokeVertices;
        MKPaint*		_fillPaintForPath;
        MKPaint*		_strokePaintForPath;

        
    private:		// tesseleator callbacks
        void endOfTesselation( VGbitfield paintModes );
        
    private:	// utility methods
        
        GLenum primType() {
            return _primType;
        }
        void setPrimType( GLenum t ) {
            _primType = t;
        }
        
        float* tessVerticesBackPtr() {
            return &(_tessVertices.back().x);
        }
        
        void updateBounds(float x, float y) {
            _minX = std::min(_minX, x);
            _width = std::max(_width, x);
            _minY = std::min(_minY, y);
            _height = std::max(_height, y);
        }
        
        void addVertex( float* v ) {
            VGfloat x = (VGfloat)v[0];
            VGfloat y = (VGfloat)v[1];
            updateBounds(x, y);
            _vertices.push_back(x);
            _vertices.push_back(y);
        }
        
        float * addTessVertex( const v3_t & v ) {
            //updateBounds(v.x, v.y);
            _tessVertices.push_back( v );
            return tessVerticesBackPtr();
        }
        
        void buildFill();
        void buildStroke();
        void buildFatLineSegment( std::vector<v2_t>& vertices, const v2_t& p0, const v2_t& p1, const float stroke_width );
        
        // stroke styles
        void applyLineStyles( std::vector<v2_t>& vertices, VGCapStyle style, VGJoinStyle join, VGfloat miter, VGfloat stroke_width );
        size_t numberOfvertices( std::vector<v2_t>& vertices );
        
        int32_t e1;
        int32_t e2;
        int32_t e3;
        
        struct TriangleElement {
            TriangleElement(uint16_t a_, uint16_t b_, uint16_t c_) : a(a_), b(b_), c(c_) {}
            uint16_t a, b, c;
        };
        void addCurrentVertex(const Coordinate& currentVertex, float flip, double distance,
                              const vec2<double>& normal, float endLeft, float endRight, bool round,
                              int32_t startVertex, std::vector<TriangleElement>& triangleStore, std::vector<v2_t> &vertices);
        void addPieSliceVertex(const Coordinate& currentVertex, float flip, double distance,
                               const vec2<double>& extrude, bool lineTurnsLeft, int32_t startVertex,
                               std::vector<TriangleElement>& triangleStore, std::vector<v2_t> &vertices);
        size_t addVertix(std::vector<v2_t> &vertices, int8_t x, int8_t y, float ex, float ey, int8_t tx, int8_t ty, int32_t linesofar);
        
        /*
         * Scale the extrusion vector so that the normal length is this value.
         * Contains the "texture" normals (-1..1). This is distinct from the extrude
         * normals for line joins, because the x-value remains 0 for the texture
         * normal array, while the extrude normal actually moves the vertex to create
         * the acute/bevelled line join.
         */
        static const int8_t extrudeScale = 63;
	};
}
#endif //__mkPath_h__
