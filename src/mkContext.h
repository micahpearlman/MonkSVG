/*
 *  mkvgContext.h
 *  MonkVG-XCode
 *
 *  Created by Micah Pearlman on 2/22/09.
 *  Copyright 2009 Monk Games. All rights reserved.
 *
 */

#ifndef __mkContext_h__
#define __mkContext_h__

#include <VG/openvg.h>
#include <VG/vgext.h>
#include "mkPath.h"
#include "mkBatch.h"
#include "mkMath.h"
#include "mkPaint.h"

namespace MonkVG {

#define GL (((MKContext*)&MKContext::instance())->gl())

	class MKContext {
	public:
		MKContext();
		
		// singleton instance
		static MKContext& instance();
				
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
		inline VGint getWidth() const {
			return _width;
		}
		inline void setWidth( VGint w ) {
			_width = w;
		}
		
		inline VGint getHeight() const {
			return _height;
		}
		inline void setHeight( VGint h ) {
			_height = h;
		}
		
		//// parameters ////
		void set( VGuint type, VGfloat f );
		void set( VGuint type, const VGfloat * values );
		void set( VGuint type, VGint i );
		void get( VGuint type, VGfloat &f ) const;
		void get( VGuint type, VGfloat *fv ) const;
		void get( VGuint type, VGint &i ) const; 
		
		//// stroke parameters ////
		inline void setStrokeLineWidth( VGfloat w ) {
			_stroke_line_width = w;
		}
		inline VGfloat getStrokeLineWidth() const {
			return _stroke_line_width;
		}
		
		//// surface properties ////
		inline void setClearColor( const VGfloat *c ) {
			_clear_color[0] = c[0]; _clear_color[1] = c[1];
			_clear_color[2] = c[2]; _clear_color[3] = c[3];
		}
		inline void getClearColor( VGfloat *c ) const {
			c[0] = _clear_color[0]; c[1] = _clear_color[1];
			c[2] = _clear_color[2]; c[3] = _clear_color[3];
		}
		
		//// transforms ////
		inline Matrix33& getPathUserToSurface() {
			return _path_user_to_surface;
		}
		inline void setPathUserToSurface( const Matrix33& m ) {
			_path_user_to_surface = m;
		}
		void setMatrixMode( VGMatrixMode mode ) {
			//			VG_MATRIX_PATH_USER_TO_SURFACE              = 0x1400,
			//			VG_MATRIX_IMAGE_USER_TO_SURFACE             = 0x1401,
			//			VG_MATRIX_FILL_PAINT_TO_USER                = 0x1402,
			//			VG_MATRIX_STROKE_PAINT_TO_USER              = 0x1403,
			//			VG_MATRIX_GLYPH_USER_TO_SURFACE             = 0x1404,
			_matrixMode = mode;
			switch (mode) {
				case VG_MATRIX_PATH_USER_TO_SURFACE:
					_active_matrix = &_path_user_to_surface;
					break;
				case VG_MATRIX_IMAGE_USER_TO_SURFACE:
					_active_matrix = &_image_user_to_surface;
					break;
				case VG_MATRIX_GLYPH_USER_TO_SURFACE:
					_active_matrix = &_glyph_user_to_surface;
					break;
				default:
					SetError(VG_ILLEGAL_ARGUMENT_ERROR);
					break;
			}
            loadGLMatrix();
		}
		inline VGMatrixMode getMatrixMode() const { return _matrixMode; }
		inline Matrix33* getActiveMatrix() {
			return _active_matrix;
		}
		
		//// error handling ////
		inline VGErrorCode getError() const { 
			return _error; 
		}
		inline void setError( const VGErrorCode e ) {
			_error = e;
		}
		
		/// rendering quality
		inline VGRenderingQuality getRenderingQuality() const { return _renderingQuality; }
		inline void setRenderingQuality( VGRenderingQuality rc ) { _renderingQuality = rc; }
		
		inline int32_t getTessellationIterations() const { return _tessellationIterations; }
		inline void setTessellationIterations( int32_t i ) { _tessellationIterations = i; }
		
		MKBatch* currentBatch() { return _currentBatch; }
		
	protected:
	
		// surface properties
		VGint				_width, _height;
		VGfloat				_clear_color[4];
		
		// matrix transforms
		Matrix33			_path_user_to_surface;
		Matrix33			_image_user_to_surface;
		Matrix33			_glyph_user_to_surface;
		Matrix33			*_active_matrix;
		VGMatrixMode		_matrixMode;
		
		// stroke properties
		VGfloat				_stroke_line_width;			// VG_STROKE_LINE_WIDTH
		
		// rendering quality
		VGRenderingQuality	_renderingQuality;
		int32_t				_tessellationIterations;

        // paints
        MKPaint*				_stroke_paint;
        MKPaint*				_fill_paint;
        VGFillRule			_fill_rule;
        
		// batch
		MKBatch*				_currentBatch;
		
		// error
		VGErrorCode			_error;
        
    public:
        bool Initialize();
        bool Terminate();
        
        //// factories ////
        MKPaint* createPaint();
        void destroyPaint( MKPaint* paint );
        MKPath* createPath( VGint pathFormat, VGPathDatatype datatype, VGfloat scale, VGfloat bias, VGint segmentCapacityHint, VGint coordCapacityHint, VGbitfield capabilities );
        void destroyPath( MKPath* path );
        MKBatch* createBatch();
        void destroyBatch( MKBatch* batch );
        
        //// platform specific execution of stroke and fill ////
        void stroke();
        void fill();
        
        //// platform specific execution of Masking and Clearing ////
        void clear(VGint x, VGint y, VGint width, VGint height);
        
        //// platform specific implementation of transform ////
        void setIdentity();
        void transform( VGfloat* t );
        void scale( VGfloat sx, VGfloat sy );
        void translate( VGfloat x, VGfloat y );
        float angle ();
        void rotate( VGfloat angle );
        void rotate( VGfloat angle , VGfloat x, VGfloat y, VGfloat z);
        void setTransform( const VGfloat* t ) ;
        void multiply( const VGfloat* t );
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
}

#endif // __mkContext_h__
