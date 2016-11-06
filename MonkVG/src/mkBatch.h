//
//  mkBatch.h
//  MonkVG-iOS
//
//  Created by Micah Pearlman on 6/27/11.
//  Copyright 2011 Zero Vision. All rights reserved.
//
#ifndef __mkBatch_h__
#define __mkBatch_h__

#include "mkMath.h"
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
            MonkVG::Pos min;     // smallest x,y from p,q,r
            MonkVG::Pos max;     // biggest x,y from p,q,r
            
            // Data
            MonkVG::Pos p;       // Leftmost point
            MonkVG::Pos q;       // Counterclockwise next point
            MonkVG::Pos r;       // Clockwise next point
            MonkVG::Color color;       // Color being used
            
            inline triangle_t(const int& _id, const MonkVG::Pos& _min, const MonkVG::Pos& _max, const MonkVG::Pos& _p, const MonkVG::Pos& _q, const MonkVG::Pos& _r, const MonkVG::Color& _color) :
                id(_id), min(_min), max(_max), p(_p), q(_q), r(_r), color(_color)
            {
            }
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
        
        void addTriangle(int32_t v[6], const MonkVG::Color& color);
        void finalizeTriangleBatch();
        void reset();
    };
	
}

#endif // __mkBatch_h__
