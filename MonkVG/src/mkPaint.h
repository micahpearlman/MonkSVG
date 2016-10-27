/*
 *  mkPaint.h
 *  MonkVG-Quartz
 *
 *  Created by Micah Pearlman on 3/3/09.
 *  Copyright 2009 Monk Games. All rights reserved.
 *
 */

#ifndef __mkPaint_h__
#define __mkPaint_h__

#include "mkBaseObject.h"
#include <vector>

namespace MonkVG {
	
	class MKPaint : public BaseObject {
	public:
	
        MKPaint();
        ~MKPaint();
	
		inline BaseObject::Type getType() const {
			return BaseObject::kPaintType;
		}
		
		//// parameter accessors/mutators ////
		virtual VGint getParameteri( const VGint p ) const;
		virtual VGfloat getParameterf( const VGint f ) const;
		virtual void getParameterfv( const VGint p, VGfloat *fv ) const;
		virtual void setParameter( const VGint p, const VGfloat f );
		virtual void setParameter( const VGint p, const VGint i );
		virtual void setParameter( const VGint p, const VGfloat* fv, const VGint cnt );
		
		const VGfloat* getPaintColor() const {
			return _paintColor;
		}
		
		VGPaintType getPaintType() { return _paintType; }
		void setPaintType( VGPaintType t ) { _paintType = t; }
		
		bool isDirty() { return _isDirty; }
		void setIsDirty( bool b ) { _isDirty = b; }
		
        void setGLState();
        
    private:
        bool		_isDirty;
        
        void calcStops(float ** stop0, float ** stop1, float g);
        void lerpColor(float * dst, float * stop0, float * stop1, float g);

		
	protected:
		
		VGPaintType				_paintType;
		VGfloat					_paintColor[4];
		VGColorRampSpreadMode	_colorRampSpreadMode;
		VGboolean				_colorRampPremultiplied;
		VGfloat					_paintLinearGradient[4];
		VGfloat					_paintRadialGradient[5];
		VGfloat					_paint2x3Gradient[6];
		VGTilingMode			_patternTilingMode;

		struct Stop_t {
			VGfloat a[5];
		};
		std::vector<Stop_t>		_colorRampStops;

		
	};
	
}
#endif // __mkPaint_h__
