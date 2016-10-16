/*
 *  mkMath.cpp
 *  MonkVG-Quartz
 *
 *  Created by Micah Pearlman on 3/11/09.
 *  Copyright 2009 Monk Games. All rights reserved.
 *
 */

#include "mkMath.h"
#include "mkContext.h"

using namespace MonkVG;

//// OpenVG API Implementation ////

//// Matrix Manipulation ////
VG_API_CALL void VG_API_ENTRY vgLoadIdentity(void) VG_API_EXIT {
	MKContext::instance().setIdentity();
}

VG_API_CALL void VG_API_ENTRY vgLoadMatrix(const VGfloat * m) VG_API_EXIT {
	MKContext::instance().setTransform( m );
}

VG_API_CALL void VG_API_ENTRY vgGetMatrix(VGfloat * m) VG_API_EXIT {
	MKContext::instance().transform( m );
}

VG_API_CALL void VG_API_ENTRY vgMultMatrix(const VGfloat * m) VG_API_EXIT {
	MKContext::instance().multiply( m );
}

VG_API_CALL void VG_API_ENTRY vgTranslate(VGfloat tx, VGfloat ty) VG_API_EXIT {
	MKContext::instance().translate( tx, ty );
}

VG_API_CALL void VG_API_ENTRY vgScale(VGfloat sx, VGfloat sy) VG_API_EXIT {
	MKContext::instance().scale( sx, sy );
}

//VG_API_CALL void VG_API_ENTRY vgShear(VGfloat shx, VGfloat shy) VG_API_EXIT;

VG_API_CALL void VG_API_ENTRY vgRotate(VGfloat angle) VG_API_EXIT {
	MKContext::instance().rotate( angle );
}

VG_API_CALL void VG_API_ENTRY vgRotateXYZ(VGfloat angle, VGfloat x, VGfloat y, VGfloat z) VG_API_EXIT {
    MKContext::instance().rotate( angle , x , y , z );
}

VG_API_CALL float VG_API_ENTRY vgAngle() VG_API_EXIT {
   return MKContext::instance().angle();
}
