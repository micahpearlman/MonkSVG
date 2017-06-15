/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */


#include "mkPath.h"
#include "mkSVG.h"
#include "vgCompat.h"
#include <cassert>
#include <vector>

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
        _vertices.clear();
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
    
    template <typename T, typename InnerFloat = float>
    struct Cubic
    {
        struct Point
        {
            T x, y;
            
            constexpr Point operator + (const Point& p) const
            {
                return { x + p.x, y + p.y };
            }
            constexpr Point operator - (const Point& p) const
            {
                return { x - p.x, y - p.y };
            }
            template <typename Oper>
            constexpr Point operator * (Oper f) const
            {
                return { x * f, y * f };
            }
            constexpr InnerFloat operator * (const Point& p) const
            {
                return x*p.x + y*p.y;
            }
            template <typename Oper>
            constexpr Point operator / (Oper f) const
            {
                return { x / f, y / f };
            }
            constexpr InnerFloat square() const
            {
                return x*x + y*y;
            }
            constexpr InnerFloat dist() const
            {
                return sqrtf(square());
            }
            constexpr Point min(const Point& p) const
            {
                return { std::min(x, p.x), std::min(y, p.y) };
            }
            constexpr Point max(const Point& p) const
            {
                return { std::max(x, p.x), std::max(y, p.y) };
            }
        };
        
        // This value has been selected as it barely passes the edge cases testing. These aren't evil tests as such, but will provide an adequate resolution while limiting the number of points to a minimum.
        constexpr static const InnerFloat rangeEpsilon = (InnerFloat)0.0002;
        
        // Every new cubic-to-quad segment will be verified at 4 different locations for adequate distance: 2 at left, 2 at right.
        constexpr static const auto numVerifyPointsPerCubic = 4;
        
        // It's possible to get 3 inflections, and with this value, every inflection can get 4 points, giving a maximum 12 quads, or 25 points total.
        constexpr static const auto maxRecurseSplitLevel = 2;
        
        using PointVec = std::vector<Point>;
        
        Point p1, c1, c2, p2;
        
        
        // The algorithm used for CubicToQuads is geared towards making calculations as fast as possible.
        // In order to make sure the quads are adequately placed, system checks at a few different places whether we are conforming
        // to the curve.
        
        // Returns a vector of points with quads, under the format 1,c,2,c,3,c,...,N
        PointVec toQuad() const
        {
            // Initial split by inflection
            auto splitCubic(getSplitCubicByInflection());
            
            const auto size = (p1.max(c1.max(c2.max(p2))) - p1.min(c1.min(c2.min(p2)))).dist();
            const auto epsilon = rangeEpsilon * splitCubic.size() * size;
            
            // Further subdivisions where required, otherwise copy
            PointVec result;
            result.reserve(splitCubic.size() * 2 * (size_t)pow(2, maxRecurseSplitLevel) + 1);
            for (auto& cubic : splitCubic)
            {
                recurseRange(result, maxRecurseSplitLevel, epsilon, cubic);
            }
            result.push_back(splitCubic.back().p2);
            return result;
        }
        
        
    protected:
        struct CubicQuad;
        
        using CubicList = std::list<CubicQuad>;
        using FloatVec = std::vector<InnerFloat>;
        
        constexpr static bool isValidQuadRange(InnerFloat value)
        {
            return value > rangeEpsilon && value < (InnerFloat)1 - rangeEpsilon;
        }
        static void pushIfValidRange(FloatVec& vec, const float& value)
        {
            if (isValidQuadRange(value))
            {
                vec.push_back(value);
            }
        }
        constexpr static bool isZero(InnerFloat value)
        {
            return value < rangeEpsilon && value > -rangeEpsilon;
        }
        
        static void recurseRange(PointVec& vec, int level, InnerFloat epsilon, CubicQuad& rhs)
        {
            if (level > 0 && rhs.dist > epsilon)
            {
                CubicQuad lhs = rhs.splitAt(0.5);
                recurseRange(vec, level - 1, epsilon * 2, lhs);
                rhs.update();
                recurseRange(vec, level - 1, epsilon * 2, rhs);
            }
            else
            {
                if (level == 0 && rhs.dist > epsilon)
                {
                    //printf("Not enough range @%f of %f\n", rhs.dist, epsilon);
                }
                vec.push_back(rhs.p1);
                vec.push_back(rhs.quadC);
            }
        }
        
        static FloatVec solveQuad(InnerFloat a, InnerFloat b, InnerFloat c)
        {
            // Return to high school and solve (-b+-sqrt(b2-4ac))/2a
            FloatVec pos;
            pos.reserve(3 + 2);     // 3 required + 2 leeway
            if (a == 0)
            {
                if (b != 0)
                {
                    pushIfValidRange(pos, -c/b);
                }
            }
            else
            {
                const auto a2 = a * 2;
                const auto d = b*b - 4*a*c;
                
                if (isZero(d))
                {
                    pushIfValidRange(pos, -b / a2);
                }
                else if (d > 0)
                {
                    const auto dSqrt = sqrtf(d);
                    if (a < 0.f)            // Sort from smallest to biggest
                    {
                        pushIfValidRange(pos, (-b + dSqrt) / a2);
                        pushIfValidRange(pos, (-b - dSqrt) / a2);
                    }
                    else
                    {
                        pushIfValidRange(pos, (-b - dSqrt) / a2);
                        pushIfValidRange(pos, (-b + dSqrt) / a2);
                    }
                }
            }
            return pos;
        }
        
        constexpr static InnerFloat cubicRoot(InnerFloat x)
        {
            return (x < 0) ? -pow(-x, 1./3.) : pow(x, 1./3.);
        }
        
        // This function and getMaxDist are pretty much copied from https://github.com/fontello/cubic2quad
        static FloatVec solveCubic(InnerFloat a, InnerFloat b, InnerFloat c, InnerFloat d)
        {
            // Return to college and solve our closest pal to a*x^3 + b*x^2 + c*x + d = 0
            if (isZero(a))
            {
                return solveQuad(b, c, d);
            }
            
            FloatVec pos;
            pos.reserve(3 + 2);     // 3 required + 2 leeway for 0 and 1
            
            const auto xn = -b / (3*a); // point of symmetry x coordinate
            const auto yn = ((a * xn + b) * xn + c) * xn + d; // point of symmetry y coordinate
            const auto deltaSq = (b*b - 3*a*c) / (9*a*a); // delta^2
            const auto hSq = 4*a*a * pow(deltaSq, 3); // h^2
            const auto D3 = yn*yn - hSq;
            
            if (isZero(D3))
            { // 2 real roots
                const auto delta1 = cubicRoot(yn/(2*a));
                pushIfValidRange(pos, xn - 2 * delta1);
                pushIfValidRange(pos, xn + delta1);
            }
            else if (D3 > 0)
            { // 1 real root
                const auto D3Sqrt = sqrt(D3);
                pushIfValidRange(pos, xn + cubicRoot((-yn + D3Sqrt)/(2*a)) + cubicRoot((-yn - D3Sqrt)/(2*a)));
            }
            else
            { // 3 real roots
                const auto theta = acos(-yn / sqrt(hSq)) / 3;
                const auto delta = sqrt(deltaSq);
                pushIfValidRange(pos, xn + 2 * delta * cos(theta));
                pushIfValidRange(pos, xn + 2 * delta * cos(theta + M_PI * 2 / 3));
                pushIfValidRange(pos, xn + 2 * delta * cos(theta + M_PI * 4 / 3));
            }
            
            return pos;
        }
        
        struct PowerCoefficients
        {
            Point a, b, c, d;
            
            constexpr Point solve(InnerFloat t) const
            {
                // Solve a*t^3 + b*t^2 + c*t + d
                return ((a*t + b)*t + c)*t + d;
            }
            constexpr Point derivative(InnerFloat t) const
            {
                // Solve d/dt(solve(t))
                return (a*t*3 + b*2)*t + c;
            }
        };
        constexpr PowerCoefficients toPowerCoefficients() const
        {
            return
            {
                p2 - p1 + (c1 - c2) * 3,
                (p1 + c2) * 3 - c1 * 6,
                (c1 - p1) * 3,
                p1
            };
        }
        
        // Based on http://www.caffeineowl.com/graphics/2d/vectorial/cubic-inflexion.html.
        // Goal is to solve (-b+-sqrt(b2-4ac))/2a
        constexpr InnerFloat getA() const
        {
            return
            p1.x * (                c1.y - 2 * c2.y +     p2.y)
            - c1.x * (     p1.y            - 3 * c2.y + 2 * p2.y)
            + c2.x * ( 2 * p1.y - 3 * c1.y            +     p2.y)
            - p2.y * (     p1.y - 2 * c1.y +     c2.y           );
        }
        constexpr InnerFloat getB() const
        {
            return
            - p1.x * (            2 * c1.y - 3 * c2.y +     p2.y)
            + c1.x * ( 2 * p1.y            - 3 * c2.y +     p2.y)
            - c2.x * ( 3 * p1.y - 3 * c1.y                      )
            + p2.x * (     p1.y -     c1.y                      );
        }
        constexpr InnerFloat getC() const
        {
            return
            p1.x * (                c1.y -     c2.y           )
            + c1.x * (-    p1.y            +     c2.y           )
            + c2.x * (     p1.y -     c1.y                      );
        }
        
        // This is a glorified cubic with precalculated single-quad data
        struct CubicQuad : public Cubic<T, InnerFloat>
        {
            using Cubic = Cubic<T, InnerFloat>;
            
            PowerCoefficients coef;
            Point f1;
            Point f2;
            Point f1d;
            Point f2d;
            InnerFloat d;
            
            bool isStraightLine;
            Point quadC;
            float dist;
            
            CubicQuad(const Cubic& c):
            Cubic(c)
            {
                update();
            }
            
            CubicQuad(const Cubic&& c):
            Cubic(c)
            {
                update();
            }
            
            CubicQuad(const Point& p1_, const Point& c1_, const Point& c2_, const Point& p2_):
            Cubic({p1_, c1_, c2_, p2_})
            {
                update();
            }
            
            void update()
            {
                coef = toPowerCoefficients();
                f1 = coef.solve(0);
                f2 = coef.solve(1);
                f1d = coef.derivative(0);
                f2d = coef.derivative(1);
                d = segmentDenominator();
                isStraightLine = isZero(d);
                quadC = getQuadC();
                dist = getMaxDist();
            }
            
        protected:
            constexpr InnerFloat segmentDenominator() const
            {
                return f2d.x*f1d.y - f1d.x*f2d.y;
            }
            
            constexpr Point straightLineSegment() const
            {
                return (f1 + f2) / 2.f;
            }
            
        private:
            constexpr InnerFloat curvedLineSegmentE1() const
            {
                return f2.y*f2d.x - f2.x*f2d.y;
            }
            constexpr InnerFloat curvedLineSegmentE2() const
            {
                return f1.x*f1d.y - f1.y*f1d.x;
            }
        protected:
            constexpr Point curvedLineSegment() const
            {
                return Point({
                    f1d.x*(curvedLineSegmentE1()) + f2d.x*(curvedLineSegmentE2()),
                    f1d.y*(curvedLineSegmentE1()) + f2d.y*(curvedLineSegmentE2())
                }) / d;
            }
            
            constexpr Point getQuadC() const
            {
                return isStraightLine ? straightLineSegment() : curvedLineSegment();
            }
            
            // This function and solveCubic are pretty much copied from https://github.com/fontello/cubic2quad
            InnerFloat getMaxDist() const
            {
                InnerFloat maxDist = 0;
                
                constexpr const InnerFloat dt(1.f/(numVerifyPointsPerCubic + 1.f));
                InnerFloat t = dt;
                for (int i=0; i < numVerifyPointsPerCubic; ++i)
                {
                    const auto cubicpt = coef.solve(t);
                    t += dt;
                    
                    const auto a = p1 + p2 - quadC * 2;
                    const auto b = (quadC - p1) * 2;
                    const auto c = p1;
                    const auto cSubPt = c - cubicpt;
                    const auto e3 = a.square() * 2;
                    const auto e2 = a * b * 3;
                    const auto e1 = b.square() + a * cSubPt * 2;
                    const auto e0 = cSubPt * b;
                    
                    auto candidates = solveCubic(e3, e2, e1, e0);
                    candidates.push_back(0);
                    candidates.push_back(1);
                    
                    InnerFloat minDist = MAXFLOAT;
                    for (auto candidateT : candidates)
                    {
                        const auto distance = ((a*candidateT + b)*candidateT + c - cubicpt).dist();
                        if (distance < minDist)
                        {
                            minDist = distance;
                            if (distance == 0) break;
                        }
                    }
                    
                    if (minDist > maxDist)
                    {
                        maxDist = minDist;
                    }
                }
                return maxDist;
            }
        };
        
        Cubic splitAt(float pos)
        {
            Cubic leftSide;
            
            const auto u = pos;
            const auto v = 1-u;
            
            leftSide.p1   = p1;
            leftSide.c1   = p1 * u          + c1 * v;
            const Point s = c1 * u          + c2 * v;
            leftSide.c2   = leftSide.c1 * u + s  * v;
            
            c2            = c2 * u          + p2 * v;
            c1            = s * u           + c2 * v;
            p1            = leftSide.c2 * u + c1 * v;
            leftSide.p2   = p1;
            
            return leftSide;
        }
        
        CubicList getSplitCubicByInflection() const
        {
            const auto a = getA();
            const auto b = getB();
            const auto c = getC();
            const auto inflectionPosVec = solveQuad(a, b, c);
            
            CubicList result;
            
            if (inflectionPosVec.empty())
            {
                result.push_back(std::move(CubicQuad(*this)));
                return result;
            }
            
            // Split at inflectionPos https://math.stackexchange.com/questions/877725
            Cubic rightSide(*this);
            float prevPoint = 0;
            for (auto inflectionPos : inflectionPosVec)
            {
                float pos = (1 - inflectionPos) / (1 - prevPoint);
                result.push_back(std::move(CubicQuad(rightSide.splitAt(pos))));
                prevPoint = inflectionPos;
            }
            result.push_back(std::move(CubicQuad(std::move(rightSide))));
            
            return result;
        }
    };
    
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
    
    
    void MKPath::buildFill() {
        
        _vertices.clear();
        
        // reset the bounds
        _minX = std::numeric_limits<float>::max();
        _minY = std::numeric_limits<float>::max();
        _width = -1;
        _height = -1;
        
        _fillTesselator = new Tess::Tesselator;
        
        Tess::WindingRule winding = Tess::TESS_WINDING_POSITIVE;
        if( _handler->getFillRule() == VG_EVEN_ODD ) {
            winding = Tess::TESS_WINDING_ODD;
        } else if( _handler->getFillRule() == VG_NON_ZERO ) {
            winding = Tess::TESS_WINDING_NONZERO;
        }
        
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
                _fillTesselator->beginContour();
                inContour = true;
                contourBeginning = coords;
            }
            switch (segment >> 1) {
                case (VG_CLOSE_PATH >> 1):
                {
                    if ( inContour ) {
                        inContour = false;
                        coords = contourBeginning;
                        _fillTesselator->addVertex(coords.x, coords.y);
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

                    _fillTesselator->beginContour();
                    inContour = true;
                    contourBeginning = coords;
                    
                    _fillTesselator->addVertex( coords.x, coords.y );
                    
                } break;
                case (VG_LINE_TO >> 1):
                {
                    coords.x = *coordsIter; coordsIter++;
                    coords.y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.x += prev.x;
                        coords.y += prev.y;
                    }
                    
                    _fillTesselator->addVertex( coords.x, coords.y );
                } break;
                case (VG_HLINE_TO >> 1):
                {
                    coords.x = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.x += prev.x;
                    }
                    
                    _fillTesselator->addVertex( coords.x, coords.y );
                } break;
                case (VG_VLINE_TO >> 1):
                {
                    coords.y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        coords.y += prev.y;
                    }
                    
                    _fillTesselator->addVertex( coords.x, coords.y );
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
                            v2_t c;
                            c.x = quad.x;
                            c.y = quad.y;

                            _fillTesselator->addVertex( c.x, c.y );
                        }
                        // TODO: quads are not processed atm. This will be done gpu-side
                        coord = !coord;
                    }

                    coords.x = p3x;
                    coords.y = p3y;
                }
                    break;
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
                            v2_t c;
                            c.x = quad.x;
                            c.y = quad.y;
                            
                            _fillTesselator->addVertex( c.x, c.y );
                        }
                        // TODO: quads are not processed atm. This will be done gpu-side
                        coord = !coord;
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
                    
                    v2_t c;
                    c.x = px;
                    c.y = py;
                    // TODO: quads are not processed atm. This will be done gpu-side
                    _fillTesselator->addVertex( c.x, c.y );
                    
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
                        // TODO: Convert ellipses to quads and process in gpu
                        for ( float g = startAngle; g < endAngle; g+=360/steps ) {
                            v2_t c;
                            
                            float alpha = g * ((float)M_PI / 180.0f);
                            float sinalpha = sinf( alpha );
                            float cosalpha = cosf( alpha );
                            c.x = cx0[0] + (rh * cosalpha * cosbeta - rv * sinalpha * sinbeta);
                            c.y = cx0[1] + (rh * cosalpha * sinbeta + rv * sinalpha * cosbeta);
                            _fillTesselator->addVertex( c.x, c.y );
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
        
        const int nvp = 3;
        _fillTesselator->tesselate(winding, Tess::TESS_POLYGONS, nvp, NULL);
        
        float v[6];
        
        const float* verts = _fillTesselator->getVertices();
        const int* elems = _fillTesselator->getElements();
        const int nelems = _fillTesselator->getElementCount();
        
        for (int i = 0; i < nelems; ++i)
        {
            const int* p = &elems[i*nvp];
            v[0] = verts[p[0]*2];
            v[1] = verts[p[0]*2+1];
            v[2] = verts[p[1]*2];
            v[3] = verts[p[1]*2+1];
            v[4] = verts[p[2]*2];
            v[5] = verts[p[2]*2+1];
            addVertex( &v[0] );
            addVertex( &v[2] );
            addVertex( &v[4] );
        }
        
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
                    
                    float cp1x = 2.0f * cp2x - p3x;
                    float cp1y = 2.0f * cp2y - p3y;
                    
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
                    }
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
        if( (_vertices.size() > 0 || _strokeVertices.size() > 0) ) {
            _handler->addPathVertexData( &_vertices[0], _vertices.size()/2,
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
        _fcoords = nullptr;

        if ( _fillTesselator ) {
            delete _fillTesselator;
            _fillTesselator = nullptr;
        }
    }
    
}
