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

#include "vgCompat.h"
#include <vector>

namespace MonkVG {
	
	class MKPaint {
	public:
	
        MKPaint();
        ~MKPaint();
	
		const float* getPaintColor() const {
			return _paintColor;
		}
		
		VGPaintType getPaintType() { return _paintType; }
		void setPaintType( VGPaintType t ) { _paintType = t; }

        void getPaintColor( float f[4] );
        void setPaintColor( const float f[4] );
        void setPaintLinearGradient( const float f[4] );
        void setPaintRadialGradient( const float f[5] );
        void setPaintColorRampStops( const float* f, int cnt );
		
    private:
        void calcStops(float ** stop0, float ** stop1, float g);
        void lerpColor(float * dst, float * stop0, float * stop1, float g);

		
	protected:
		
		VGPaintType				_paintType;
		float					_paintColor[4];
		VGColorRampSpreadMode	_colorRampSpreadMode;
		bool				_colorRampPremultiplied;
		float					_paintLinearGradient[4];
		float					_paintRadialGradient[5];
		float					_paint2x3Gradient[6];
		VGTilingMode			_patternTilingMode;

		struct Stop_t {
			float a[5];
		};
		std::vector<Stop_t>		_colorRampStops;

		
	};
	
}
#endif // __mkPaint_h__
