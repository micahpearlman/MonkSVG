//
//  mkBatch.h
//  MonkVG-iOS
//
//  Created by Micah Pearlman on 6/27/11.
//  Copyright 2011 Zero Vision. All rights reserved.
//
#ifndef __mkBatch_h__
#define __mkBatch_h__

#include <stdlib.h>
#include "mkBaseObject.h"

#include "glPlatform.h"

#include <map>
#include <sqlite3.h>

namespace MonkVG {
	
	class IBatch : public BaseObject {
	public:
        IBatch();
        virtual ~IBatch();
        
		inline BaseObject::Type getType() const {
			return BaseObject::kBatchType;
		}
		
		//// parameter accessors/mutators ////
		virtual VGint getParameteri( const VGint p ) const;
		virtual VGfloat getParameterf( const VGint f ) const;
		virtual void getParameterfv( const VGint p, VGfloat *fv ) const;
		virtual void setParameter( const VGint p, const VGfloat f );
		virtual void setParameter( const VGint p, const VGint i );
		virtual void setParameter( const VGint p, const VGfloat* fv, const VGint cnt );
		
        virtual void draw();
        virtual void dump( void **vertices, size_t *size );
        virtual void finalize();
        
        void addPathVertexData( GLfloat* fillVerts, size_t fillVertCnt, GLfloat* strokeVerts, size_t strokeVertCnt, VGbitfield paintModes );
        
        struct vertex_t {
            GLfloat		v[2];
            GLuint		color;
        };
    private:
        
        std::map<GLuint, int>   _colorMap;
        size_t					_vertexCount;
        GLuint					_vbo;
        
        sqlite3*                _verticesdb;
        sqlite3_stmt*           _getPotentialTriangles;
        sqlite3_stmt*           _insertTriangle;
        sqlite3_stmt*           _deleteTriangle;
        sqlite3_stmt*           _getNumTriangles;
        sqlite3_stmt*           _getAllTriangles;
	};
	
}

#endif // __mkBatch_h__
