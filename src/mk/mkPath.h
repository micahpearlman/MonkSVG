/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */

#ifndef __mkPath_h__
#define __mkPath_h__

#include "mkMath.h"

#include "mkPaint.h"
#include <OpenGLES/ES2/gl.h>
#include "sakaDefs.h"

namespace MonkSVG {
    class MKSVGHandler;
}

namespace Tess {
    template <typename Options, typename Allocators>
    class Tesselator;
    
    template <typename Options>
    struct BaseAllocators;
}

namespace MonkVG {
    class MKPath {
	public:
        using Coord = value_t;
        using PointVec = positionData_t;
        using Vec = vertexData_t;
        using SentinelVec = PointVec;
        
        struct TessOptions;
        using Tesselator = Tess::Tesselator<TessOptions, Tess::BaseAllocators<TessOptions> >;

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
        ,	_fillTesselator( 0 )
        ,   _fcoords(new Saka::vector<float>)
		{
		}
		
        ~MKPath();
        
        
	protected:
		int				_numSegments;	// VG_PATH_NUM_SEGMENTS
		int				_numCoords;		// VG_PATH_NUM_COORDS
		
		// data
		Saka::vector< unsigned char >	_segments;
		Saka::vector< float >	*_fcoords;
		
		// bounds
		float				_minX;
		float				_minY;
		float				_height;
		float				_width;

    private:
        MonkSVG::MKSVGHandler* _handler;
        
    private:
        Tesselator*		_fillTesselator;
        Saka::vector<v2_t>		_strokeVertices;
        int					_numberStrokeVertices;
        MKPaint*		_fillPaintForPath;
        MKPaint*		_strokePaintForPath;

    private:		// tesseleator callbacks
        void endOfTesselation( GLbitfield paintModes );
        
    private:	// utility methods
        
        void buildFill();
        void buildStroke();
        void buildFatLineSegment( Saka::vector<v2_t>& vertices, const v2_t& p0, const v2_t& p1, const float stroke_width );
                
    };
}
#endif //__mkPath_h__
