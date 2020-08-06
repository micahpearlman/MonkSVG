/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */


#include "mkPath.h"
#include "mkSVG.h"
#include "vgCompat.h"
#include <cassert>
#include <vector>
#include "libtess3/Source/tess.h"
#include "CubicToQuad/CubicToQuad.h"


namespace MonkVG {	// Internal Implementation
    struct MKPath::TessOptions : public Tess::DefaultOptions
    {
        TessOptions(MKPath* _dest) : dest(_dest) {}
        
        using Coord = value_t;
        using Vec = vertexData_t;
        using SentinelVec = positionData_t;
        using SweepPlaneVec = positionData_t*;            // One vector, as used internally. Member variables are opaque.
        using Id = ebo_t;
        using AddResult = std::pair<SweepPlaneVec, Id>;

        MonkSVG::MKPath* dest;
        
        static inline Coord getS(const SweepPlaneVec& sVec)
        {
            return sVec->position.x;
        }
        static inline Coord getT(const SweepPlaneVec& sVec)
        {
            return sVec->position.y;
        }
        
        // Adding a point whose origin is a Vector (meaning it's being added from the addVertex outside world)
        AddResult addPoint(const Vec& vec)
        {
            return dest->_handler->addPoint(Vec(vec));
        }
        
        // Adding a point whose origin is a Sweep Plane Vector (meaning it's being added from the Sweep algorithm)
        inline AddResult addPoint(Coord s, Coord t)
        {
            return dest->_handler->addPoint(Vec({s, t}, dest->_fillPaintForPath->getColor()));
        }
        inline AddResult addPoint(const InternalVec& iVec)
        {
            return dest->_handler->addPoint(Vec({iVec.s, iVec.t}, dest->_fillPaintForPath->getColor()));
        }
        
        // Adding a point that will be probably ignored by the end result
        inline SweepPlaneVec addSentinelPoint(Coord s, Coord t)
        {
            return dest->_handler->addSentinelPoint(PointVec({s, t}));
        }

        template <typename Vertex>
        inline void addContour(Id idx, const Vertex*)
        {
            addContourIdx(idx);
        }
        inline void addContourIdx(Id) {}
        
        template <typename Vertex>
        inline void addVertex(Id idx, const Vertex*)
        {
            addVertexIdx(idx);
        }
        inline void addVertexIdx(const Id& idx)
        {
            dest->_handler->addVertexIdx(idx);
        }
        
        using Allocators = Tess::BaseAllocators<MKPath::TessOptions>;
        using AllocatorPool = Tess::AllocatorPool<Allocators>;
        static Tess::AllocatorPool<Allocators>* allocatorPool;
        
        // Retrieves an allocator memory pool that is shared by all the tesselators. Doesn't have any by default.
        template <typename _AllocatorPool>
        constexpr _AllocatorPool* getAllocatorPool() { return allocatorPool; }
    };
    
    Tess::AllocatorPool<MKPath::TessOptions::Allocators>* MKPath::TessOptions::allocatorPool = new AllocatorPool;

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

namespace MonkVG {
    void MKPath::clear( GLbitfield caps ) {
        _segments.clear();
        _numSegments = 0;
        _numCoords = 0;
        
        _fcoords->clear();
    }
    
    void MKPath::buildFillIfDirty() {
        MKPaint* currentFillPaint = _handler->getFillPaint();
        if ( currentFillPaint != _fillPaintForPath ) {
            _fillPaintForPath = (MKPaint*)currentFillPaint;
        }
        // only build the fill if dirty or we are in batch build mode
        buildFill();
    }
    
    bool MKPath::draw( GLbitfield paintModes ) {
        
        if ( paintModes == 0 )
            return false;
        
        // get the native OpenGL context
        if( paintModes & VG_FILL_PATH ) {	// build the fill polygons
            buildFillIfDirty();
        }
        
        buildStroke();
        
        endOfTesselation( paintModes );
        
        return true;
    }
   
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
        
        s = sqrtf(disc);
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
        COS = cosf(rot);
        SIN = sinf(rot);
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

    // Quads and Cubic actually updates the previous vector's Quad. So we must keep it until we're ready to use it
    struct PrevVectorKeeper
    {
        using Tesselator = MKPath::Tesselator;
        using Vec = MKPath::Vec;
        
        Tesselator* fillTesselator;
        bool set = false;
        Vec prevVec;
        
        PrevVectorKeeper(Tesselator* _fillTesselator) :
            fillTesselator(_fillTesselator)
        {}
        
        void add(Vec&& v)
        {
            if (set)
            {
                fillTesselator->addVertex(prevVec);
            }
            prevVec = v;
            set = true;
        }
        void replace(Vec&& v)
        {
            prevVec = v;
            set = true;
        }
        void done(bool startContour = true)
        {
            if (set)
            {
                fillTesselator->addVertex(prevVec);
                set = false;
            }
            if (startContour)
            {
                fillTesselator->beginContour();
            }
        }
    };
    
    void MKPath::buildFill() {
        // reset the bounds
        _minX = std::numeric_limits<float>::max();
        _minY = std::numeric_limits<float>::max();
        _width = -1;
        _height = -1;
        
        TessOptions options(this);
        options.m_windingRule = Tess::TESS_WINDING_POSITIVE;
        if( _handler->getFillRule() == VG_EVEN_ODD ) {
            options.m_windingRule = Tess::TESS_WINDING_ODD;
        } else if( _handler->getFillRule() == VG_NON_ZERO ) {
            options.m_windingRule = Tess::TESS_WINDING_NONZERO;
        }

        _fillTesselator = new Tesselator(options);
        PrevVectorKeeper vec(_fillTesselator);

        std::vector< float >::iterator coordsIter = _fcoords->begin();
        unsigned char segment = VG_CLOSE_PATH;
        v2_t coords({0,0});
        v2_t prev;
        v2_t contourBeginning;
        bool inContour = false;
        
        for ( std::vector< unsigned char >::iterator segmentIter = _segments.begin(); segmentIter != _segments.end(); segmentIter++ ) {
            segment = (*segmentIter);
            
            // todo: deal with relative move
            bool isRelative = segment & VG_RELATIVE;
            prev = coords;
            
            if (!inContour && (segment >> 1) != (VG_CLOSE_PATH >> 1) && (segment >> 1) != (VG_MOVE_TO >> 1))
            {
                vec.done();
                inContour = true;
                contourBeginning = coords;
            }
            switch (segment >> 1) {
                case (VG_CLOSE_PATH >> 1):
                {
                    if ( inContour ) {
                        inContour = false;
                        coords = contourBeginning;
                        vec.add(Vec(affineTransform(_handler->_active_matrix, coords), _fillPaintForPath->getColor()));
                        // Should add contour as closed
                    }
                } break;
                case (VG_MOVE_TO >> 1):
                {
                    if (inContour)
                    {
                        // Should keep contour open
                    }

                    coords.x = *coordsIter; coordsIter++;
                    coords.y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.x += prev.x;
                        coords.y += prev.y;
                    }

                    vec.done();
                    inContour = true;
                    contourBeginning = coords;
                    
                    vec.add(Vec(affineTransform(_handler->_active_matrix, coords), _fillPaintForPath->getColor()));
                    
                } break;
                case (VG_LINE_TO >> 1):
                {
                    coords.x = *coordsIter; coordsIter++;
                    coords.y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.x += prev.x;
                        coords.y += prev.y;
                    }
                    
                    vec.add(Vec(affineTransform(_handler->_active_matrix, coords), _fillPaintForPath->getColor()));
                } break;
                case (VG_HLINE_TO >> 1):
                {
                    coords.x = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.x += prev.x;
                    }
                    
                    vec.add(Vec(affineTransform(_handler->_active_matrix, coords), _fillPaintForPath->getColor()));
                } break;
                case (VG_VLINE_TO >> 1):
                {
                    coords.y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.y += prev.y;
                    }
                    
                    vec.add(Vec(affineTransform(_handler->_active_matrix, coords), _fillPaintForPath->getColor()));
                } break;
                case (VG_SCUBIC_TO >> 1):
                {
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
                    
                    const auto quads(Cubic<float>({{coords.x, coords.y}, {cp1x, cp1y}, {cp2x, cp2y}, {p3x, p3y}}).toQuad());
                    bool first = true;
                    for (const auto& quad : quads)
                    {
                        if (first)
                        {
                            vec.replace(Vec(affineTransform(_handler->_active_matrix, {quad.p.x, quad.p.y}),
                                            affineTransform(_handler->_active_matrix, {quad.q.x, quad.q.y}),
                                            _fillPaintForPath->getColor()));
                            first = false;
                        }
                        else
                        {
                            vec.add(Vec(affineTransform(_handler->_active_matrix, {quad.p.x, quad.p.y}),
                                        affineTransform(_handler->_active_matrix, {quad.q.x, quad.q.y}),
                                        _fillPaintForPath->getColor()));
                        }
                    }
                    coords.x = p3x;
                    coords.y = p3y;
                } break;
                case (VG_CUBIC_TO >> 1):
                {
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

                    const auto quads(Cubic<float>({{coords.x, coords.y}, {cp1x, cp1y}, {cp2x, cp2y}, {p3x, p3y}}).toQuad());
                    bool first = true;
                    for (const auto& quad : quads)
                    {
                        if (first)
                        {
                            vec.replace(Vec(affineTransform(_handler->_active_matrix, {quad.p.x, quad.p.y}),
                                            affineTransform(_handler->_active_matrix, {quad.q.x, quad.q.y}),
                                            _fillPaintForPath->getColor()));
                            first = false;
                        }
                        else
                        {
                            vec.add(Vec(affineTransform(_handler->_active_matrix, {quad.p.x, quad.p.y}),
                                        affineTransform(_handler->_active_matrix, {quad.q.x, quad.q.y}),
                                        _fillPaintForPath->getColor()));
                        }
                    }

                    coords.x = p3x;
                    coords.y = p3y;
                } break;
                    
                case (VG_QUAD_TO >> 1):
                {
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
                    
                    vec.replace(Vec(affineTransform(_handler->_active_matrix, {coords.x, coords.y}),
                                    affineTransform(_handler->_active_matrix, {cpx, cpy}),
                                    _fillPaintForPath->getColor()));
                    vec.add(Vec(affineTransform(_handler->_active_matrix, {px, py}),
                                affineTransform(_handler->_active_matrix, {px, py}),
                                _fillPaintForPath->getColor()));
                    
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
                            v2_t c;
                            
                            float alpha = g * ((float)M_PI / 180.0f);
                            float sinalpha = sinf( alpha );
                            float cosalpha = cosf( alpha );
                            c.x = cx0[0] + (rh * cosalpha * cosbeta - rv * sinalpha * sinbeta);
                            c.y = cx0[1] + (rh * cosalpha * sinbeta + rv * sinalpha * cosbeta);
                            vec.add(Vec(affineTransform(_handler->_active_matrix, c), _fillPaintForPath->getColor()));
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
        vec.done(false);        // All done! Push last point in the contour
        
        SAKA_LOG << "Tesselating" << std::endl;
        
        const int nvp = 3;
        _fillTesselator->tesselate(Tess::TESS_POLYGONS, nvp);

        SAKA_LOG << "Done tesselating" << std::endl;
        
        delete _fillTesselator;
        _fillTesselator = nullptr;
        
        // final calculation of the width and height
        _width = fabsf(_width - _minX);
        _height = fabsf(_height - _minY);
    }
    
    void MKPath::buildFatLineSegment( Saka::vector<v2_t>& vertices, const v2_t& p0, const v2_t& p1, const float stroke_width ) {
        
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
                    
//                    float cp1x = 2.0f * cp2x - p3x;
//                    float cp1y = 2.0f * cp2y - p3y;

                    coords.x = p3x;
                    coords.y = p3y;
                    
                    buildFatLineSegment( _strokeVertices, prev, coords, stroke_width );
/*
                    const auto quads(Cubic<float>({{coords.x, coords.y}, {cp1x, cp1y}, {cp2x, cp2y}, {p3x, p3y}}).toQuad());
                    bool coord = true;
                    bool first = true;
                    for (const auto& quad : quads)
                    {
                        if (first)
                        {
                            first = false;
                            continue;
                        }
                        if (coord)
                        {
                            coords.x = quad.x;
                            coords.y = quad.y;
                            
                            buildFatLineSegment( _strokeVertices, prev, coords, stroke_width );
                        }
                        // TODO: quads are not processed atm. This will be done gpu-side
                        coord = !coord;
                    }
 */
                } break;
                    
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

//                    float cp1x = prev.x + 2 * (cpx - prev.x) / 3;
//                    float cp1y = prev.y + 2 * (cpy - prev.y) / 3;
//                    float cp2x = px + 2 * (cpx - px) / 3;
//                    float cp2y = py + 2 * (cpy - py) / 3;

                    coords.x = px;
                    coords.y = py;
                    
                    // TODO: quads are not processed atm. This will be done gpu-side
                    buildFatLineSegment( _strokeVertices, prev, coords, stroke_width );
                    
                } break;
                    
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
                    
                    coords.x = p3x;
                    coords.y = p3y;
                    
                    buildFatLineSegment( _strokeVertices, prev, coords, stroke_width );
/*
                    const auto quads(Cubic<float>({{coords.x, coords.y}, {cp1x, cp1y}, {cp2x, cp2y}, {p3x, p3y}}).toQuad());
                    bool coord = true;
                    bool first = true;
                    for (const auto& quad : quads)
                    {
                        if (first)
                        {
                            first = false;
                            continue;
                        }
                        if (coord)
                        {
                            coords.x = quad.x;
                            coords.y = quad.y;
                        }
                        // TODO: quads are not processed atm. This will be done gpu-side
                        coord = !coord;
                    } */
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
                            
                            float alpha = g * ((float)M_PI / 180.0f);
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
    }
    
    void MKPath::endOfTesselation( GLbitfield paintModes ) {
        if( (_strokeVertices.size() > 0) ) {
            _handler->addPathVertexData((float*)&_strokeVertices[0], _strokeVertices.size(),
                                       paintModes );
        }
        
        // clear out vertex buffer
        _strokeVertices.clear();
    }
  
    MKPath::~MKPath() {
        _fcoords->clear();
        delete _fcoords;
        _fcoords = nullptr;

        if ( _fillTesselator ) {
            delete _fillTesselator;
            _fillTesselator = nullptr;
        }
    }
    
}
