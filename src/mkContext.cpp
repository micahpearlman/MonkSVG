/*
 *  mkContext.cpp
 *  MonkVG-XCode
 *
 *  Created by Micah Pearlman on 2/22/09.
 *  Copyright 2009 Monk Games. All rights reserved.
 *
 */
#include "mkContext.h"
#include "mkPath.h"
#include <Tess2/tesselator.h>

using namespace MonkVG;

//static VGContext *g_context = NULL;

extern "C"
{
VG_API_CALL VGboolean vgCreateContextMNK( VGint width, VGint height )
{
	MK_LOG("Creating context %d, %d\n", width, height);
	MKContext::instance().setWidth( width );
	MKContext::instance().setHeight( height );
	MKContext::instance().Initialize();
	
	return VG_TRUE;
}

VG_API_CALL void vgResizeSurfaceMNK(VGint width, VGint height)
{
	MKContext::instance().setWidth( width );
	MKContext::instance().setHeight( height );
	MKContext::instance().resize();

}

VG_API_CALL void vgDestroyContextMNK()
{
    MKContext::instance().Terminate();
}

VG_API_CALL void VG_API_ENTRY vgSetf (VGParamType type, VGfloat value) VG_API_EXIT {
	MKContext::instance().set( type, value );
}

VG_API_CALL void VG_API_ENTRY vgSeti (VGParamType type, VGint value) VG_API_EXIT {
	MKContext::instance().set( type, value );
}

VG_API_CALL void VG_API_ENTRY vgSetfv(VGParamType type, VGint count,
									  const VGfloat * values) VG_API_EXIT {
	MKContext::instance().set( type, values );
}
VG_API_CALL void VG_API_ENTRY vgSetiv(VGParamType type, VGint count,
									  const VGint * values) VG_API_EXIT {
}

VG_API_CALL VGfloat VG_API_ENTRY vgGetf(VGParamType type) VG_API_EXIT {
    VGfloat ret = -1;
    MKContext::instance().get( type, ret );
    return ret;
}

VG_API_CALL VGint VG_API_ENTRY vgGeti(VGParamType type) VG_API_EXIT {
    VGint ret = -1;
    MKContext::instance().get( type, ret );
    return ret;
}

VG_API_CALL VGint VG_API_ENTRY vgGetVectorSize(VGParamType type) VG_API_EXIT {
	return -1;
}

VG_API_CALL void VG_API_ENTRY vgGetfv(VGParamType type, VGint count, VGfloat * values) VG_API_EXIT {
	
}

VG_API_CALL void VG_API_ENTRY vgGetiv(VGParamType type, VGint count, VGint * values) VG_API_EXIT {
	
}

/* Masking and Clearing */
VG_API_CALL void VG_API_ENTRY vgClear(VGint x, VGint y, VGint width, VGint height) VG_API_EXIT {
	MKContext::instance().clear( x, y, width, height );
}

VG_API_CALL void VG_API_ENTRY vgMask(VGHandle mask, VGMaskOperation operation,VGint x, VGint y,
		VGint width, VGint height) VG_API_EXIT {

}


/* Finish and Flush */
VG_API_CALL void VG_API_ENTRY vgFinish(void) VG_API_EXIT {
	glFinish();
}
VG_API_CALL void VG_API_ENTRY vgFlush(void) VG_API_EXIT {
	glFlush();
}

/*--------------------------------------------------
 * Returns the oldest error pending on the current
 * context and clears its error code
 *--------------------------------------------------*/

VG_API_CALL VGErrorCode vgGetError(void)
{
	return MKContext::instance().getError();
}
    
}


namespace MonkVG {
	
	MKContext::MKContext() 
		:	_error( VG_NO_ERROR )
		,	_width( 0 )
		,	_height( 0 )
		,	_stroke_line_width( 1.0f )
		,	_stroke_paint( 0 )
		,	_fill_paint( 0 )
		,	_active_matrix( &_path_user_to_surface )
		,	_fill_rule( VG_EVEN_ODD )
		,	_renderingQuality( VG_RENDERING_QUALITY_BETTER )
		,	_tessellationIterations( 16 )
		,	_matrixMode( VG_MATRIX_PATH_USER_TO_SURFACE )
		,	_currentBatch( 0 )
	{
		_path_user_to_surface.setIdentity();
		_glyph_user_to_surface.setIdentity();
		_image_user_to_surface.setIdentity();
		_active_matrix->setIdentity();
        
        disableAutomaticCleanup();
	}
	
	//// parameters ////
	void MKContext::set( VGuint type, VGfloat f ) {
		switch ( type ) {
			case VG_STROKE_LINE_WIDTH:
				setStrokeLineWidth( f );
				break;
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				break;
		}
	}
	
	void MKContext::set( VGuint type, const VGfloat * fv ) {
		switch ( type ) {
			case VG_CLEAR_COLOR:
				setClearColor( fv );
				break;
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				break;
		}
	}
	
	void MKContext::set( VGuint type, VGint i ) {
		
		switch ( type ) {
			case VG_MATRIX_MODE:
				setMatrixMode( (VGMatrixMode)i );
				break;
			case VG_FILL_RULE:
				setFillRule( (VGFillRule)i );
				break;
			case VG_TESSELLATION_ITERATIONS_MNK:
				setTessellationIterations( i );
				break;
			default:
				break;
		}
		
	}
	void MKContext::get( VGuint type, VGfloat &f ) const {
		switch ( type ) {
			case VG_STROKE_LINE_WIDTH:
				f = getStrokeLineWidth();
				break;
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				break;
		}
		
	}
	
	void MKContext::get( VGuint type, VGfloat *fv ) const {
		switch ( type ) {
			case VG_CLEAR_COLOR:
				getClearColor( fv );
				break;
				
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				break;
		}
		
	}
	void MKContext::get( VGuint type, VGint& i ) const {
		i = -1;

		switch ( type ) {
			case VG_MATRIX_MODE:
				i = getMatrixMode( );
				break;
			case VG_FILL_RULE:
				i =  getFillRule( );
				break;
			case VG_TESSELLATION_ITERATIONS_MNK:
				i = getTessellationIterations( );
				break;
            case VG_SURFACE_WIDTH_MNK:
                i = getWidth();
                break;
            case VG_SURFACE_HEIGHT_MNK:
                i = getHeight();
                break;
				
			default:
				break;
		}
		

	}

	void SetError( const VGErrorCode e ) {
		MKContext::instance().setError( e );
	}
}

#include "mkPath.h"
#include "mkPaint.h"
#include "mkBatch.h"
#include "mkCommon.h"


namespace MonkVG {
    
    //// singleton implementation ////
    MKContext& MKContext::instance()
    {
        static MKContext g_context;
        return g_context;
    }
    
    
    void MKContext::checkGLError() {
        
        int err = glGetError();
        
        
        const char* RVAL = "GL_UNKNOWN_ERROR";
        
        switch( err )
        {
            case GL_NO_ERROR:
                RVAL =  "GL_NO_ERROR";
                break;
            case GL_INVALID_ENUM:
                RVAL =  "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                RVAL =  "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                RVAL = "GL_INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                RVAL =  "GL_STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                RVAL =  "GL_STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                RVAL =  "GL_OUT_OF_MEMORY";
                break;
            default:
                break;
        }
        
        if( err != GL_NO_ERROR ) {
            printf("GL_ERROR: %s\n", RVAL );
            MK_ASSERT(0);
        }
    }
    
    bool MKContext::Initialize() {
        glGetIntegerv(GL_VIEWPORT, _viewport);
        resize();
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);

        return true;
    }
    
    void MKContext::resize() {
        // setup GL projection
        glViewport(0,0, _width, _height);
    }
    
    
    bool MKContext::Terminate() {
        _stroke_paint = NULL;
        _fill_paint = NULL;
        return true;
    }
    
    
    void MKContext::beginRender() {
    }
    void MKContext::endRender() {
    }
    
    
    /// factories
    
    MKPath* MKContext::createPath( VGint pathFormat, VGPathDatatype datatype, VGfloat scale, VGfloat bias, VGint segmentCapacityHint, VGint coordCapacityHint, VGbitfield capabilities ) {
        
        MKPath *path = new MKPath(pathFormat, datatype, scale, bias, segmentCapacityHint, coordCapacityHint, capabilities  &= VG_PATH_CAPABILITY_ALL);
        if( path == 0 )
            SetError( VG_OUT_OF_MEMORY_ERROR );
        
        return (MKPath*)path;
    }
    
    void MKContext::destroyPath( MKPath* path ) {
        delete (MKPath*)path;
    }
    
    void MKContext::destroyPaint( MKPaint* paint ) {
        delete (MKPaint*)paint;
    }
    
    MKPaint* MKContext::createPaint() {
        MKPaint *paint = new MKPaint();
        if( paint == 0 )
            SetError( VG_OUT_OF_MEMORY_ERROR );
        return (MKPaint*)paint;
    }
    
    MKBatch* MKContext::createBatch() {
        MKBatch *batch = new MKBatch();
        if( batch == 0 )
            SetError( VG_OUT_OF_MEMORY_ERROR );
        return (MKBatch*)batch;
    }
    
    void MKContext::destroyBatch( MKBatch* batch ) {
        if ( batch ) {
            delete batch;
        }
    }
    
    
    /// state
    void MKContext::setStrokePaint( MKPaint* paint ) {
        if ( paint != _stroke_paint ) {
            _stroke_paint = paint;
            MKPaint* glPaint = (MKPaint*)_stroke_paint;
            //glPaint->setGLState();
            if (glPaint)
                glPaint->setIsDirty( true );
        }
    }
    
    void MKContext::setFillPaint( MKPaint* paint ) {
        if ( paint != _fill_paint ) {
            _fill_paint = paint;
            MKPaint* glPaint = (MKPaint*)_fill_paint;
            //glPaint->setGLState();
            if (glPaint)
                glPaint->setIsDirty( true );
        }
        
    }
    
    
    void MKContext::stroke() {
        if ( _stroke_paint ) {
            MKPaint* glPaint = (MKPaint*)_stroke_paint;
            glPaint->setGLState();
            glPaint->setIsDirty( false );
            // set the fill paint to dirty
            if ( _fill_paint ) {
                glPaint = (MKPaint*)_fill_paint;
                glPaint->setIsDirty( true );
            }
        }
    }
    
    void MKContext::fill() {
        
        if ( _fill_paint && _fill_paint->getPaintType() == VG_PAINT_TYPE_COLOR ) {
            MKPaint* glPaint = (MKPaint*)_fill_paint;
            glPaint->setGLState();
            glPaint->setIsDirty( false );
            // set the stroke paint to dirty
            if ( _stroke_paint ) {
                glPaint = (MKPaint*)_stroke_paint;
                glPaint->setIsDirty( true );
            }
        }
   }
    
    void MKContext::startBatch( MKBatch* batch ) {
        assert( _currentBatch == 0 );	// can't have multiple batches going on at once
        _currentBatch = batch;
    }
    void MKContext::endBatch( MKBatch* batch ) {
        _currentBatch->finalize();
        _currentBatch = 0;
    }
    
    
    void MKContext::clear(VGint x, VGint y, VGint width, VGint height) {
        // TODO:
    }
    
    void MKContext::loadGLMatrix() {
/*        Matrix33& active = *getActiveMatrix();
        GLfloat mat44[4][4];
        for( int x = 0; x < 4; x++ )
            for( int y = 0; y < 4; y++ )
                mat44[x][y] = 0;
        mat44[0][0] = 1.0f;
        mat44[1][1] = 1.0f;
        mat44[2][2] = 1.0f;
        mat44[3][3]	= 1.0f;
        
        
        //		a, c, e,			// cos(a) -sin(a) tx
        //		b, d, f,			// sin(a) cos(a)  ty
        //		ff0, ff1, ff2;		// 0      0       1
        
        mat44[0][0] = active.a;	mat44[0][1] = active.b;
        mat44[1][0] = active.c;	mat44[1][1] = active.d;
        mat44[3][0] = active.e;	mat44[3][1] = active.f;
        glLoadMatrixf( &mat44[0][0] );
*/        
    }
    
    
    
    void MKContext::setIdentity() {
        Matrix33* active = getActiveMatrix();
        active->setIdentity();
        loadGLMatrix();
    }
    
    void MKContext::transform( VGfloat* t ) {
        // a	b	0
        // c	d	0
        // tx	ty	1
        Matrix33* active = getActiveMatrix();
        for( int i = 0; i < 9; i++ )
            t[i] = active->m[i];
        
    }
    
    void MKContext::setTransform( const VGfloat* t )  {
        //	OpenVG:
        //	sh	shx	tx
        //	shy	sy	ty
        //	0	0	1
        //
        // OpenGL
        // a	b	0
        // c	d	0
        // tx	ty	1
        
        Matrix33* active = getActiveMatrix();
        for( int i = 0; i < 9; i++ )
            active->m[i] = t[i];
        loadGLMatrix();
    }
    
    
    void MKContext::multiply( const VGfloat* t ) {
        Matrix33 m;
        for ( int x = 0; x < 3; x++ ) {
            for ( int y = 0; y < 3; y++ ) {
                m.set( y, x, t[(y*3)+x] );
            }
        }
        Matrix33* active = getActiveMatrix();
        active->postMultiply( m );
        loadGLMatrix();
    }
    
    void MKContext::scale( VGfloat sx, VGfloat sy ) {
        Matrix33* active = getActiveMatrix();
        Matrix33 scale;
        scale.setIdentity();
        scale.setScale( sx, sy );
        Matrix33 tmp;
        Matrix33::multiply( tmp, scale, *active );
        active->copy( tmp );
        loadGLMatrix();
    }
    void MKContext::translate( VGfloat x, VGfloat y ) {
        
        Matrix33* active = getActiveMatrix();
        Matrix33 translate;
        translate.setTranslate( x, y );
        Matrix33 tmp;
        tmp.setIdentity();
        Matrix33::multiply( tmp, translate, *active );
        active->copy( tmp );
        loadGLMatrix();
    }
    float MKContext::angle()
    {
        Matrix33* active = getActiveMatrix();
        
        return active->angle();
    }
    
    void MKContext::rotate( VGfloat angle ) {
        Matrix33* active = getActiveMatrix();
        Matrix33 rotate;
        rotate.setRotation( radians( angle ) );
        Matrix33 tmp;
        tmp.setIdentity();
        Matrix33::multiply( tmp, rotate, *active );
        active->copy( tmp );
        loadGLMatrix();
    }
    
    void MKContext::rotate(VGfloat angle, VGfloat x, VGfloat y, VGfloat z) {
        
        translate(x, y);
        rotate(angle);
        
    }
}

