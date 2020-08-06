/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */

#ifndef __mkPaint_h__
#define __mkPaint_h__

#include "vgCompat.h"
#include "sakaDefs.h"
#include "mkMath.h"

namespace MonkVG {
	
	class MKPaint {
	public:
	
        MKPaint();
        ~MKPaint();
	
		const float* getPaintColor() const {
			return _paintColor;
		}
        
        Color getColor() const {
            return Color(typename Color::value_type(_paintColor[0] * 255.0f),
                         typename Color::value_type(_paintColor[1] * 255.0f),
                         typename Color::value_type(_paintColor[2] * 255.0f),
                         typename Color::value_type(_paintColor[3] * 255.0f));
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
		Saka::vector<Stop_t>		_colorRampStops;

		
	};
	
}
#endif // __mkPaint_h__
