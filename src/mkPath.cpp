/*
 *  mkPath.cpp
 *  MonkVG-XCode
 *
 *  Created by Micah Pearlman on 2/22/09.
 *  Copyright 2009 Monk Games. All rights reserved.
 *
 */

#include "mkPath.h"
#include "mkContext.h"
#include <cassert>

namespace MonkVG {	// Internal Implementation

	int32_t IPath::segmentToNumCoordinates(VGPathSegment segment)
	{
		static const int32_t coords[13] = {0,2,2,1,1,4,6,2,4,5,5,5,5};
		return coords[(int32_t)segment >> 1];
	}
	
	void IPath::appendData( const VGint numSegments, const VGubyte * pathSegments, const void * pathData ) 
	{
		int numCoords = 0;
		for( int i = 0; i < numSegments; i++ ) {
			_segments.push_back(pathSegments[i] );
			numCoords += segmentToNumCoordinates( static_cast<VGPathSegment>( pathSegments[i] ) );
		}
		
		_numSegments += numSegments;
		_numCoords += numCoords;
		
		for( int i = 0; i < numCoords; i++ ) {
			switch (_datatype) {
				case VG_PATH_DATATYPE_F:
					_fcoords->push_back( *(((VGfloat*)(pathData)) + i) );
					break;
				default:
					// error
					assert( !"unsupported path data type" );
					break;
			}
		}
		
		setIsDirty( true );
	}
	
	void IPath::copy( const IPath& src, const Matrix33& transform ) {
		// TODO: transform!!!
		// BUGBUG
		setNumCoords( src.getNumCoords() );
		setNumSegments( src.getNumSegments() );
		_segments = src._segments;
		*_fcoords = *src._fcoords;
	}
	
	VGint IPath::getParameteri( const VGint p ) const {
		switch (p) {
			case VG_PATH_FORMAT:
				return getFormat();
			case VG_PATH_DATATYPE:
				return getDataType();
			case VG_PATH_NUM_SEGMENTS:
				return getNumSegments();
			case VG_PATH_NUM_COORDS:
				return getNumCoords();
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				return -1;	
				break;
		}
	}
	
	VGfloat IPath::getParameterf( const VGint p ) const {
		switch (p) {
			case VG_PATH_SCALE:
				return getScale();
				break;
			case VG_PATH_BIAS:
				return getBias();
				break;
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				return -1;	
				break;
		}
	}
	
	void IPath::getParameterfv( const VGint p, VGfloat *fv ) const {
		SetError( VG_ILLEGAL_ARGUMENT_ERROR );
	}

	
	void IPath::setParameter( const VGint p, const VGint v ) {
		switch (p) {
			case VG_PATH_FORMAT:
				setFormat( v );
				break;
			case VG_PATH_DATATYPE:
				setDataType( static_cast<VGPathDatatype>( v ) );
				break;
			case VG_PATH_NUM_SEGMENTS:
				setNumSegments( v );
				break;
			case VG_PATH_NUM_COORDS:
				setNumCoords( v );
				break;
			default:
				break;
		}
	}
	
	void IPath::setParameter( const VGint p, const VGfloat v ) 
	{
		switch (p) {
			case VG_PATH_SCALE:
				setScale( v );
				break;
			case VG_PATH_BIAS:
				setBias( v );
				break;
			default:
				break;
		}
	}
	
	void IPath::setParameter( const VGint p, const VGfloat* fv, const VGint cnt ) {
		SetError( VG_ILLEGAL_ARGUMENT_ERROR );
	}
	
	
}


///// OpenVG API Implementation /////

using namespace MonkVG;

VG_API_CALL VGPath vgCreatePath( VGint pathFormat, VGPathDatatype datatype, VGfloat scale, VGfloat bias, VGint segmentCapacityHint, VGint coordCapacityHint, VGbitfield capabilities ) {
	IPath* path = IContext::instance().createPath( pathFormat, datatype, scale, bias, segmentCapacityHint, coordCapacityHint, capabilities  &= VG_PATH_CAPABILITY_ALL );
	
	return (VGPath)path;
}

VG_API_CALL void VG_API_ENTRY vgDestroyPath(VGPath path) VG_API_EXIT {
	if( path ) {
		IContext::instance().destroyPath( (IPath*)path );
		path = VG_INVALID_HANDLE;
	}
}


VG_API_CALL void vgAppendPathData( VGPath dstPath, VGint numSegments, const VGubyte * pathSegments, const void * pathData ) {
	IPath* path = (IPath*)dstPath;
	path->appendData( numSegments, pathSegments, pathData );
}

VG_API_CALL void vgDrawPath(VGPath path, VGbitfield paintModes) {
	if ( path == VG_INVALID_HANDLE ) {
		SetError( VG_BAD_HANDLE_ERROR );
		return;
	}
	
	// force image matrix mode
	if( IContext::instance().getMatrixMode() != VG_MATRIX_PATH_USER_TO_SURFACE ) {
		IContext::instance().setMatrixMode( VG_MATRIX_PATH_USER_TO_SURFACE );
	}

	IPath* p = (IPath*)path;
	
	
	p->draw( paintModes );
}

VG_API_CALL void VG_API_ENTRY vgPathMiterLimit(VGPath path, VGfloat miterlimit) VG_API_EXIT {
    if ( path == VG_INVALID_HANDLE ) {
        SetError( VG_BAD_HANDLE_ERROR );
        return;
    }
    
    IPath* p = (IPath*)path;
    
    p->setMiterlimit( miterlimit );
}

VG_API_CALL void VG_API_ENTRY vgPathJoinStyle(VGPath path, VGJoinStyle joinstyle) VG_API_EXIT {
    if ( path == VG_INVALID_HANDLE ) {
        SetError( VG_BAD_HANDLE_ERROR );
        return;
    }
    
    IPath* p = (IPath*)path;
    
    p->setJoinStyle( joinstyle );
}

VG_API_CALL void VG_API_ENTRY vgPathCapStyle(VGPath path, VGCapStyle capstyle) VG_API_EXIT {
    if ( path == VG_INVALID_HANDLE ) {
        SetError( VG_BAD_HANDLE_ERROR );
        return;
    }
    
    IPath* p = (IPath*)path;
    
    p->setCapStyle( capstyle );
}


VG_API_CALL void VG_API_ENTRY vgClearPath(VGPath path, VGbitfield capabilities) VG_API_EXIT {
	if ( path == VG_INVALID_HANDLE ) {
		SetError( VG_BAD_HANDLE_ERROR );
		return;
	}
	
	IPath* p = (IPath*)path;
	p->clear( capabilities );
	
}

VG_API_CALL void VG_API_ENTRY vgTransformPath(VGPath dstPath, VGPath srcPath) VG_API_EXIT {
	IPath* dp = (IPath*)dstPath;
	dp->copy( *(IPath*)srcPath, IContext::instance().getPathUserToSurface() );
}

VG_API_CALL void VG_API_ENTRY vgPathBounds(VGPath path,
										   VGfloat * minX, VGfloat * minY,
										   VGfloat * width, VGfloat * height) VG_API_EXIT {
	
	IPath* p = (IPath*)path;
	p->buildFillIfDirty();	// NOTE: according to the OpenVG specs we only care about the fill bounds, NOT the fill + stroke
	*minX = p->getMinX();
	*minY = p->getMinY();
	*width = p->getWidth();
	*height = p->getHeight();
	
}

VG_API_CALL void VG_API_ENTRY vgPathTransformedBounds(VGPath path,
						      VGfloat * minX, VGfloat * minY,
						      VGfloat * width, VGfloat * height) VG_API_EXIT {
	IPath* p = (IPath*)path;
	p->buildFillIfDirty();	// NOTE: according to the OpenVG specs we only care about the fill bounds, NOT the fill + stroke
	float x = p->getMinX();
	float y = p->getMinX();
	float w = p->getWidth();
	float h = p->getHeight();

	float p0[2];
	p0[0] = x;
	p0[1] = y;
	float p1[2];
	p1[0] = x + w;
	p1[1] = y;
	float p2[2];
	p2[0] = x + w;
	p2[1] = y + h;
	float p3[2];
	p3[0] = x;
	p3[1] = y + h;

	const Matrix33 & m = IContext::instance().getPathUserToSurface();

	affineTransform(m, p0);
	affineTransform(m, p1);
	affineTransform(m, p2);
	affineTransform(m, p3);

	*minX = std::min(std::min(std::min(p0[0], p1[0]), p2[0]), p3[0]);
	*width = std::max(std::max(std::max(p0[0], p1[0]), p2[0]), p3[0]) - *minX;
	*minY = std::min(std::min(std::min(p0[1], p1[1]), p2[1]), p3[1]);
	*height = std::max(std::max(std::max(p0[1], p1[1]), p2[1]), p3[1]) - *minY;
}


#include "mkBatch.h"
#include <cassert>
#include "vec.hpp"
#include <vector>

namespace MonkVG {
    
    
    
    void IPath::clear( VGbitfield caps ) {
        _segments.clear();
        _numSegments = 0;
        _numCoords = 0;
        
        switch (_datatype) {
            case VG_PATH_DATATYPE_F:
                _fcoords->clear();
                break;
            default:
                // error
                assert( !"unsupported path data type" );
                break;
        }
       
        _vertices.clear();
        
        // delete vbo buffers
        if ( _strokeVBO != -1 ) {
            GL->glDeleteBuffers( 1, &_strokeVBO );
            _strokeVBO = -1;
        }
        
        if ( _fillVBO != -1 ) {
            GL->glDeleteBuffers( 1, &_fillVBO );
            _fillVBO = -1;
        }
    }
    
    void IPath::buildFillIfDirty() {
        IPaint* currentFillPaint = IContext::instance().getFillPaint();
        if ( currentFillPaint != _fillPaintForPath ) {
            _fillPaintForPath = (IPaint*)currentFillPaint;
            _isFillDirty = true;
        }
        // only build the fill if dirty or we are in batch build mode
        if ( _isFillDirty || IContext::instance().currentBatch() ) {
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
    
    bool IPath::draw( VGbitfield paintModes ) {
        
        if ( paintModes == 0 )
            return false;
        
        // get the native OpenGL context
        IContext& glContext = (MonkVG::IContext&)IContext::instance();
        
        if( paintModes & VG_FILL_PATH ) {	// build the fill polygons
            buildFillIfDirty();
        }
        
        if( paintModes & VG_STROKE_PATH && (_isStrokeDirty == true || IContext::instance().currentBatch())  ) {
            buildStroke();
            _isStrokeDirty = false;
        }
        
        endOfTesselation( paintModes );
        
        
        if ( glContext.currentBatch() ) {
            return true;		// creating a batch so bail from here
        }
        
        
        glContext.beginRender();
        
        GL->glEnableClientState( GL_VERTEX_ARRAY );
        GL->glDisableClientState( GL_COLOR_ARRAY );
        VGImageMode oldImageMode = glContext.getImageMode();
        
        // configure based on paint type
        if ( _fillPaintForPath && _fillPaintForPath->getPaintType() == VG_PAINT_TYPE_COLOR ) {
            GL->glDisable(GL_TEXTURE_2D);
            GL->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
        } else if ( _fillPaintForPath && (_fillPaintForPath->getPaintType() == VG_PAINT_TYPE_LINEAR_GRADIENT || _fillPaintForPath->getPaintType() == VG_PAINT_TYPE_RADIAL_GRADIENT /* || _fillPaintForPath->getPaintType() == VG_PAINT_TYPE_RADIAL_2x3_GRADIENT || _fillPaintForPath->getPaintType() == VG_PAINT_TYPE_LINEAR_2x3_GRADIENT */) ) {
            GL->glEnable( GL_TEXTURE_2D );
            GL->glEnableClientState( GL_TEXTURE_COORD_ARRAY );
            //GL->glColor4f(1, 1, 1, 1);  // HACKHACK: need to fix when drawing texture with GL_REPLACE we don't use the current glColor
            
            glContext.setImageMode( VG_DRAW_IMAGE_NORMAL );
            
        }
        
        
        if( (paintModes & VG_FILL_PATH) && _fillVBO != -1 && _fillPaintForPath) {
            // draw
            IContext::instance().fill();
            GL->glBindBuffer( GL_ARRAY_BUFFER, _fillVBO );
            if ( _fillPaintForPath->getPaintType() == VG_PAINT_TYPE_COLOR ) {
                GL->glVertexPointer( 2, GL_FLOAT, sizeof(v2_t), 0 );
            }
            GL->glDrawArrays( GL_TRIANGLES, 0, _numberFillVertices );
            
            // unbind any textures being used
            if ( (_fillPaintForPath->getPaintType() == VG_PAINT_TYPE_LINEAR_GRADIENT || _fillPaintForPath->getPaintType() == VG_PAINT_TYPE_RADIAL_GRADIENT /* || _fillPaintForPath->getPaintType() == VG_PAINT_TYPE_RADIAL_2x3_GRADIENT || _fillPaintForPath->getPaintType() == VG_PAINT_TYPE_LINEAR_2x3_GRADIENT */) ) {
                glContext.setImageMode( oldImageMode );
                
                GL->glDisable(GL_TEXTURE_2D);
                GL->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
            }
            
            // this is important to unbind the vbo when done
            GL->glBindBuffer( GL_ARRAY_BUFFER, 0 );
        }
        
        if ( (paintModes & VG_STROKE_PATH) && _strokeVBO != -1 ) {
            // draw
            IContext::instance().stroke();
            GL->glBindBuffer( GL_ARRAY_BUFFER, _strokeVBO );
            GL->glVertexPointer( 2, GL_FLOAT, sizeof(v2_t), 0 );
            GL->glDrawArrays( GL_TRIANGLE_STRIP, 0, _numberStrokeVertices );
            GL->glBindBuffer( GL_ARRAY_BUFFER, 0 );
        }
        
        glContext.endRender();
        
        return true;
    }
    
    static inline VGfloat calcCubicBezier1d( VGfloat x0, VGfloat x1, VGfloat x2, VGfloat x3, VGfloat t ) {
        // see openvg 1.0 spec section 8.3.2 Cubic Bezier Curves
        VGfloat oneT = 1.0f - t;
        VGfloat x =		x0 * (oneT * oneT * oneT)
        +	3.0f * x1 * (oneT * oneT) * t
        +	3.0f * x2 * oneT * (t * t)
        +	x3 * (t * t * t);
        return x;
    }
    
    static inline VGfloat calcQuadBezier1d( VGfloat start, VGfloat control, VGfloat end, VGfloat time ) {
        float inverseTime = 1.0f - time;
        return (powf(inverseTime, 2.0f) * start) + (2.0f * inverseTime * time * control) + (powf(time, 2.0f) * end);
    }
    
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
    // Given: Points (x0, y0) and (x1, y1)
    // Return: TRUE if a solution exists, FALSE otherwise
    //	Circle centers are written to (cx0, cy0) and (cx1, cy1)
    static VGboolean findUnitCircles(VGfloat x0, VGfloat y0, VGfloat x1, VGfloat y1,
                                     VGfloat *cx0, VGfloat *cy0, VGfloat *cx1, VGfloat *cy1) {
        
        // Compute differences and averages
        VGfloat dx = x0 - x1;
        VGfloat dy = y0 - y1;
        VGfloat xm = (x0 + x1)/2;
        VGfloat ym = (y0 + y1)/2;
        VGfloat dsq, disc, s, sdx, sdy;
        // Solve for intersecting unit circles
        dsq = dx*dx + dy*dy;
        if (dsq == 0.0)
            return VG_FALSE; // Points are coincident
        disc = 1.0f/dsq - 1.0f/4.0f;
        if (disc < 0.0)
            return VG_FALSE; // Points are too far apart
        
        s = sqrt(disc);
        sdx = s*dx;
        sdy = s*dy;
        *cx0 = xm + sdy;
        *cy0 = ym - sdx;
        *cx1 = xm - sdy;
        *cy1 = ym + sdx;
        
        return VG_TRUE;
    }
    
    //Given:
    //Return:
    //Ellipse parameters rh, rv, rot (in degrees), endpoints (x0, y0) and (x1, y1) TRUE if a solution exists, FALSE otherwise. Ellipse centers are written to (cx0, cy0) and (cx1, cy1)
    
    static VGboolean findEllipses(VGfloat rh, VGfloat rv, VGfloat rot,
                                  VGfloat x0, VGfloat y0, VGfloat x1, VGfloat y1,
                                  VGfloat *cx0, VGfloat *cy0, VGfloat *cx1, VGfloat *cy1) {
        VGfloat COS, SIN, x0p, y0p, x1p, y1p, pcx0, pcy0, pcx1, pcy1;
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
            return VG_FALSE;
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
        
        return VG_TRUE;
    }
    
    
    void IPath::buildFill() {
        
        _vertices.clear();
        
        // reset the bounds
        _minX = VG_MAX_FLOAT;
        _minY = VG_MAX_FLOAT;
        _width = -VG_MAX_FLOAT;
        _height = -VG_MAX_FLOAT;
        
        _fillTesseleator = tessNewTess(NULL);
        
        //		gluTessCallback( _fillTesseleator, GLU_TESS_BEGIN_DATA, (GLvoid (APIENTRY *) ( )) &IPath::tessBegin );
        //		gluTessCallback( _fillTesseleator, GLU_TESS_END_DATA, (GLvoid (APIENTRY *) ( )) &IPath::tessEnd );
        //		gluTessCallback( _fillTesseleator, GLU_TESS_VERTEX_DATA, (GLvoid (APIENTRY *) ( )) &IPath::tessVertex );
        //		gluTessCallback( _fillTesseleator, GLU_TESS_COMBINE_DATA, (GLvoid (APIENTRY *) ( )) &IPath::tessCombine );
        //		gluTessCallback( _fillTesseleator, GLU_TESS_ERROR, (GLvoid (APIENTRY *)())&IPath::tessError );
        TessWindingRule winding = TESS_WINDING_POSITIVE;
        if( IContext::instance().getFillRule() == VG_EVEN_ODD ) {
            winding = TESS_WINDING_ODD;
            //			gluTessProperty( _fillTesseleator, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_ODD );
        } else if( IContext::instance().getFillRule() == VG_NON_ZERO ) {
            winding = TESS_WINDING_NONZERO;
            //			gluTessProperty( _fillTesseleator, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO );
        }
        //		gluTessProperty( _fillTesseleator, GLU_TESS_TOLERANCE, 0.5f );
        
        //		tessBeginPolygon( _fillTesseleator, this );
        
        
        std::vector< VGfloat >::iterator coordsIter = _fcoords->begin();
        VGbyte segment = VG_CLOSE_PATH;
        v3_t coords(0,0,0);
        v3_t prev(0,0,0);
        int num_contours = 0;
        
        for ( std::vector< VGubyte >::iterator segmentIter = _segments.begin(); segmentIter != _segments.end(); segmentIter++ ) {
            segment = (*segmentIter);
            //int numCoords = segmentToNumCoordinates( static_cast<VGPathSegment>( segment ) );
            //segment = segment >> 1;
            
            
            //			VG_CLOSE_PATH                               = ( 0 << 1),
            //			VG_MOVE_TO                                  = ( 1 << 1),
            //			VG_LINE_TO                                  = ( 2 << 1),
            //			VG_HLINE_TO                                 = ( 3 << 1),
            //			VG_VLINE_TO                                 = ( 4 << 1),
            //			VG_QUAD_TO                                  = ( 5 << 1),
            //			VG_CUBIC_TO                                 = ( 6 << 1),
            //			VG_SQUAD_TO                                 = ( 7 << 1),
            //			VG_SCUBIC_TO                                = ( 8 << 1),
            //			VG_SCCWARC_TO                               = ( 9 << 1),
            //			VG_SCWARC_TO                                = (10 << 1),
            //			VG_LCCWARC_TO                               = (11 << 1),
            //			VG_LCWARC_TO                                = (12 << 1),
            
            // todo: deal with relative move
            bool isRelative = segment & VG_RELATIVE;
            switch (segment >> 1) {
                case (VG_CLOSE_PATH >> 1):
                {
                    if ( num_contours ) {
                        //						gluTessEndContour( _fillTesseleator );
                        num_contours--;
                    }
                    
                } break;
                case (VG_MOVE_TO >> 1):
                {
                    if ( num_contours ) {
                        //						gluTessEndContour( _fillTesseleator );
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
                    VGfloat cp2x = *coordsIter; coordsIter++;
                    VGfloat cp2y = *coordsIter; coordsIter++;
                    VGfloat p3x = *coordsIter; coordsIter++;
                    VGfloat p3y = *coordsIter; coordsIter++;
                    
                    
                    if ( isRelative ) {
                        cp2x += prev.x;
                        cp2y += prev.y;
                        p3x += prev.x;
                        p3y += prev.y;
                    }
                    
                    VGfloat cp1x = 2.0f * cp2x - p3x;
                    VGfloat cp1y = 2.0f * cp2y - p3y;
                    
                    
                    VGfloat increment = 1.0f / IContext::instance().getTessellationIterations();
                    //printf("\tcubic: ");
                    for ( VGfloat t = increment; t < 1.0f + increment; t+=increment ) {
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
                    VGfloat cp1x = *coordsIter; coordsIter++;
                    VGfloat cp1y = *coordsIter; coordsIter++;
                    VGfloat cp2x = *coordsIter; coordsIter++;
                    VGfloat cp2y = *coordsIter; coordsIter++;
                    VGfloat p3x = *coordsIter; coordsIter++;
                    VGfloat p3y = *coordsIter; coordsIter++;
                    
                    if ( isRelative ) {
                        cp1x += prev.x;
                        cp1y += prev.y;
                        cp2x += prev.x;
                        cp2y += prev.y;
                        p3x += prev.x;
                        p3y += prev.y;
                    }
                    
                    VGfloat increment = 1.0f / IContext::instance().getTessellationIterations();
                    //printf("\tcubic: ");
                    for ( VGfloat t = increment; t < 1.0f + increment; t+=increment ) {
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
                    VGfloat cpx = *coordsIter; coordsIter++;
                    VGfloat cpy = *coordsIter; coordsIter++;
                    VGfloat px = *coordsIter; coordsIter++;
                    VGfloat py = *coordsIter; coordsIter++;
                    
                    if ( isRelative ) {
                        cpx += prev.x;
                        cpy += prev.y;
                        px += prev.x;
                        py += prev.y;
                    }
                    
                    VGfloat increment = 1.0f / IContext::instance().getTessellationIterations();
                    for ( VGfloat t = increment; t < 1.0f + increment; t+=increment ) {
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
                    VGfloat rh = *coordsIter; coordsIter++;
                    VGfloat rv = *coordsIter; coordsIter++;
                    VGfloat rot = *coordsIter; coordsIter++;
                    VGfloat cp1x = *coordsIter; coordsIter++;
                    VGfloat cp1y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        cp1x += prev.x;
                        cp1y += prev.y;
                    }
                    
                    // convert to Center Parameterization (see OpenVG Spec Apendix A)
                    VGfloat cx0[2];
                    VGfloat cx1[2];
                    VGboolean success = findEllipses( rh, rv, rot,
                                                     coords.x, coords.y, cp1x, cp1y,
                                                     &cx0[0], &cx0[1], &cx1[0], &cx1[1] );
                    
                    if ( success ) {
                        // see: http://en.wikipedia.org/wiki/Ellipse#Ellipses_in_computer_graphics
                        const int steps = IContext::instance().getTessellationIterations();
                        VGfloat beta = 0;	// angle. todo
                        VGfloat sinbeta = sinf( beta );
                        VGfloat cosbeta = cosf( beta );
                        
                        // calculate the start and end angles
                        v2_t center;
                        center.x = cx0[0];
                        center.y = cx0[1];
                        v2_t norm[2];
                        norm[0].x = center.x - coords.x;
                        norm[0].y = center.y - coords.y;
                        VGfloat inverse_len = 1.0f/sqrtf( (norm[0].x * norm[0].x) + (norm[0].y * norm[0].y) );
                        norm[0].x *= inverse_len;
                        norm[0].y *= inverse_len;
                        
                        norm[1].x = center.x - cp1x;
                        norm[1].y = center.y - cp1y;
                        inverse_len = 1.0f/sqrtf( (norm[1].x * norm[1].x) + (norm[1].y * norm[1].y) );
                        norm[1].x *= inverse_len;
                        norm[1].y *= inverse_len;
                        VGfloat startAngle = degrees( acosf( -norm[0].x ) );
                        VGfloat endAngle = degrees( acosf( -norm[1].x ) );
                        
                        VGfloat cross = norm[0].x;
                        
                        if ( cross >= 0  ) {
                            startAngle = 360 - startAngle;
                            endAngle = 360 - endAngle;
                        }
                        if ( startAngle > endAngle ) {
                            VGfloat tmp = startAngle;
                            startAngle = endAngle;
                            endAngle = tmp;
                            startAngle = startAngle - 90;
                            endAngle = endAngle - 90;
                        }
                        for ( VGfloat g = startAngle; g < endAngle; g+=360/steps ) {
                            v3_t c;
                            
                            VGfloat alpha = g * (M_PI / 180.0f);
                            VGfloat sinalpha = sinf( alpha );
                            VGfloat cosalpha = cosf( alpha );
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
            //			gluTessEndContour( _fillTesseleator );
            num_contours--;
        }
        
        assert(num_contours == 0);
        
        const int nvp = 6;
        const int nve = 2;
        assert(tessTesselate(_fillTesseleator, winding, TESS_POLYGONS, nvp, nve, NULL) == 1);
        
        GLdouble startVertex_[2];
        GLdouble lastVertex_[2];
        GLdouble v[2];
        
        const float* verts = tessGetVertices(_fillTesseleator);
        //const int* vinds = tessGetVertexIndices(_fillTesseleator);
        const int* elems = tessGetElements(_fillTesseleator);
        //const int nverts = tessGetVertexCount(_fillTesseleator);
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
        
        //		gluTessEndPolygon( _fillTesseleator );
        
        tessDeleteTess(_fillTesseleator);
        _fillTesseleator = 0;
        
        // final calculation of the width and height
        _width = fabsf(_width - _minX);
        _height = fabsf(_height - _minY);
    }
    
    void IPath::buildFatLineSegment( std::vector<v2_t>& vertices, const v2_t& p0, const v2_t& p1, const float stroke_width ) {
        
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
    
    size_t IPath::numberOfvertices(std::vector<v2_t>& vertices) {
        size_t l = vertices.size();
        // If the line has duplicate vertices at the end, adjust length to remove them.
        while (l > 2 && vertices[l - 1] == vertices[l - 2]) {
            l--;
        }
        return l;
        
    }
    
    void IPath::applyLineStyles( std::vector<v2_t> &vertices, VGCapStyle style, VGJoinStyle join, VGfloat miterLimit, VGfloat stroke_width) {
        size_t len = numberOfvertices(vertices);
        
        const Coordinate firstVertex = vertices.front();
        const Coordinate lastVertex = vertices[len - 1];
        const bool closed = firstVertex == lastVertex;
        
        if (len == 2 && closed)return;
        
        const VGCapStyle beginCap = style;
        const VGCapStyle endCap = closed ? VG_CAP_BUTT : style;
        
        int8_t flip = 1;
        double distance = 0;
        bool startOfLine = true;
        
        Coordinate currentVertex = Coordinate::null(), prevVertex = Coordinate::null(),
        nextVertex = Coordinate::null();
        vec2<double> prevNormal = vec2<double>::null(), nextNormal = vec2<double>::null();
        
        // the last three vertices added
        e1 = e2 = e3 = -1;
        
        if (closed) {
            currentVertex = vertices[len - 2];
            nextNormal = util::perp(util::unit(vec2<double>(firstVertex - currentVertex)));
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
                nextVertex = Coordinate::null();
            }
            
            // if two consecutive vertices exist, skip the current one
            if (nextVertex && vertices[i] == nextVertex) {
                continue;
            }
            
            if (nextNormal) {
                prevNormal = nextNormal;
            }
            if (currentVertex) {
                prevVertex = currentVertex;
            }
            
            currentVertex = vertices[i];
            
            // Calculate how far along the line the currentVertex is
            if (prevVertex)
                distance += util::dist<double>(currentVertex, prevVertex);
            
            // Calculate the normal towards the next vertex in this line. In case
            // there is no next vertex, pretend that the line is continuing straight,
            // meaning that we are just using the previous normal.
            nextNormal = nextVertex ? util::perp(util::unit(vec2<double>(nextVertex - currentVertex)))
            : prevNormal;
            
            // If we still don't have a previous normal, this is the beginning of a
            // non-closed line, so we're doing a straight "join".
            if (!prevNormal) {
                prevNormal = nextNormal;
            }
            
            // Determine the normal of the join extrusion. It is the angle bisector
            // of the segments between the previous line and the next line.
            vec2<double> joinNormal = util::unit(prevNormal + nextNormal);
            
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
            const bool middleVertex = prevVertex && nextVertex;
            VGJoinStyle currentJoin = join;
            const VGCapStyle currentCap = nextVertex ? beginCap : endCap;
            
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
                    const float bevelLength = miterLength * util::mag(prevNormal + nextNormal) /
                    util::mag(prevNormal - nextNormal);
                    joinNormal = util::perp(joinNormal) * bevelLength * direction;
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
                        auto approxFractionalJoinNormal = util::unit(nextNormal * ((m + 1.0f) / (n + 1.0f)) + prevNormal);
                        addPieSliceVertex(currentVertex, flip, distance, approxFractionalJoinNormal, lineTurnsLeft, startVertex, triangleStore, vertices);
                    }
                    
                    addPieSliceVertex(currentVertex, flip, distance, joinNormal, lineTurnsLeft, startVertex, triangleStore, vertices);
                    
                    for (int k = n - 1; k >= 0; k--) {
                        auto approxFractionalJoinNormal = util::unit(prevNormal * ((k + 1.0f) / (n + 1.0f)) + nextNormal);
                        addPieSliceVertex(currentVertex, flip, distance, approxFractionalJoinNormal, lineTurnsLeft, startVertex, triangleStore, vertices);
                    }
                }
                
                // Start next segment
                if (nextVertex) {
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
                if (nextVertex) {
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
                if (nextVertex) {
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
                if (nextVertex) {
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
    
    void IPath::addCurrentVertex(const Coordinate& currentVertex,
                                      float flip,
                                      double distance,
                                      const vec2<double>& normal,
                                      float endLeft,
                                      float endRight,
                                      bool round,
                                      int32_t startVertex,
                                      std::vector<TriangleElement>& triangleStore, std::vector<v2_t> &vertices) {
        int8_t tx = round ? 1 : 0;
        
        vec2<double> extrude = normal * flip;
        if (endLeft)
            extrude = extrude - (util::perp(normal) * endLeft);
        e3 = (int32_t)addVertix(vertices, currentVertex.x, currentVertex.y, extrude.x, extrude.y, tx, 0,
                                distance) - startVertex;
        if (e1 >= 0 && e2 >= 0) {
            triangleStore.emplace_back(e1, e2, e3);
        }
        e1 = e2;
        e2 = e3;
        
        extrude = normal * (-flip);
        if (endRight)
            extrude = extrude - (util::perp(normal) * endRight);
        e3 = (int32_t)addVertix(vertices, currentVertex.x, currentVertex.y, extrude.x, extrude.y, tx, 1,
                                distance) - startVertex;
        if (e1 >= 0 && e2 >= 0) {
            triangleStore.emplace_back(e1, e2, e3);
        }
        e1 = e2;
        e2 = e3;
    }
    
    void IPath::addPieSliceVertex(const Coordinate& currentVertex,
                                       float flip,
                                       double distance,
                                       const vec2<double>& extrude,
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
    
    size_t IPath::addVertix(std::vector<v2_t> &vertices, int8_t x, int8_t y, float ex, float ey, int8_t tx, int8_t ty, int32_t linesofar) {
        
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
    
    void IPath::buildStroke() {
        _strokeVertices.clear();
        
        // get the native OpenGL context
        IContext& glContext = (MonkVG::IContext&)IContext::instance();
        
        const VGfloat stroke_width = glContext.getStrokeLineWidth();
        
        std::vector< VGfloat >::iterator coordsIter = _fcoords->begin();
        VGbyte segment = VG_CLOSE_PATH;
        v2_t coords = v2_t(0,0);
        v2_t prev = v2_t(0,0);
        v2_t closeTo = v2_t(0,0);
        for ( std::vector< VGubyte >::iterator segmentIter = _segments.begin(); segmentIter != _segments.end(); segmentIter++ ) {
            segment = (*segmentIter);
            //int numCoords = segmentToNumCoordinates( static_cast<VGPathSegment>( segment ) );
            //segment = segment >> 1;
            
            
            //			VG_CLOSE_PATH                               = ( 0 << 1),
            //			VG_MOVE_TO                                  = ( 1 << 1),
            //			VG_LINE_TO                                  = ( 2 << 1),
            //			VG_HLINE_TO                                 = ( 3 << 1),
            //			VG_VLINE_TO                                 = ( 4 << 1),
            //			VG_QUAD_TO                                  = ( 5 << 1),
            //			VG_CUBIC_TO                                 = ( 6 << 1),
            //			VG_SQUAD_TO                                 = ( 7 << 1),
            //			VG_SCUBIC_TO                                = ( 8 << 1),
            //			VG_SCCWARC_TO                               = ( 9 << 1),
            //			VG_SCWARC_TO                                = (10 << 1),
            //			VG_LCCWARC_TO                               = (11 << 1),
            //			VG_LCWARC_TO                                = (12 << 1),
            
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
                    VGfloat cp2x = *coordsIter; coordsIter++;
                    VGfloat cp2y = *coordsIter; coordsIter++;
                    VGfloat p3x = *coordsIter; coordsIter++;
                    VGfloat p3y = *coordsIter; coordsIter++;
                    
                    
                    if ( isRelative ) {
                        cp2x += prev.x;
                        cp2y += prev.y;
                        p3x += prev.x;
                        p3y += prev.y;
                    }
                    
                    VGfloat cp1x = 2.0f * cp2x - p3x;
                    VGfloat cp1y = 2.0f * cp2y - p3y;
                    
                    
                    VGfloat increment = 1.0f / IContext::instance().getTessellationIterations();
                    //printf("\tcubic: ");
                    for ( VGfloat t = increment; t < 1.0f + increment; t+=increment ) {
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
                    VGfloat cpx = *coordsIter; coordsIter++;
                    VGfloat cpy = *coordsIter; coordsIter++;
                    VGfloat px = *coordsIter; coordsIter++;
                    VGfloat py = *coordsIter; coordsIter++;
                    
                    if ( isRelative ) {
                        cpx += prev.x;
                        cpy += prev.y;
                        px += prev.x;
                        py += prev.y;
                    }
                    
                    VGfloat increment = 1.0f / IContext::instance().getTessellationIterations();
                    
                    for ( VGfloat t = increment; t < 1.0f + increment; t+=increment ) {
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
                    VGfloat cp1x = *coordsIter; coordsIter++;
                    VGfloat cp1y = *coordsIter; coordsIter++;
                    VGfloat cp2x = *coordsIter; coordsIter++;
                    VGfloat cp2y = *coordsIter; coordsIter++;
                    VGfloat p3x = *coordsIter; coordsIter++;
                    VGfloat p3y = *coordsIter; coordsIter++;
                    
                    if ( isRelative ) {
                        cp1x += prev.x;
                        cp1y += prev.y;
                        cp2x += prev.x;
                        cp2y += prev.y;
                        p3x += prev.x;
                        p3y += prev.y;
                    }
                    
                    
                    VGfloat increment = 1.0f / IContext::instance().getTessellationIterations();
                    
                    for ( VGfloat t = increment; t < 1.0f + increment; t+=increment ) {
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
                    VGfloat rh = *coordsIter; coordsIter++;
                    VGfloat rv = *coordsIter; coordsIter++;
                    VGfloat rot = *coordsIter; coordsIter++;
                    VGfloat cp1x = *coordsIter; coordsIter++;
                    VGfloat cp1y = *coordsIter; coordsIter++;
                    if ( isRelative ) {
                        cp1x += prev.x;
                        cp1y += prev.y;
                    }
                    
                    
                    // convert to Center Parameterization (see OpenVG Spec Apendix A)
                    VGfloat cx0[2];
                    VGfloat cx1[2];
                    VGboolean success = findEllipses( rh, rv, rot,
                                                     coords.x, coords.y, cp1x, cp1y,
                                                     &cx0[0], &cx0[1], &cx1[0], &cx1[1] );
                    
                    if ( success ) {
                        // see: http://en.wikipedia.org/wiki/Ellipse#Ellipses_in_computer_graphics 
                        const int steps = IContext::instance().getTessellationIterations();
                        VGfloat beta = 0;	// angle. todo
                        VGfloat sinbeta = sinf( beta );
                        VGfloat cosbeta = cosf( beta );
                        
                        // calculate the start and end angles
                        v2_t center;
                        center.x = cx0[0];//(cx0[0] + cx1[0])*0.5f;
                        center.y = cx0[1];//(cx0[1] + cx1[1])*0.5f;
                        v2_t norm[2];
                        norm[0].x = center.x - coords.x;
                        norm[0].y = center.y - coords.y;
                        VGfloat inverse_len = 1.0f/sqrtf( (norm[0].x * norm[0].x) + (norm[0].y * norm[0].y) );
                        norm[0].x *= inverse_len;
                        norm[0].y *= inverse_len;
                        
                        norm[1].x = center.x - cp1x;
                        norm[1].y = center.y - cp1y;
                        inverse_len = 1.0f/sqrtf( (norm[1].x * norm[1].x) + (norm[1].y * norm[1].y) );
                        norm[1].x *= inverse_len;
                        norm[1].y *= inverse_len;
                        VGfloat startAngle = degrees( acosf( -norm[0].x ) );
                        VGfloat endAngle = degrees( acosf( -norm[1].x ) );
                        VGfloat cross = norm[0].x;
                        if ( cross >= 0 ) {
                            startAngle = 360 - startAngle;
                            endAngle = 360 - endAngle;
                        }
                        if ( startAngle > endAngle ) {
                            VGfloat tmp = startAngle;
                            startAngle = endAngle;
                            endAngle = tmp;
                            startAngle = startAngle - 90;
                            endAngle = endAngle - 90;
                        }
                        
                        
                        prev = coords;
                        for ( VGfloat g = startAngle; g < endAngle + (360/steps); g+=360/steps ) {
                            v2_t c;
                            
                            VGfloat alpha = g * (M_PI / 180.0f);
                            VGfloat sinalpha = sinf( alpha );
                            VGfloat cosalpha = cosf( alpha );
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
    
    void IPath::endOfTesselation( VGbitfield paintModes ) {
        
        /// build fill vbo
        // TODO: BUGBUG: if in batch mode don't build the VBO!
        if ( _vertices.size() > 0 ) {
            if ( _fillVBO != -1 ) {
                GL->glDeleteBuffers( 1, &_fillVBO );
                _fillVBO = -1;
            }
            
            GL->glGenBuffers( 1, &_fillVBO );
            GL->glBindBuffer( GL_ARRAY_BUFFER, _fillVBO );
            if ( _fillPaintForPath && _fillPaintForPath->getPaintType() == VG_PAINT_TYPE_COLOR ) {
                GL->glBufferData( GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), &_vertices[0], GL_STATIC_DRAW );
            }
            
            _numberFillVertices = (int)_vertices.size()/2;
            _tessVertices.clear();
        } 
        
        /// build stroke vbo 
        if ( _strokeVertices.size() > 0 ) {
            // build the vertex buffer object VBO
            if ( _strokeVBO != -1 ) {
                GL->glDeleteBuffers( 1, &_strokeVBO );
                _strokeVBO = -1;
            }
            
            GL->glGenBuffers( 1, &_strokeVBO );
            GL->glBindBuffer( GL_ARRAY_BUFFER, _strokeVBO );
            GL->glBufferData( GL_ARRAY_BUFFER, _strokeVertices.size() * sizeof(float) * 2, &_strokeVertices[0], GL_STATIC_DRAW );
            _numberStrokeVertices = (int)_strokeVertices.size();
            
        }
        
        IBatch* glBatch = (IBatch*)IContext::instance().currentBatch();
        if( glBatch ) {	// if in batch mode update the current batch
            glBatch->addPathVertexData( &_vertices[0], _vertices.size()/2, 
                                       (float*)&_strokeVertices[0], _strokeVertices.size(), 
                                       paintModes );
            
        }
        
        // clear out vertex buffer
        _vertices.clear();
        _strokeVertices.clear();
    }
  
    IPath::~IPath() {
        switch (_datatype) {
            case VG_PATH_DATATYPE_F:
                _fcoords->clear();
                delete _fcoords;
                _fcoords = 0;
                break;
            default:
                // error 
                break;
        }

        if ( _fillTesseleator ) {
            tessDeleteTess( _fillTesseleator );
            _fillTesseleator = 0;
        }
        
        GL->glDeleteBuffers( 1, &_fillVBO );
    }
    
}

