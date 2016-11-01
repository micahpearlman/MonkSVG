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
#include <OpenGLES/ES2/gl.h>
#include "vgCompat.h"
#include <cpp_btree/btree_map.h>
#include <map>
#include <deque>

namespace MonkSVG {
    class MKSVGHandler;
}

namespace MonkVG {
	class MKBatch {
	public:
        MKBatch(MonkSVG::MKSVGHandler* handler);
        ~MKBatch();
        
        void draw();
        void finalize();
        
        void addPathVertexData( GLfloat* fillVerts, size_t fillVertCnt, GLfloat* strokeVerts, size_t strokeVertCnt, GLbitfield paintModes );
        
    private:
        MonkSVG::MKSVGHandler* _handler;
        
        GLuint _vao;
        GLuint _vbo;
        GLuint _ebo;
        GLsizei _numEboIndices;
        GLfloat _batchMinX;
        GLfloat _batchMaxX;
        GLfloat _batchMinY;
        GLfloat _batchMaxY;

        struct triangle_t {
            // Indexes & helpers
            int id;             // incremental ID, playback order (-1 if it got deleted)
            int32_t min[2];     // smallest x,y from p,q,r
            int32_t max[2];     // biggest x,y from p,q,r
            
            // Data
            int32_t p[2];       // Leftmost point
            int32_t q[2];       // Counterclockwise next point
            int32_t r[2];       // Clockwise next point
            GLuint color;       // Color being used
        };

        struct build_t
        {
            std::deque<triangle_t> trianglesDb;
            btree::btree_multimap<int32_t, triangle_t*> trianglesByXMin;
            std::deque<triangle_t*> trianglesToAdd;
            int32_t                 maxSizeX, maxSizeY, newMaxSizeX, newMaxSizeY;
            int                     numDeletedId;
            
            build_t() :
                maxSizeX(0), maxSizeY(0), newMaxSizeX(0), newMaxSizeY(0),
                numDeletedId(0)
            {
            }
        }* _b;
        
        void addTriangle(int32_t v[6], GLuint color);
        void finalizeTriangleBatch();
        void reset();
    };
	
}

#endif // __mkBatch_h__
