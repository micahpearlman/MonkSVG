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

using namespace MonkVG;

//static VGContext *g_context = NULL;

extern "C"
{
VG_API_CALL VGboolean vgCreateContextMNK( VGint width, VGint height, VGRenderingBackendTypeMNK backend )
{
	MK_LOG("Creating context %d, %d, %d", width, height, (int)backend);

    IContext::instance().setRenderingBackendType( backend );

	IContext::instance().setWidth( width );
	IContext::instance().setHeight( height );
	IContext::instance().Initialize();
	
	return VG_TRUE;
}

VG_API_CALL void vgResizeSurfaceMNK(VGint width, VGint height)
{
	IContext::instance().setWidth( width );
	IContext::instance().setHeight( height );
	IContext::instance().resize();

}

VG_API_CALL void vgDestroyContextMNK()
{
    IContext::instance().Terminate();
}

VG_API_CALL void VG_API_ENTRY vgSetf (VGParamType type, VGfloat value) VG_API_EXIT {
	IContext::instance().set( type, value );
}

VG_API_CALL void VG_API_ENTRY vgSeti (VGParamType type, VGint value) VG_API_EXIT {
	IContext::instance().set( type, value );
}

VG_API_CALL void VG_API_ENTRY vgSetfv(VGParamType type, VGint count,
									  const VGfloat * values) VG_API_EXIT {
	IContext::instance().set( type, values );
}
VG_API_CALL void VG_API_ENTRY vgSetiv(VGParamType type, VGint count,
									  const VGint * values) VG_API_EXIT {
}

VG_API_CALL VGfloat VG_API_ENTRY vgGetf(VGParamType type) VG_API_EXIT {
    VGfloat ret = -1;
    IContext::instance().get( type, ret );
    return ret;
}

VG_API_CALL VGint VG_API_ENTRY vgGeti(VGParamType type) VG_API_EXIT {
    VGint ret = -1;
    IContext::instance().get( type, ret );
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
	IContext::instance().clear( x, y, width, height );
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
	return IContext::instance().getError();
}
    
}


namespace MonkVG {
	
	IContext::IContext() 
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
		,	_imageMode( VG_DRAW_IMAGE_NORMAL )
	{
		_path_user_to_surface.setIdentity();
		_glyph_user_to_surface.setIdentity();
		_image_user_to_surface.setIdentity();
		_active_matrix->setIdentity();
		_glyph_origin[0] = _glyph_origin[1] = 0;
		
		setImageMode( _imageMode );
	}
	
	//// parameters ////
	void IContext::set( VGuint type, VGfloat f ) {
		switch ( type ) {
			case VG_STROKE_LINE_WIDTH:
				setStrokeLineWidth( f );
				break;
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				break;
		}
	}
	
	void IContext::set( VGuint type, const VGfloat * fv ) {
		switch ( type ) {
			case VG_CLEAR_COLOR:
				setClearColor( fv );
				break;
			case VG_GLYPH_ORIGIN:
				setGlyphOrigin( fv );
				break;
	
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				break;
		}
	}
	
	void IContext::set( VGuint type, VGint i ) {
		
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
			case VG_IMAGE_MODE:
				setImageMode( (VGImageMode)i );
				break;
			default:
				break;
		}
		
	}
	void IContext::get( VGuint type, VGfloat &f ) const {
		switch ( type ) {
			case VG_STROKE_LINE_WIDTH:
				f = getStrokeLineWidth();
				break;
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				break;
		}
		
	}
	
	void IContext::get( VGuint type, VGfloat *fv ) const {
		switch ( type ) {
			case VG_CLEAR_COLOR:
				getClearColor( fv );
				break;
			case VG_GLYPH_ORIGIN:
				getGlyphOrigin( fv );
				break;
				
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				break;
		}
		
	}
	void IContext::get( VGuint type, VGint& i ) const {
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
			case VG_IMAGE_MODE:
				i = getImageMode( );
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
		IContext::instance().setError( e );
	}
}

#include "mkPath.h"
#include "mkPaint.h"
#include "mkBatch.h"
#include "mkCommon.h"


namespace MonkVG {
    
    //// singleton implementation ////
    IContext& IContext::instance()
    {
        static IContext g_context;
        return g_context;
    }
    
    
    void IContext::checkGLError() {
        
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
    
    bool IContext::Initialize() {
        // create the gl backend context dependent on user selected backend
        if ( getRenderingBackendType() == VG_RENDERING_BACKEND_TYPE_OPENGLES11 ) {
            _gl = new OpenGLES::OpenGLES1::OpenGLES11Context();
        } else if ( getRenderingBackendType() == VG_RENDERING_BACKEND_TYPE_OPENGLES20 ) {
            _gl = new OpenGLES::OpenGLES2::OpenGLES20Context();
        } else {    // error
            MK_ASSERT( !"ERROR: No OpenGL rendering backend selected" );
        }
        
        // get viewport to restore back when we are done
        gl()->glGetIntegerv( GL_VIEWPORT, _viewport );
        //fixme?		gl()->glGetFloatv( GL_PROJECTION_MATRIX, _projection );
        //fixme?		gl()->glGetFloatv( GL_MODELVIEW_MATRIX, _modelview );
        
        // get the color to back up when we are done
        gl()->glGetFloatv( GL_CURRENT_COLOR, _color );
        
        resize();
        
        gl()->glDisable(GL_CULL_FACE);
        gl()->glDisable(GL_TEXTURE_2D);
        
        // turn on blending
        gl()->glEnable(GL_BLEND);
        gl()->glEnable(GL_POINT_SMOOTH);
        gl()->glEnable(GL_LINE_SMOOTH);
        gl()->glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        gl()->glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
        
        gl()->glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        gl()->glEnable(GL_MULTISAMPLE);
        gl()->glEnable(GL_DEPTH_TEST);
        
        gl()->glDisable(GL_TEXTURE_2D);
        gl()->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
        gl()->glDisableClientState( GL_COLOR_ARRAY );
        gl()->glEnableClientState( GL_VERTEX_ARRAY );
        return true;
    }
    
    void IContext::resize() {
        // setup GL projection
        gl()->glViewport(0,0, _width, _height);
        
        gl()->glMatrixMode(GL_PROJECTION);
        gl()->glLoadIdentity();
        gl()->glOrthof(0, _width,		// left, right
                       0, _height,	// top, botton
                       -1, 1);		// near value, far value (depth)
        
        gl()->glMatrixMode(GL_MODELVIEW);
        gl()->glLoadIdentity();
    }
    
    
    bool IContext::Terminate() {
        if (_gl) {
            delete _gl;
            _gl = NULL;
        }
        _stroke_paint = NULL;
        _fill_paint = NULL;
        return true;
    }
    
    
    void IContext::beginRender() {
        //		glDisable(GL_TEXTURE_2D);
        //		glDisableClientState( GL_TEXTURE_COORD_ARRAY );
        //		glDisableClientState( GL_COLOR_ARRAY );
        //		glEnableClientState( GL_VERTEX_ARRAY );
        
        //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        
        
        //		CHECK_GL_ERROR;
        //
        //		// get viewport to restore back when we are done
        //		glGetIntegerv( GL_VIEWPORT, _viewport );
        //		glGetFloatv( GL_PROJECTION_MATRIX, _projection );
        //		glGetFloatv( GL_MODELVIEW_MATRIX, _modelview );
        //
        //		// get the color to back up when we are done
        //		glGetFloatv( GL_CURRENT_COLOR, _color );
        //
        //		// setup GL projection
        //		glViewport(0,0, _width, _height);
        //
        //		glMatrixMode(GL_PROJECTION);
        //		glLoadIdentity();
        //		glOrthof(0, _width,		// left, right
        //				 0, _height,	// top, botton
        //				 -1, 1);		// near value, far value (depth)
        //
        //		glMatrixMode(GL_MODELVIEW);
        //		glLoadIdentity();
        //
        //		glDisable( GL_CULL_FACE );
        //
        //		// turn on blending
        //		glEnable(GL_BLEND);
        //		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //
        //		CHECK_GL_ERROR;
        
    }
    void IContext::endRender() {
        //
        //		CHECK_GL_ERROR;
        //
        //		// todo: restore state to be nice to other apps
        //		// restore the old viewport
        //		glMatrixMode( GL_PROJECTION );
        //		glLoadMatrixf( _projection );
        //		glViewport( _viewport[0], _viewport[1], _viewport[2], _viewport[3] );
        //		glMatrixMode( GL_MODELVIEW );
        //		glLoadMatrixf( _modelview );
        //
        //		// restore color
        //		glColor4f( _color[0], _color[1], _color[2], _color[3] );
        //
        //		CHECK_GL_ERROR;
    }
    
    
    /// factories
    
    IPath* IContext::createPath( VGint pathFormat, VGPathDatatype datatype, VGfloat scale, VGfloat bias, VGint segmentCapacityHint, VGint coordCapacityHint, VGbitfield capabilities ) {
        
        IPath *path = new IPath(pathFormat, datatype, scale, bias, segmentCapacityHint, coordCapacityHint, capabilities  &= VG_PATH_CAPABILITY_ALL);
        if( path == 0 )
            SetError( VG_OUT_OF_MEMORY_ERROR );
        
        return (IPath*)path;
    }
    
    void IContext::destroyPath( IPath* path ) {
        delete (IPath*)path;
    }
    
    void IContext::destroyPaint( IPaint* paint ) {
        delete (IPaint*)paint;
    }
    
    IPaint* IContext::createPaint() {
        IPaint *paint = new IPaint();
        if( paint == 0 )
            SetError( VG_OUT_OF_MEMORY_ERROR );
        return (IPaint*)paint;
    }
    
    IBatch* IContext::createBatch() {
        IBatch *batch = new IBatch();
        if( batch == 0 )
            SetError( VG_OUT_OF_MEMORY_ERROR );
        return (IBatch*)batch;
    }
    
    void IContext::destroyBatch( IBatch* batch ) {
        if ( batch ) {
            delete batch;
        }
    }
    
    
    /// state
    void IContext::setStrokePaint( IPaint* paint ) {
        if ( paint != _stroke_paint ) {
            _stroke_paint = paint;
            IPaint* glPaint = (IPaint*)_stroke_paint;
            //glPaint->setGLState();
            if (glPaint)
                glPaint->setIsDirty( true );
        }
    }
    
    void IContext::setFillPaint( IPaint* paint ) {
        if ( paint != _fill_paint ) {
            _fill_paint = paint;
            IPaint* glPaint = (IPaint*)_fill_paint;
            //glPaint->setGLState();
            if (glPaint)
                glPaint->setIsDirty( true );
        }
        
    }
    
    
    void IContext::stroke() {
        if ( _stroke_paint ) {
            IPaint* glPaint = (IPaint*)_stroke_paint;
            glPaint->setGLState();
            glPaint->setIsDirty( false );
            // set the fill paint to dirty
            if ( _fill_paint ) {
                glPaint = (IPaint*)_fill_paint;
                glPaint->setIsDirty( true );
            }
        }
    }
    
    void IContext::fill() {
        
        if ( _fill_paint && _fill_paint->getPaintType() == VG_PAINT_TYPE_COLOR ) {
            IPaint* glPaint = (IPaint*)_fill_paint;
            glPaint->setGLState();
            glPaint->setIsDirty( false );
            // set the stroke paint to dirty
            if ( _stroke_paint ) {
                glPaint = (IPaint*)_stroke_paint;
                glPaint->setIsDirty( true );
            }
        }
        
        //		if ( _fill_paint ) {
        //			const VGfloat* c = _fill_paint->getPaintColor();
        //			glColor4f(c[0], c[1], c[2], c[3] );
        //		}
        
    }
    
    void IContext::startBatch( IBatch* batch ) {
        assert( _currentBatch == 0 );	// can't have multiple batches going on at once
        _currentBatch = batch;
    }
    void IContext::dumpBatch( IBatch *batch, void **vertices, size_t *size ) {
        _currentBatch->dump( vertices, size );
    }
    void IContext::endBatch( IBatch* batch ) {
        _currentBatch->finalize();
        _currentBatch = 0;
    }
    
    
    void IContext::clear(VGint x, VGint y, VGint width, VGint height) {
        // TODO:
    }
    
    void IContext::loadGLMatrix() {
        Matrix33& active = *getActiveMatrix();
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
        gl()->glLoadMatrixf( &mat44[0][0] );
        
    }
    
    
    
    void IContext::setIdentity() {
        Matrix33* active = getActiveMatrix();
        active->setIdentity();
        loadGLMatrix();
    }
    
    void IContext::transform( VGfloat* t ) {
        // a	b	0
        // c	d	0
        // tx	ty	1
        Matrix33* active = getActiveMatrix();
        for( int i = 0; i < 9; i++ )
            t[i] = active->m[i];
        
    }
    
    void IContext::setTransform( const VGfloat* t )  {
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
    
    
    void IContext::multiply( const VGfloat* t ) {
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
    
    void IContext::scale( VGfloat sx, VGfloat sy ) {
        Matrix33* active = getActiveMatrix();
        Matrix33 scale;
        scale.setIdentity();
        scale.setScale( sx, sy );
        Matrix33 tmp;
        Matrix33::multiply( tmp, scale, *active );
        active->copy( tmp );
        loadGLMatrix();
    }
    void IContext::translate( VGfloat x, VGfloat y ) {
        
        Matrix33* active = getActiveMatrix();
        Matrix33 translate;
        translate.setTranslate( x, y );
        Matrix33 tmp;
        tmp.setIdentity();
        Matrix33::multiply( tmp, translate, *active );
        active->copy( tmp );
        loadGLMatrix();
    }
    float IContext::angle()
    {
        Matrix33* active = getActiveMatrix();
        
        return active->angle();
    }
    
    void IContext::rotate( VGfloat angle ) {
        Matrix33* active = getActiveMatrix();
        Matrix33 rotate;
        rotate.setRotation( radians( angle ) );
        Matrix33 tmp;
        tmp.setIdentity();
        Matrix33::multiply( tmp, rotate, *active );
        active->copy( tmp );
        loadGLMatrix();
    }
    
    void IContext::rotate(VGfloat angle, VGfloat x, VGfloat y, VGfloat z) {
        
        translate(x, y);
        rotate(angle);
        
    }
}

