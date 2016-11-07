/*
 *  mkPaint.cpp
 *  MonkVG-Quartz
 *
 *  Created by Micah Pearlman on 3/3/09.
 *  Copyright 2009 Monk Games. All rights reserved.
 *
 */

#include "mkPaint.h"
#include <cassert>

namespace MonkVG {	// Internal Implementation
    void MKPaint::getPaintColor( float f[4] )
    {
        for( int i = 0; i < 4; i++ ) {
            f[i] = _paintColor[i];
        }
        
    }
    void MKPaint::setPaintColor( const float f[4] )
    {
        for( int i = 0; i < 4; i++ ) {
            _paintColor[i] = f[i];
        }

    }
    void MKPaint::setPaintLinearGradient( const float f[4] )
    {
        for ( int i = 0; i < 4; i++ ) {
            _paintLinearGradient[i] = f[i];
        }

    }
    void MKPaint::setPaintRadialGradient( const float f[5] )
    {
        for ( int i = 0; i < 5; i++ ) {
            _paintRadialGradient[i] = f[i];
        }

    }
    void MKPaint::setPaintColorRampStops( const float* f, int cnt )
    {
        for ( int j = 0; j < cnt/5; j++ ) {
            Stop_t stop;
            for ( int p = 0; p < 5; p++ ) {
                stop.a[p] = f[(j * 5) + p];
            }
            _colorRampStops.push_back( stop );
        }
    }
}


///// OpenVG API Implementation /////

namespace MonkVG {	// Internal Implementation

    MKPaint::MKPaint() :
        _paintType( VG_PAINT_TYPE_COLOR )	// default paint type is color
    {
        
    }

    MKPaint::~MKPaint() {
    }

    void MKPaint::lerpColor(float * dst, float * stop0, float * stop1, float g) {
        float den = std::max(0.00001f, stop1 != stop0? stop1[0] - stop0[0] : 1 - stop0[0]);
        for ( int i = 0; i < 4; i++ )
            dst[i] = stop0[i+1] + (stop1[i+1] - stop0[i+1])*(g - stop0[0])/den;
    }

    void MKPaint::calcStops(float ** stop0, float ** stop1, float g) {
        assert(g >= 0);
        assert(g <= 1);
        size_t stopCnt = _colorRampStops.size();
        float * s0 = 0;
        float * s1 = 0;
        for ( size_t i = 0; i < stopCnt && !s1; i++ ) {
            float * curr = _colorRampStops[i].a;
            if ( g >= curr[0] )
                s0 = curr;
            else if ( s0 && g <= curr[0] )
                s1 = curr;
        }
        if ( stopCnt == 0 ) {
            static float implicit0[] = {0,0,0,1};
            static float implicit1[] = {1,1,1,1};
            s0 = implicit0;
            s1 = implicit1;
        } else {
            if ( !s0 )
                s0 = _colorRampStops[0].a;
            if ( !s1 )
                s1 = _colorRampStops[stopCnt - 1].a;
        }
        assert(s0[0] <= g && (g <= s1[0] || s1 == _colorRampStops[stopCnt-1].a));
        *stop0 = s0;
        *stop1 = s1;
    }
}
