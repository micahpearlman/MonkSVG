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
#include <Tess2/tesselator.h>
#include <OpenGLES/ES2/gl.h>


namespace MonkSVG {
    class MKSVGHandler;
}

namespace MonkVG {
    class MKPath {
	public:
        void clear( GLbitfield caps );
		
		inline int getNumSegments() const {
			return _numSegments;
		}
		inline void setNumSegments( const int ns ) {
			_numSegments = ns;
		}
		
		inline int getNumCoords() const {
			return _numCoords;
		}
		inline void setNumCoords( const int nc ) {
			_numCoords = nc;
		}
		
        inline float  getMiterlimit ()
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
        
        inline void setMiterlimit (float m)
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
		inline float getMinX() {
			return _minX;
		}
		inline float getMinY() {
			return _minY;
		}
		inline float getWidth() {
			return _width;
		}
		inline float getHeight() {
			return _height;
		}
		
		//// internal data manipulators ////
        bool draw( GLbitfield paintModes );
		void appendData( const int numSegments, const unsigned char * pathSegments, const float * pathData ) ;
		int32_t segmentToNumCoordinates(VGPathSegment segment);
		void copy( const MKPath& src, const Matrix33& transform );
		void buildFillIfDirty();
		

        MKPath( MonkSVG::MKSVGHandler* h )
		:   _handler(h)
		,	_numSegments( 0 )
		,	_numCoords( 0 )
        ,	_minX( std::numeric_limits<float>::max() )
		,	_minY( std::numeric_limits<float>::max() )
		,	_width( -1 )
		,	_height( -1 )
        ,	_fillTesseleator( 0 )
        ,	_strokeVBO(-1)
        ,	_fillVBO(-1)
        ,   _fcoords(new std::vector<float>)
		{
		}
		
        ~MKPath();
        
        
	protected:
		int				_numSegments;	// VG_PATH_NUM_SEGMENTS
		int				_numCoords;		// VG_PATH_NUM_COORDS
		
		// data
		std::vector< unsigned char >	_segments;
		std::vector< float >	*_fcoords;
		
		// bounds
		float				_minX;
		float				_minY;
		float				_height;
		float				_width;

        float     _stroke_miterlimit;
        VGJoinStyle _joinStyle;
        VGCapStyle  _capStyle;

    private:
        MonkSVG::MKSVGHandler* _handler;
        
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
        void endOfTesselation( GLbitfield paintModes );
        
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
            float x = (float)v[0];
            float y = (float)v[1];
            updateBounds(x, y);
            _vertices.push_back(x);
            _vertices.push_back(y);
        }
        
        float * addTessVertex( const v3_t & v ) {
            _tessVertices.push_back( v );
            return tessVerticesBackPtr();
        }
        
        void buildFill();
        void buildStroke();
        void buildFatLineSegment( std::vector<v2_t>& vertices, const v2_t& p0, const v2_t& p1, const float stroke_width );
                
    };
}
#endif //__mkPath_h__
