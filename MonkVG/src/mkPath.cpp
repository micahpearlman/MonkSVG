/*
 *  mkPath.cpp
 *  MonkVG-XCode
 *
 *  Created by Micah Pearlman on 2/22/09.
 *  Copyright 2009 Monk Games. All rights reserved.
 *
 */

#include "mkPath.h"
#include "mkSVG.h"
#include "vgCompat.h"
#include <cassert>

namespace MonkVG {	// Internal Implementation

	int32_t MKPath::segmentToNumCoordinates(VGPathSegment segment)
	{
		static const int32_t coords[13] = {0,2,2,1,1,4,6,2,4,5,5,5,5};
		return coords[(int32_t)segment >> 1];
	}
	
	void MKPath::appendData( const int numSegments, const unsigned char * pathSegments, const float * pathData )
	{
		int numCoords = 0;
		for( int i = 0; i < numSegments; i++ ) {
			_segments.push_back(pathSegments[i] );
			numCoords += segmentToNumCoordinates( static_cast<VGPathSegment>( pathSegments[i] ) );
		}
		
		_numSegments += numSegments;
		_numCoords += numCoords;
		
		for( int i = 0; i < numCoords; i++ ) {
            _fcoords->push_back( pathData[i] );
		}
		
		setIsDirty( true );
	}
	
	void MKPath::copy( const MKPath& src, const Matrix33& transform ) {
		// TODO: transform!!!
		// BUGBUG
		setNumCoords( src.getNumCoords() );
		setNumSegments( src.getNumSegments() );
		_segments = src._segments;
		*_fcoords = *src._fcoords;
	}
	
}

#include "mkBatch.h"
#include <cassert>
#include <vector>

namespace MonkVG {
    
    
    
    void MKPath::clear( GLbitfield caps ) {
        _segments.clear();
        _numSegments = 0;
        _numCoords = 0;
        
        _fcoords->clear();
        _vertices.clear();
        
        // delete vbo buffers
        if ( _strokeVBO != -1 ) {
            glDeleteBuffers( 1, &_strokeVBO );
            _strokeVBO = -1;
        }
        
        if ( _fillVBO != -1 ) {
            glDeleteBuffers( 1, &_fillVBO );
            _fillVBO = -1;
        }
    }
    
    void MKPath::buildFillIfDirty() {
        MKPaint* currentFillPaint = _handler->getFillPaint();
        if ( currentFillPaint != _fillPaintForPath ) {
            _fillPaintForPath = (MKPaint*)currentFillPaint;
            _isFillDirty = true;
        }
        // only build the fill if dirty or we are in batch build mode
        if ( _isFillDirty || _handler->currentBatch() ) {
            buildFill();
        }
        _isFillDirty = false;
    }
    
    void printMat44( float m[4][4] ) {
        printf("--\n");
        for ( int x = 0; x < 4; x++ ) {
            printf("%f\t%f\t%f\t%f\n", m[x][0], m[x][1], m[x][2], m[x][3]);
        }
    }
    
    bool MKPath::draw( GLbitfield paintModes ) {
        
        if ( paintModes == 0 )
            return false;
        
        // get the native OpenGL context
        if( paintModes & VG_FILL_PATH ) {	// build the fill polygons
            buildFillIfDirty();
        }
        
        if( paintModes & VG_STROKE_PATH && (_isStrokeDirty == true || _handler->currentBatch())  ) {
            buildStroke();
            _isStrokeDirty = false;
        }
        
        endOfTesselation( paintModes );
        
        
        if ( _handler->currentBatch() ) {
            return true;		// creating a batch so bail from here
        }

        return true;
    }
    
    static inline float calcCubicBezier1d( float x0, float x1, float x2, float x3, float t ) {
        // see openvg 1.0 spec section 8.3.2 Cubic Bezier Curves
        float oneT = 1.0f - t;
        float x =		x0 * (oneT * oneT * oneT)
        +	3.0f * x1 * (oneT * oneT) * t
        +	3.0f * x2 * oneT * (t * t)
        +	x3 * (t * t * t);
        return x;
    }
    
    static inline float calcQuadBezier1d( float start, float control, float end, float time ) {
        float inverseTime = 1.0f - time;
        return (powf(inverseTime, 2.0f) * start) + (2.0f * inverseTime * time * control) + (powf(time, 2.0f) * end);
    }
    
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
    // Given: Points (x0, y0) and (x1, y1)
    // Return: TRUE if a solution exists, FALSE otherwise
    //	Circle centers are written to (cx0, cy0) and (cx1, cy1)
    static bool findUnitCircles(float x0, float y0, float x1, float y1,
                                     float *cx0, float *cy0, float *cx1, float *cy1) {
        
        // Compute differences and averages
        float dx = x0 - x1;
        float dy = y0 - y1;
        float xm = (x0 + x1)/2;
        float ym = (y0 + y1)/2;
        float dsq, disc, s, sdx, sdy;
        // Solve for intersecting unit circles
        dsq = dx*dx + dy*dy;
        if (dsq == 0.0)
            return false; // Points are coincident
        disc = 1.0f/dsq - 1.0f/4.0f;
        if (disc < 0.0)
            return false; // Points are too far apart
        
        s = sqrt(disc);
        sdx = s*dx;
        sdy = s*dy;
        *cx0 = xm + sdy;
        *cy0 = ym - sdx;
        *cx1 = xm - sdy;
        *cy1 = ym + sdx;
        
        return true;
    }
    
    //Given:
    //Return:
    //Ellipse parameters rh, rv, rot (in degrees), endpoints (x0, y0) and (x1, y1) TRUE if a solution exists, FALSE otherwise. Ellipse centers are written to (cx0, cy0) and (cx1, cy1)
    
    static bool findEllipses(float rh, float rv, float rot,
                                  float x0, float y0, float x1, float y1,
                                  float *cx0, float *cy0, float *cx1, float *cy1) {
        float COS, SIN, x0p, y0p, x1p, y1p, pcx0, pcy0, pcx1, pcy1;
        // Convert rotation angle from degrees to radians
        rot *= M_PI/180.0;
        // Pre-compute rotation matrix entries
        COS = cos(rot);
        SIN = sin(rot);
        // Transform (x0, y0) and (x1, y1) into unit space
        // using (inverse) rotate, followed by (inverse) scale
        x0p = (x0*COS + y0*SIN)/rh;
        y0p = (-x0*SIN + y0*COS)/rv;
        x1p = (x1*COS + y1*SIN)/rh;
        y1p = (-x1*SIN + y1*COS)/rv;
        if (!findUnitCircles(x0p, y0p, x1p, y1p, &pcx0, &pcy0, &pcx1, &pcy1)) {
            return false;
        }
        // Transform back to original coordinate space
        // using (forward) scale followed by (forward) rotate
        pcx0 *= rh;
        pcy0 *= rv;
        pcx1 *= rh;
        pcy1 *= rv;
        *cx0 = pcx0*COS - pcy0*SIN;
        *cy0 = pcx0*SIN + pcy0*COS;
        *cx1 = pcx1*COS - pcy1*SIN;
        *cy1 = pcx1*SIN + pcy1*COS;
        
        return true;
    }
    
    
    void MKPath::buildFill() {
        
        _vertices.clear();
        
        // reset the bounds
        _minX = std::numeric_limits<float>::max();
        _minY = std::numeric_limits<float>::max();
        _width = -1;
        _height = -1;
        
        _fillTesseleator = tessNewTess(NULL);
        
        TessWindingRule winding = TESS_WINDING_POSITIVE;
        if( _handler->getFillRule() == VG_EVEN_ODD ) {
            winding = TESS_WINDING_ODD;
        } else if( _handler->getFillRule() == VG_NON_ZERO ) {
            winding = TESS_WINDING_NONZERO;
        }
        
        std::vector< float >::iterator coordsIter = _fcoords->begin();
        unsigned char segment = VG_CLOSE_PATH;
        v3_t coords(0,0,0);
        v3_t prev(0,0,0);
        int num_contours = 0;
        
        for ( std::vector< unsigned char >::iterator segmentIter = _segments.begin(); segmentIter != _segments.end(); segmentIter++ ) {
            segment = (*segmentIter);
            
            // todo: deal with relative move
            bool isRelative = segment & VG_RELATIVE;
            switch (segment >> 1) {
                case (VG_CLOSE_PATH >> 1):
                {
                    if ( num_contours ) {
                        num_contours--;
                    }
                    
                } break;
                case (VG_MOVE_TO >> 1):
                {
                    if ( num_contours ) {
                        num_contours--;
                    }
                    
                    tessBeginContour( _fillTesseleator );
                    num_contours++;
                    coords.x = *coordsIter; coordsIter++;
                    coords.y = *coordsIter; coordsIter++;
                    
                    addTessVertex( coords );
                    tessAddVertex( _fillTesseleator, coords.x, coords.y, coords.z );
                    
                } break;
                case (VG_LINE_TO >> 1):
                {
                    prev = coords;
                    coords.x = *coordsIter; coordsIter++;
                    coords.y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.x += prev.x;
                        coords.y += prev.y;
                    }
                    
                    addTessVertex( coords );
                    tessAddVertex( _fillTesseleator, coords.x, coords.y, coords.z );
                } break;
                case (VG_HLINE_TO >> 1):
                {
                    prev = coords;
                    coords.x = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.x += prev.x;
                    }
                    
                    addTessVertex( coords );
                    tessAddVertex( _fillTesseleator, coords.x, coords.y, coords.z );
                } break;
                case (VG_VLINE_TO >> 1):
                {
                    prev = coords;
                    coords.y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.y += prev.y;
                    }
                    
                    addTessVertex( coords );
                    tessAddVertex(_fillTesseleator, coords.x, coords.y, coords.z );
                } break;
                case (VG_SCUBIC_TO >> 1):
                {
                    prev = coords;
                    float cp2x = *coordsIter; coordsIter++;
                    float cp2y = *coordsIter; coordsIter++;
                    float p3x = *coordsIter; coordsIter++;
                    float p3y = *coordsIter; coordsIter++;
                    
                    
                    if ( isRelative ) {
                        cp2x += prev.x;
                        cp2y += prev.y;
                        p3x += prev.x;
                        p3y += prev.y;
                    }
                    
                    float cp1x = 2.0f * cp2x - p3x;
                    float cp1y = 2.0f * cp2y - p3y;
                    
                    
                    float increment = 1.0f / _handler->getTessellationIterations();
                    //printf("\tcubic: ");
                    for ( float t = increment; t < 1.0f + increment; t+=increment ) {
                        v3_t c;
                        c.x = calcCubicBezier1d( coords.x, cp1x, cp2x, p3x, t );
                        c.y = calcCubicBezier1d( coords.y, cp1y, cp2y, p3y, t );
                        c.z = 0;
                        addTessVertex( c );
                        tessAddVertex( _fillTesseleator, c.x, c.y, c.z );
                        //	c.print();
                    }
                    //printf("\n");
                    coords.x = p3x;
                    coords.y = p3y;
                    
                }
                    break;
                case (VG_CUBIC_TO >> 1):
                {
                    prev = coords;
                    float cp1x = *coordsIter; coordsIter++;
                    float cp1y = *coordsIter; coordsIter++;
                    float cp2x = *coordsIter; coordsIter++;
                    float cp2y = *coordsIter; coordsIter++;
                    float p3x = *coordsIter; coordsIter++;
                    float p3y = *coordsIter; coordsIter++;
                    
                    if ( isRelative ) {
                        cp1x += prev.x;
                        cp1y += prev.y;
                        cp2x += prev.x;
                        cp2y += prev.y;
                        p3x += prev.x;
                        p3y += prev.y;
                    }
                    
                    float increment = 1.0f / _handler->getTessellationIterations();
                    //printf("\tcubic: ");
                    for ( float t = increment; t < 1.0f + increment; t+=increment ) {
                        v3_t c;
                        c.x = calcCubicBezier1d( coords.x, cp1x, cp2x, p3x, t );
                        c.y = calcCubicBezier1d( coords.y, cp1y, cp2y, p3y, t );
                        c.z = 0;
                        addTessVertex( c );
                        tessAddVertex( _fillTesseleator, c.x, c.y, c.z );
                        //	c.print();
                    }
                    //printf("\n");
                    coords.x = p3x;
                    coords.y = p3y;
                    
                } break;
                    
                case (VG_QUAD_TO >> 1):
                {
                    prev = coords;
                    float cpx = *coordsIter; coordsIter++;
                    float cpy = *coordsIter; coordsIter++;
                    float px = *coordsIter; coordsIter++;
                    float py = *coordsIter; coordsIter++;
                    
                    if ( isRelative ) {
                        cpx += prev.x;
                        cpy += prev.y;
                        px += prev.x;
                        py += prev.y;
                    }
                    
                    float increment = 1.0f / _handler->getTessellationIterations();
                    for ( float t = increment; t < 1.0f + increment; t+=increment ) {
                        v3_t c;
                        c.x = calcQuadBezier1d( coords.x, cpx, px, t );
                        c.y = calcQuadBezier1d( coords.y, cpy, py, t );
                        c.z = 0;
                        addTessVertex( c );
                        tessAddVertex( _fillTesseleator, c.x, c.y, c.z );
                    }
                    
                    coords.x = px;
                    coords.y = py;
                    
                } break;
                    
                case (VG_SCCWARC_TO >> 1):
                case (VG_SCWARC_TO >> 1):
                case (VG_LCCWARC_TO >> 1):
                case (VG_LCWARC_TO >> 1):
                    
                {
                    float rh = *coordsIter; coordsIter++;
                    float rv = *coordsIter; coordsIter++;
                    float rot = *coordsIter; coordsIter++;
                    float cp1x = *coordsIter; coordsIter++;
                    float cp1y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        cp1x += prev.x;
                        cp1y += prev.y;
                    }
                    
                    // convert to Center Parameterization (see OpenVG Spec Apendix A)
                    float cx0[2];
                    float cx1[2];
                    bool success = findEllipses( rh, rv, rot,
                                                     coords.x, coords.y, cp1x, cp1y,
                                                     &cx0[0], &cx0[1], &cx1[0], &cx1[1] );
                    
                    if ( success ) {
                        // see: http://en.wikipedia.org/wiki/Ellipse#Ellipses_in_computer_graphics
                        const int steps = _handler->getTessellationIterations();
                        float beta = 0;	// angle. todo
                        float sinbeta = sinf( beta );
                        float cosbeta = cosf( beta );
                        
                        // calculate the start and end angles
                        v2_t center;
                        center.x = cx0[0];
                        center.y = cx0[1];
                        v2_t norm[2];
                        norm[0].x = center.x - coords.x;
                        norm[0].y = center.y - coords.y;
                        float inverse_len = 1.0f/sqrtf( (norm[0].x * norm[0].x) + (norm[0].y * norm[0].y) );
                        norm[0].x *= inverse_len;
                        norm[0].y *= inverse_len;
                        
                        norm[1].x = center.x - cp1x;
                        norm[1].y = center.y - cp1y;
                        inverse_len = 1.0f/sqrtf( (norm[1].x * norm[1].x) + (norm[1].y * norm[1].y) );
                        norm[1].x *= inverse_len;
                        norm[1].y *= inverse_len;
                        float startAngle = degrees( acosf( -norm[0].x ) );
                        float endAngle = degrees( acosf( -norm[1].x ) );
                        
                        float cross = norm[0].x;
                        
                        if ( cross >= 0  ) {
                            startAngle = 360 - startAngle;
                            endAngle = 360 - endAngle;
                        }
                        if ( startAngle > endAngle ) {
                            float tmp = startAngle;
                            startAngle = endAngle;
                            endAngle = tmp;
                            startAngle = startAngle - 90;
                            endAngle = endAngle - 90;
                        }
                        for ( float g = startAngle; g < endAngle; g+=360/steps ) {
                            v3_t c;
                            
                            float alpha = g * (M_PI / 180.0f);
                            float sinalpha = sinf( alpha );
                            float cosalpha = cosf( alpha );
                            c.x = cx0[0] + (rh * cosalpha * cosbeta - rv * sinalpha * sinbeta);
                            c.y = cx0[1] + (rh * cosalpha * sinbeta + rv * sinalpha * cosbeta);
                            c.z = 0;
                            addTessVertex( c );
                            tessAddVertex( _fillTesseleator, c.x, c.y, c.z );
                        }
                    }
                    
                    coords.x = cp1x;
                    coords.y = cp1y;
                    
                } break;
                    
                default:
                    printf("unkwown command\n");
                    break;
            }
        }	// foreach segment
        
        if ( num_contours ) {
            num_contours--;
        }
        
        assert(num_contours == 0);
        
        const int nvp = 6;
        const int nve = 2;
        int result = tessTesselate(_fillTesseleator, winding, TESS_POLYGONS, nvp, nve, NULL);
        assert(result == 1);
        
        float startVertex_[2];
        float lastVertex_[2];
        float v[2];
        
        const float* verts = tessGetVertices(_fillTesseleator);
        const int* elems = tessGetElements(_fillTesseleator);
        const int nelems = tessGetElementCount(_fillTesseleator);
        
        for (int i = 0; i < nelems; ++i)
        {
            const int* p = &elems[i*nvp];
            for (int j = 0; j < nvp && p[j] != TESS_UNDEF; ++j)
            {
                v[0] = verts[p[j]*2];
                v[1] = verts[p[j]*2+1];
                switch ( j ) {
                    case 0:
                        startVertex_[0] = v[0];
                        startVertex_[1] = v[1];
                        break;
                    case 1:
                        lastVertex_[0] = v[0];
                        lastVertex_[1] = v[1];
                        break;
                        
                    default:
                        addVertex( startVertex_ );
                        addVertex( lastVertex_ );
                        addVertex( v );
                        lastVertex_[0] = v[0];
                        lastVertex_[1] = v[1];
                        break;
                }
            }
        }
        
        tessDeleteTess(_fillTesseleator);
        _fillTesseleator = 0;
        
        // final calculation of the width and height
        _width = fabsf(_width - _minX);
        _height = fabsf(_height - _minY);
    }
    
    void MKPath::buildFatLineSegment( std::vector<v2_t>& vertices, const v2_t& p0, const v2_t& p1, const float stroke_width ) {
        
        if ( p0 == p1 ) {
            return;
        }
        
        float dx = p1.y - p0.y;
        float dy = p0.x - p1.x;
        const float inv_mag = 1.0f / sqrtf(dx*dx + dy*dy);
        dx = dx * inv_mag;
        dy = dy * inv_mag;
        
        v2_t v0, v1, v2, v3;
        const float radius = stroke_width * 0.5f;
        
        v0.x = p0.x + radius * dx;
        v0.y = p0.y + radius * dy;
        vertices.push_back( v0 );
        
        v1.x = p0.x - radius * dx;
        v1.y = p0.y - radius * dy;
        vertices.push_back( v1 );
        
        
        v2.x = p1.x + radius * dx;
        v2.y = p1.y + radius * dy;
        vertices.push_back( v2 );
        
        v3.x = p1.x - radius * dx;
        v3.y = p1.y - radius * dy;
        vertices.push_back( v3 );
        
    }
    
    size_t MKPath::numberOfvertices(std::vector<v2_t>& vertices) {
        size_t l = vertices.size();
        // If the line has duplicate vertices at the end, adjust length to remove them.
        while (l > 2 && vertices[l - 1] == vertices[l - 2]) {
            l--;
        }
        return l;
        
    }
    
    template <typename T>
    inline T perp(const T& a) {
        return T(-a.y, a.x);
    }
    
    template <typename T>
    inline typename T::value_type mag(const T& v) {
        return glm::sqrt(glm::dot(v, v));
    }

    
    void MKPath::applyLineStyles( std::vector<v2_t> &vertices, VGCapStyle style, VGJoinStyle join, float miterLimit, float stroke_width) {
        size_t len = numberOfvertices(vertices);
        
        const v2_t firstVertex = vertices.front();
        const v2_t lastVertex = vertices[len - 1];
        const bool closed = firstVertex == lastVertex;
        
        if (len == 2 && closed)return;
        
        const VGCapStyle beginCap = style;
        const VGCapStyle endCap = closed ? VG_CAP_BUTT : style;
        
        int8_t flip = 1;
        double distance = 0;
        bool startOfLine = true;
        
        v2_t currentVertex, prevVertex, nextVertex;
        v2_t prevNormal, nextNormal;
        
        // the last three vertices added
        e1 = e2 = e3 = -1;
        
        if (closed) {
            currentVertex = vertices[len - 2];
            nextNormal = perp(glm::normalize(firstVertex - currentVertex));
        }
        
        const int32_t startVertex = 0;
        
        std::vector<TriangleElement> triangleStore;
        
        for (size_t i = 0; i < len; ++i) {
            if (closed && i == len - 1) {
                // if the line is closed, we treat the last vertex like the first
                nextVertex = vertices[1];
            } else if (i + 1 < len) {
                // just the next vertex
                nextVertex = vertices[i + 1];
            } else {
                // there is no next vertex
                nextVertex = v2_t();
            }
            
            // if two consecutive vertices exist, skip the current one
            if (nextVertex != v2_t() && vertices[i] == nextVertex) {
                continue;
            }
            
            if (nextNormal != v2_t()) {
                prevNormal = nextNormal;
            }
            if (currentVertex != v2_t()) {
                prevVertex = currentVertex;
            }
            
            currentVertex = vertices[i];
            
            // Calculate how far along the line the currentVertex is
            if (prevVertex != v2_t())
                distance += glm::distance(currentVertex, prevVertex);
            
            // Calculate the normal towards the next vertex in this line. In case
            // there is no next vertex, pretend that the line is continuing straight,
            // meaning that we are just using the previous normal.
            nextNormal = nextVertex != v2_t() ? perp(glm::normalize(nextVertex - currentVertex)) : prevNormal;
            
            // If we still don't have a previous normal, this is the beginning of a
            // non-closed line, so we're doing a straight "join".
            if (prevNormal == v2_t()) {
                prevNormal = nextNormal;
            }
            
            // Determine the normal of the join extrusion. It is the angle bisector
            // of the segments between the previous line and the next line.
            v2_t joinNormal = glm::normalize(prevNormal + nextNormal);
            
            /*  joinNormal     prevNormal
             *             ↖      ↑
             *                .________. prevVertex
             *                |
             * nextNormal  ←  |  currentVertex
             *                |
             *     nextVertex !
             *
             */
            
            // Calculate the length of the miter (the ratio of the miter to the width).
            // Find the cosine of the angle between the next and join normals
            // using dot product. The inverse of that is the miter length.
            const float cosHalfAngle = joinNormal.x * nextNormal.x + joinNormal.y * nextNormal.y;
            const float miterLength = cosHalfAngle != 0 ? 1 / cosHalfAngle: 1;
            
            // The join if a middle vertex, otherwise the cap
            const bool middleVertex = prevVertex != v2_t() && nextVertex != v2_t();
            VGJoinStyle currentJoin = join;
            const VGCapStyle currentCap = nextVertex != v2_t() ? beginCap : endCap;
            
            if (middleVertex) {
                if (currentJoin == VG_JOIN_ROUND) {
                    if (miterLength < miterLimit) {
                        currentJoin = VG_JOIN_MITER;
                    } else if (miterLength <= 2) {
                        currentJoin = (VGJoinStyle)VG_JOIN_FAKE_ROUND;
                    }
                }
                
                if (currentJoin == VG_JOIN_MITER && miterLength > miterLimit) {
                    currentJoin = VG_JOIN_BEVEL;
                }
                
                if (currentJoin == VG_JOIN_BEVEL) {
                    // The maximum extrude length is 128 / 63 = 2 times the width of the line
                    // so if miterLength >= 2 we need to draw a different type of bevel where.
                    if (miterLength > 2) {
                        currentJoin = (VGJoinStyle)VG_JOIN_FLIP_BEVEL;
                    }
                    
                    // If the miterLength is really small and the line bevel wouldn't be visible,
                    // just draw a miter join to save a triangle.
                    if (miterLength < miterLimit) {
                        currentJoin = VG_JOIN_MITER;
                    }
                }
            }
            
            if (middleVertex && currentJoin == VG_JOIN_MITER) {
                joinNormal = joinNormal * miterLength;
                addCurrentVertex(currentVertex, flip, distance, joinNormal, 0, 0, false, startVertex,
                                 triangleStore, vertices);
                
            } else if (middleVertex && currentJoin == VG_JOIN_FLIP_BEVEL) {
                // miter is too big, flip the direction to make a beveled join
                
                if (miterLength > 100) {
                    // Almost parallel lines
                    joinNormal = nextNormal;
                } else {
                    const float direction = prevNormal.x * nextNormal.y - prevNormal.y * nextNormal.x > 0 ? -1 : 1;
                    const float bevelLength = miterLength * mag(prevNormal + nextNormal) / mag(prevNormal - nextNormal);
                    joinNormal = perp(joinNormal) * bevelLength * direction;
                }
                
                addCurrentVertex(currentVertex, flip, distance, joinNormal, 0, 0, false, startVertex,
                                 triangleStore, vertices);
                flip = -flip;
                
            } else if (middleVertex && (currentJoin == VG_JOIN_BEVEL || currentJoin == VG_JOIN_FAKE_ROUND)) {
                const bool lineTurnsLeft = flip * (prevNormal.x * nextNormal.y - prevNormal.y * nextNormal.x) > 0;
                const float offset = -std::sqrt(miterLength * miterLength - 1);
                float offsetA;
                float offsetB;
                
                if (lineTurnsLeft) {
                    offsetB = 0;
                    offsetA = offset;
                } else {
                    offsetA = 0;
                    offsetB = offset;
                }
                
                // Close previous segement with bevel
                if (!startOfLine) {
                    addCurrentVertex(currentVertex, flip, distance, prevNormal, offsetA, offsetB, false,
                                     startVertex, triangleStore, vertices);
                }
                
                if (currentJoin == VG_JOIN_FAKE_ROUND) {
                    // The join angle is sharp enough that a round join would be visible.
                    // Bevel joins fill the gap between segments with a single pie slice triangle.
                    // Create a round join by adding multiple pie slices. The join isn't actually round, but
                    // it looks like it is at the sizes we render lines at.
                    
                    // Add more triangles for sharper angles.
                    // This math is just a good enough approximation. It isn't "correct".
                    const int n = std::floor((0.5 - (cosHalfAngle - 0.5)) * 8);
                    
                    for (int m = 0; m < n; m++) {
                        auto approxFractionalJoinNormal = glm::normalize(nextNormal * ((m + 1.0f) / (n + 1.0f)) + prevNormal);
                        addPieSliceVertex(currentVertex, flip, distance, approxFractionalJoinNormal, lineTurnsLeft, startVertex, triangleStore, vertices);
                    }
                    
                    addPieSliceVertex(currentVertex, flip, distance, joinNormal, lineTurnsLeft, startVertex, triangleStore, vertices);
                    
                    for (int k = n - 1; k >= 0; k--) {
                        auto approxFractionalJoinNormal = glm::normalize(prevNormal * ((k + 1.0f) / (n + 1.0f)) + nextNormal);
                        addPieSliceVertex(currentVertex, flip, distance, approxFractionalJoinNormal, lineTurnsLeft, startVertex, triangleStore, vertices);
                    }
                }
                
                // Start next segment
                if (nextVertex != v2_t()) {
                    addCurrentVertex(currentVertex, flip, distance, nextNormal, -offsetA, -offsetB,
                                     false, startVertex, triangleStore, vertices);
                }
                
            } else if (!middleVertex && currentCap == VG_CAP_BUTT) {
                if (!startOfLine) {
                    // Close previous segment with a butt
                    addCurrentVertex(currentVertex, flip, distance, prevNormal, 0, 0, false,
                                     startVertex, triangleStore, vertices);
                }
                
                // Start next segment with a butt
                if (nextVertex != v2_t()) {
                    addCurrentVertex(currentVertex, flip, distance, nextNormal, 0, 0, false,
                                     startVertex, triangleStore, vertices);
                }
                
            } else if (!middleVertex && currentCap == VG_CAP_SQUARE) {
                if (!startOfLine) {
                    // Close previous segment with a square cap
                    addCurrentVertex(currentVertex, flip, distance, prevNormal, 1, 1, false,
                                     startVertex, triangleStore, vertices);
                    
                    // The segment is done. Unset vertices to disconnect segments.
                    e1 = e2 = -1;
                    flip = 1;
                }
                
                // Start next segment
                if (nextVertex != v2_t()) {
                    addCurrentVertex(currentVertex, flip, distance, nextNormal, -1, -1, false,
                                     startVertex, triangleStore, vertices);
                }
                
            } else if (middleVertex ? currentJoin == VG_JOIN_ROUND : currentCap == VG_CAP_ROUND) {
                if (!startOfLine) {
                    // Close previous segment with a butt
                    addCurrentVertex(currentVertex, flip, distance, prevNormal, 0, 0, false,
                                     startVertex, triangleStore, vertices);
                    
                    // Add round cap or linejoin at end of segment
                    addCurrentVertex(currentVertex, flip, distance, prevNormal, 1, 1, true, startVertex,
                                     triangleStore, vertices);
                    
                    // The segment is done. Unset vertices to disconnect segments.
                    e1 = e2 = -1;
                    flip = 1;
                }
                
                // Start next segment with a butt
                if (nextVertex != v2_t()) {
                    // Add round cap before first segment
                    addCurrentVertex(currentVertex, flip, distance, nextNormal, -1, -1, true,
                                     startVertex, triangleStore, vertices);
                    
                    addCurrentVertex(currentVertex, flip, distance, nextNormal, 0, 0, false,
                                     startVertex, triangleStore, vertices);
                }
            }
            
            startOfLine = false;
        }
        
        
    }
    
    void MKPath::addCurrentVertex(const Coordinate& currentVertex,
                                      float flip,
                                      double distance,
                                      const v2_t& normal,
                                      float endLeft,
                                      float endRight,
                                      bool round,
                                      int32_t startVertex,
                                      std::vector<TriangleElement>& triangleStore, std::vector<v2_t> &vertices) {
        int8_t tx = round ? 1 : 0;
        
        v2_t extrude = normal * flip;
        if (endLeft)
            extrude = extrude - (perp(normal) * endLeft);
        e3 = (int32_t)addVertix(vertices, currentVertex.x, currentVertex.y, extrude.x, extrude.y, tx, 0,
                                distance) - startVertex;
        if (e1 >= 0 && e2 >= 0) {
            triangleStore.emplace_back(e1, e2, e3);
        }
        e1 = e2;
        e2 = e3;
        
        extrude = normal * (-flip);
        if (endRight)
            extrude = extrude - (perp(normal) * endRight);
        e3 = (int32_t)addVertix(vertices, currentVertex.x, currentVertex.y, extrude.x, extrude.y, tx, 1,
                                distance) - startVertex;
        if (e1 >= 0 && e2 >= 0) {
            triangleStore.emplace_back(e1, e2, e3);
        }
        e1 = e2;
        e2 = e3;
    }
    
    void MKPath::addPieSliceVertex(const Coordinate& currentVertex,
                                       float flip,
                                       double distance,
                                       const v2_t& extrude,
                                       bool lineTurnsLeft,
                                       int32_t startVertex,
                                       std::vector<TriangleElement>& triangleStore, std::vector<v2_t> &vertices) {
        int8_t ty = lineTurnsLeft;
        
        auto flippedExtrude = extrude * (flip * (lineTurnsLeft ? -1 : 1));
        e3 = (int32_t)addVertix(vertices, currentVertex.x, currentVertex.y, flippedExtrude.x, flippedExtrude.y, 0, ty,
                                distance) - startVertex;
        if (e1 >= 0 && e2 >= 0) {
            triangleStore.emplace_back(e1, e2, e3);
        }
        
        if (lineTurnsLeft) {
            e2 = e3;
        } else {
            e1 = e3;
        }
        
    }
    
    size_t MKPath::addVertix(std::vector<v2_t> &vertices, int8_t x, int8_t y, float ex, float ey, int8_t tx, int8_t ty, int32_t linesofar) {
        
        intptr_t idx = vertices.size();
        
        int16_t coords[2];
        coords[0] = (x * 2) | tx;
        coords[1] = (y * 2) | ty;
        
        int8_t extrude[4];
        extrude[0] = std::round(extrudeScale * ex);
        extrude[1] = std::round(extrudeScale * ey);
        extrude[2] = static_cast<int8_t>(linesofar / 128);
        extrude[3] = static_cast<int8_t>(linesofar % 128);
        
        return idx;
    }
    
    void MKPath::buildStroke() {
        _strokeVertices.clear();
        
        // get the native OpenGL context
        const float stroke_width = _handler->getStrokeLineWidth();
        
        std::vector< float >::iterator coordsIter = _fcoords->begin();
        unsigned char segment = VG_CLOSE_PATH;
        v2_t coords = v2_t(0,0);
        v2_t prev = v2_t(0,0);
        v2_t closeTo = v2_t(0,0);
        for ( std::vector< unsigned char >::iterator segmentIter = _segments.begin(); segmentIter != _segments.end(); segmentIter++ ) {
            segment = (*segmentIter);
            
            // todo: deal with relative move
            bool isRelative = segment & VG_RELATIVE;
            switch (segment >> 1) {
                case (VG_CLOSE_PATH >> 1):
                {
                    buildFatLineSegment( _strokeVertices, coords, closeTo, stroke_width );
                } break;
                case (VG_MOVE_TO >> 1):
                {
                    prev.x = closeTo.x = coords.x = *coordsIter; coordsIter++;
                    prev.y = closeTo.y = coords.y = *coordsIter; coordsIter++;
                    
                } break;
                case (VG_LINE_TO >> 1):
                {
                    prev = coords;
                    coords.x = *coordsIter; coordsIter++;
                    coords.y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.x += prev.x;
                        coords.y += prev.y;
                    }
                    
                    buildFatLineSegment( _strokeVertices, prev, coords, stroke_width );
                    
                    
                } break;
                case (VG_HLINE_TO >> 1):
                {
                    prev = coords;
                    coords.x = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.x += prev.x;
                    }
                    
                    buildFatLineSegment( _strokeVertices, prev, coords, stroke_width );
                } break;
                case (VG_VLINE_TO >> 1):
                {
                    prev = coords;
                    coords.y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.y += prev.y;
                    }
                    
                    buildFatLineSegment( _strokeVertices, prev, coords, stroke_width );
                    
                } break;
                case (VG_SCUBIC_TO >> 1):
                {
                    prev = coords;
                    float cp2x = *coordsIter; coordsIter++;
                    float cp2y = *coordsIter; coordsIter++;
                    float p3x = *coordsIter; coordsIter++;
                    float p3y = *coordsIter; coordsIter++;
                    
                    
                    if ( isRelative ) {
                        cp2x += prev.x;
                        cp2y += prev.y;
                        p3x += prev.x;
                        p3y += prev.y;
                    }
                    
                    float cp1x = 2.0f * cp2x - p3x;
                    float cp1y = 2.0f * cp2y - p3y;
                    
                    
                    float increment = 1.0f / _handler->getTessellationIterations();
                    //printf("\tcubic: ");
                    for ( float t = increment; t < 1.0f + increment; t+=increment ) {
                        v2_t c;
                        c.x = calcCubicBezier1d( coords.x, cp1x, cp2x, p3x, t );
                        c.y = calcCubicBezier1d( coords.y, cp1y, cp2y, p3y, t );
                        buildFatLineSegment( _strokeVertices, prev, c, stroke_width );
                        prev = c;
                    }
                    coords.x = p3x;
                    coords.y = p3y;
                    
                }
                    break;
                    
                case (VG_QUAD_TO >> 1):     // added by rhcad
                {
                    prev = coords;
                    float cpx = *coordsIter; coordsIter++;
                    float cpy = *coordsIter; coordsIter++;
                    float px = *coordsIter; coordsIter++;
                    float py = *coordsIter; coordsIter++;
                    
                    if ( isRelative ) {
                        cpx += prev.x;
                        cpy += prev.y;
                        px += prev.x;
                        py += prev.y;
                    }
                    
                    float increment = 1.0f / _handler->getTessellationIterations();
                    
                    for ( float t = increment; t < 1.0f + increment; t+=increment ) {
                        v2_t c;
                        c.x = calcQuadBezier1d( coords.x, cpx, px, t );
                        c.y = calcQuadBezier1d( coords.y, cpy, py, t );
                        buildFatLineSegment( _strokeVertices, prev, c, stroke_width );
                        prev = c;
                    }
                    coords.x = px;
                    coords.y = py;
                    
                } break;
                    
                case (VG_CUBIC_TO >> 1):	// todo
                {
                    prev = coords;
                    float cp1x = *coordsIter; coordsIter++;
                    float cp1y = *coordsIter; coordsIter++;
                    float cp2x = *coordsIter; coordsIter++;
                    float cp2y = *coordsIter; coordsIter++;
                    float p3x = *coordsIter; coordsIter++;
                    float p3y = *coordsIter; coordsIter++;
                    
                    if ( isRelative ) {
                        cp1x += prev.x;
                        cp1y += prev.y;
                        cp2x += prev.x;
                        cp2y += prev.y;
                        p3x += prev.x;
                        p3y += prev.y;
                    }
                    
                    
                    float increment = 1.0f / _handler->getTessellationIterations();
                    
                    for ( float t = increment; t < 1.0f + increment; t+=increment ) {
                        v2_t c;
                        c.x = calcCubicBezier1d( coords.x, cp1x, cp2x, p3x, t );
                        c.y = calcCubicBezier1d( coords.y, cp1y, cp2y, p3y, t );
                        buildFatLineSegment( _strokeVertices, prev, c, stroke_width );
                        prev = c;
                    }
                    coords.x = p3x;
                    coords.y = p3y;
                    
                } break;
                case (VG_SCCWARC_TO >> 1):
                case (VG_SCWARC_TO >> 1):
                case (VG_LCCWARC_TO >> 1):
                case (VG_LCWARC_TO >> 1):	
                    
                {
                    float rh = *coordsIter; coordsIter++;
                    float rv = *coordsIter; coordsIter++;
                    float rot = *coordsIter; coordsIter++;
                    float cp1x = *coordsIter; coordsIter++;
                    float cp1y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        cp1x += prev.x;
                        cp1y += prev.y;
                    }
                    
                    
                    // convert to Center Parameterization (see OpenVG Spec Apendix A)
                    float cx0[2];
                    float cx1[2];
                    bool success = findEllipses( rh, rv, rot,
                                                     coords.x, coords.y, cp1x, cp1y,
                                                     &cx0[0], &cx0[1], &cx1[0], &cx1[1] );
                    
                    if ( success ) {
                        // see: http://en.wikipedia.org/wiki/Ellipse#Ellipses_in_computer_graphics 
                        const int steps = _handler->getTessellationIterations();
                        float beta = 0;	// angle. todo
                        float sinbeta = sinf( beta );
                        float cosbeta = cosf( beta );
                        
                        // calculate the start and end angles
                        v2_t center;
                        center.x = cx0[0];//(cx0[0] + cx1[0])*0.5f;
                        center.y = cx0[1];//(cx0[1] + cx1[1])*0.5f;
                        v2_t norm[2];
                        norm[0].x = center.x - coords.x;
                        norm[0].y = center.y - coords.y;
                        float inverse_len = 1.0f/sqrtf( (norm[0].x * norm[0].x) + (norm[0].y * norm[0].y) );
                        norm[0].x *= inverse_len;
                        norm[0].y *= inverse_len;
                        
                        norm[1].x = center.x - cp1x;
                        norm[1].y = center.y - cp1y;
                        inverse_len = 1.0f/sqrtf( (norm[1].x * norm[1].x) + (norm[1].y * norm[1].y) );
                        norm[1].x *= inverse_len;
                        norm[1].y *= inverse_len;
                        float startAngle = degrees( acosf( -norm[0].x ) );
                        float endAngle = degrees( acosf( -norm[1].x ) );
                        float cross = norm[0].x;
                        if ( cross >= 0 ) {
                            startAngle = 360 - startAngle;
                            endAngle = 360 - endAngle;
                        }
                        if ( startAngle > endAngle ) {
                            float tmp = startAngle;
                            startAngle = endAngle;
                            endAngle = tmp;
                            startAngle = startAngle - 90;
                            endAngle = endAngle - 90;
                        }
                        
                        
                        prev = coords;
                        for ( float g = startAngle; g < endAngle + (360/steps); g+=360/steps ) {
                            v2_t c;
                            
                            float alpha = g * (M_PI / 180.0f);
                            float sinalpha = sinf( alpha );
                            float cosalpha = cosf( alpha );
                            c.x = cx0[0] + (rh * cosalpha * cosbeta - rv * sinalpha * sinbeta);
                            c.y = cx0[1] + (rh * cosalpha * sinbeta + rv * sinalpha * cosbeta);
                            //printf( "(%f, %f)\n", c[0], c[1] );
                            buildFatLineSegment( _strokeVertices, prev, c, stroke_width );
                            prev = c;
                        }
                    }
                    
                    coords.x = cp1x;
                    coords.y = cp1y;
                    
                } break;
                    
                default:
                    printf("unkwown command: %d\n", segment >> 1);
                    break;
            }
        }	// foreach segment
        
        if (getMiterlimit()) {
            
            std::cout << "stroke cap " << getCapStyle() << std::endl;
            std::cout << "stroke join " << getJoinStyle() << std::endl;
            std::cout << "stroke miter " << getMiterlimit() << std::endl;
            //work in progress
            //applyLineStyles(_strokeVertices, getCapStyle(), getJoinStyle(), getMiterlimit(), stroke_width);
        }
        
    }
    
    void MKPath::endOfTesselation( GLbitfield paintModes ) {
        MKBatch* glBatch = (MKBatch*)_handler->currentBatch();
        if( glBatch && (_vertices.size() > 0 || _strokeVertices.size() > 0) ) {	// if in batch mode update the current batch
            glBatch->addPathVertexData( &_vertices[0], _vertices.size()/2, 
                                       (float*)&_strokeVertices[0], _strokeVertices.size(), 
                                       paintModes );
            
        }
        
        // clear out vertex buffer
        _vertices.clear();
        _strokeVertices.clear();
    }
  
    MKPath::~MKPath() {
        _fcoords->clear();
        delete _fcoords;
        _fcoords = 0;

        if ( _fillTesseleator ) {
            tessDeleteTess( _fillTesseleator );
            _fillTesseleator = 0;
        }
        
        glDeleteBuffers( 1, &_fillVBO );
    }
    
}
