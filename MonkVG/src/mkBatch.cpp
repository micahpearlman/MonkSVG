//
//  mkBatch.cpp
//  MonkVG-iOS
//
//  Created by Micah Pearlman on 6/27/11.
//  Copyright 2011 Zero Vision. All rights reserved.
//

#include "mkBatch.h"
#include <set>
#include <unordered_map>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include <tess2/tesselator.h>


namespace MonkVG {
    struct vertexData_t {
        MonkVG::Pos pos;
        MonkVG::Color color;
        
        vertexData_t(MonkVG::Pos::value_type x, MonkVG::Pos::value_type y, MonkVG::Color _color) :
            pos(x, y),
            color(_color)
        {
        }
        bool operator==(const vertexData_t& other) const
        {
            return pos == other.pos && color == other.color;
        }
    };
    struct __attribute__((packed)) gpuVertexData_t {
        MonkVG::GpuPos pos;
        MonkVG::Color color;
    };
}
template <> struct std::hash<MonkVG::vertexData_t>
{
    template <typename T> constexpr static
    T rotate_left(T val, size_t len)
    {
        return (val << len) | ((unsigned) val >> (-len & (sizeof(T) * CHAR_BIT - 1)));
    }
    
    std::size_t operator()(const MonkVG::vertexData_t& key) const
    {
        return
            std::hash<MonkVG::Pos>()(key.pos) ^
            rotate_left(std::hash<MonkVG::Color>()(key.color), 1);
    }
};

#include <cstring> // for std::memcpy
#include <deque>
#include "mkMath.h"
#include "mkSVG.h"
#include "mkPaint.h"

namespace MonkVG {
    
    MKBatch::MKBatch(MonkSVG::MKSVGHandler* h) :
        _handler(h)
    ,   _vao(-1)
    ,	_vbo(-1)
    ,   _ebo(-1)
    ,   _batchMinX(0)
    ,   _batchMaxX(0)
    ,   _batchMinY(0)
    ,   _batchMaxY(0)
    ,   _b(new build_t)
    {
    }
    MKBatch::~MKBatch() {
        reset();
    }
    
    // Based on https://hal.archives-ouvertes.fr/inria-00072100/document
    static inline int32_t orientation(const int32_t a[2], const int32_t b[2], const int32_t c[2])
    {
        int32_t result = (a[0]-c[0]) * (b[1]-c[1]) - (a[1]-c[1]) * (b[0]-c[0]);
        return result;
    }
    static inline int intersectionTestVertex(const int32_t p1[2], const int32_t q1[2], const int32_t r1[2], const int32_t p2[2], const int32_t q2[2], const int32_t r2[2])
    {
        if (orientation(r2,p2,q1) >= 0)
            if (orientation(r2,q2,q1) <= 0)
                if (orientation(p1,p2,q1) > 0) return (orientation(p1,q2,q1) <= 0) ? 1 : false;
                else if (orientation(p1,p2,r1) < 0) return false;
                else return (orientation(q1,r1,p2) >= 0) ? 2 : false;
            else if (orientation(p1,q2,q1) > 0) return false;
            else if (orientation(r2,q2,r1) > 0) return false;
            else return (orientation(q1,r1,q2) >= 0) ? 3 : false;
        else if (orientation(r2,p2,r1) < 0) return false;
        else if (orientation(q1,r1,r2) >= 0) return (orientation(p1,p2,r1) >= 0) ? 4 : false;
        else if (orientation(q1,r1,q2) < 0) return false;
        else return (orientation(r2,r1,q2) >= 0) ? 5 : false;
    }
    static inline int intersectionTestEdge(const int32_t p1[2], const int32_t q1[2], const int32_t r1[2], const int32_t p2[2], const int32_t q2[2], const int32_t r2[2])
    {
        if (orientation(r2,p2,q1) >= 0)
            if (orientation(p1,p2,q1) >= 0) return (orientation(p1,q1,r2) >= 0) ? 1 : false;
            else if (orientation(q1,r1,p2) < 0) return false;
            else return (orientation(r1,p1,p2) >= 0) ? 2 : false;
        else if (orientation(r2,p2,r1) < 0) return false;
        else if (orientation(p1,p2,r1) < 0) return false;
        else if (orientation(p1,r1,r2) >= 0) return 3;
        else return (orientation(q1,r1,r2) >= 0) ? 4 : false;
    }
    static inline int intersection(const int32_t p1[2], const int32_t q1[2], const int32_t r1[2], const int32_t p2[2], const int32_t q2[2], const int32_t r2[2])
    {
        if ( orientation(p2,q2,p1) >= 0 )
            if ( orientation(q2,r2,p1) >= 0 )
                if ( orientation(r2,p2,p1) >= 0 ) return 1;
                else return intersectionTestEdge(p1,q1,r1,p2,q2,r2) << 1;
            else if ( orientation(r2,p2,p1) >= 0 ) return intersectionTestEdge(p1,q1,r1,r2,p2,q2) << 4;
            else return intersectionTestVertex(p1,q1,r1,p2,q2,r2) << 7;
        else if (orientation(q2,r2,p1) < 0) return intersectionTestVertex(p1,q1,r1,r2,p2,q2) << 10;
        else if (orientation(r2,p2,p1) >= 0) return intersectionTestEdge(p1,q1,r1,q2,r2,p2) << 13;
        else return intersectionTestVertex(p1,q1,r1,q2,r2,p2) << 16;
    };
    
    static const GLfloat precision = 0.01f;
    static const GLfloat precisionMult = 1.f/precision;
    std::map<int, int> stat;
    
    void MKBatch::addTriangle(int32_t v[6], const MonkVG::Color& color)
    {
        int32_t* p = &v[0];
        int32_t* q = &v[2];
        int32_t* r = &v[4];
  
        // Remove null triangles
        if ((p[0] == q[0] && p[0] == r[0]) || (p[1] == q[1] && p[1] == r[1]) || (p[0] == q[0] && p[1] == q[1]) || (p[0] == r[0] && p[1] == r[1]) || (q[0] == r[0] && q[1] == r[1]))
        {
            return;
        }
        
        // Put leftmost first
        if (q[0] < p[0] && q[0] <= r[0])
        {
            std::swap(p,q); // q to the left
        }
        else if (r[0] < p[0] && r[0] <= q[0])
        {
            std::swap(p,r); // q to the left
        }
        
        // Make sure triangles are counterclockwise
        if (orientation(p, q, r) < 0)
        {
            std::swap(q,r);
        }
        
        // Find box
        const int32_t xmin = p[0];
        const int32_t xmax = std::max(q[0], r[0]);
        const int32_t ymin = std::min(p[1], std::min(q[1], r[1]));
        const int32_t ymax = std::max(p[1], std::max(q[1], r[1]));
        
        _batchMinX = std::min(_batchMinX, (GLfloat)xmin/precisionMult);
        _batchMaxX = std::max(_batchMaxX, (GLfloat)xmax/precisionMult);
        _batchMinY = std::min(_batchMinY, (GLfloat)ymin/precisionMult);
        _batchMaxY = std::max(_batchMaxY, (GLfloat)ymax/precisionMult);

        /*
        bool eraseIter;
        for (auto iter = _b->trianglesByXMin.lower_bound(xmin - _b->maxSizeX + 1); iter != _b->trianglesByXMin.end();
             eraseIter ? iter = _b->trianglesByXMin.erase(iter) : ++iter)
        {
            eraseIter = false;
            
            auto& t1(*iter->second);
            if (t1.max[1] <= ymin)
            {
                continue;
            }
            if (t1.min[1] >= ymax)
            {
                continue;
            }
            if (t1.max[0] <= xmin)
            {
                continue;
            }
            if (t1.min[0] >= xmax)
            {
                break;
            }
            
            // We got an optimization contender!
            int which = intersection(p, q, r, t1.p, t1.q, t1.r);
            
            auto addedStat = stat.emplace({which, 1});
            if (!addedStat.second)
            {
                ++addedStat.first->second;
            }
            if (which != 0)
            {
                // Optimized! TODO: just chop it!
                t1.id = -1;         // Please don't use it anymore (signal)
                eraseIter = true;
                ++_b->numDeletedId;
            }
        }
        */
        
        // Add triangle to deque
        _b->trianglesDb.push_back(triangle_t(
                                  (int)_b->trianglesDb.size(), // id
                                  MonkVG::Pos(xmin, ymin), MonkVG::Pos(xmax, ymax), // min, max
                                  MonkVG::Pos(p[0], p[1]), MonkVG::Pos(q[0], q[1]), MonkVG::Pos(r[0], r[1]), // p,q,r
                                  color
                                  ));
        _b->trianglesToAdd.push_back(&_b->trianglesDb.back());
        _b->newMaxSizeX = std::max(_b->newMaxSizeX, xmax - xmin);
        _b->newMaxSizeY = std::max(_b->newMaxSizeY, ymax - ymin);
    }
    
    void MKBatch::finalizeTriangleBatch()
    {
        for (auto triangleToAdd : _b->trianglesToAdd)
        {
            _b->trianglesByXMin.insert({triangleToAdd->min[0], triangleToAdd});
        }
        _b->maxSizeX = _b->newMaxSizeX;
        _b->maxSizeY = _b->newMaxSizeY;
        _b->trianglesToAdd.clear();
    }
    
    void MKBatch::addPathVertexData( GLfloat* fillVerts, size_t fillVertCnt, GLfloat* strokeVerts, size_t strokeVertCnt, GLbitfield paintModes ) {
        
        // get the current transform
        const Matrix33& transform = _handler->_active_matrix;
        int32_t v[6];
        
        //printf("Adding %d fill %d stroke\n", (int)fillVertCnt, (int)strokeVertCnt);
        if ( paintModes & VG_FILL_PATH) {
            // get the paint color
            MKPaint* paint = _handler->getFillPaint();
            const float* fc = paint->getPaintColor();
            
            const MonkVG::Color color(
                                      GLuint(fc[0] * 255.0f),
                                      GLuint(fc[1] * 255.0f),
                                      GLuint(fc[2] * 255.0f),
                                      GLuint(fc[3] * 255.0f));
            
            // get vertices and transform them
            for ( int i = 0; i < (int)fillVertCnt * 2 - 4; i+=6 ) {
                v2_t affine;
                affine = affineTransform(transform, &fillVerts[i + 0]);
                v[0] = static_cast<int32_t>(affine[0]*precisionMult);
                v[1] = static_cast<int32_t>(affine[1]*precisionMult);
                affine = affineTransform(transform, &fillVerts[i + 2]);
                v[2] = static_cast<int32_t>(affine[0]*precisionMult);
                v[3] = static_cast<int32_t>(affine[1]*precisionMult);
                affine = affineTransform(transform, &fillVerts[i + 4]);
                v[4] = static_cast<int32_t>(affine[0]*precisionMult);
                v[5] = static_cast<int32_t>(affine[1]*precisionMult);
                
                addTriangle(v, color);
            }
            finalizeTriangleBatch();
        }
        
        if ( paintModes & VG_STROKE_PATH) {
            // get the paint color
            MKPaint* paint = _handler->getStrokePaint();
            const float* fc = paint->getPaintColor();
            
            const MonkVG::Color color(
                                      GLuint(fc[0] * 255.0f),
                                      GLuint(fc[1] * 255.0f),
                                      GLuint(fc[2] * 255.0f),
                                      GLuint(fc[3] * 255.0f));
            
            // get vertices and transform them
            int32_t* firstV = &v[0];    // Don't use a,b,c as these need to keep in order
            int32_t* secondV = &v[2];
            int32_t* thirdV = &v[4];

            int vertcnt = 0;
            for ( int i = 0; i < strokeVertCnt * 2; i+=2, vertcnt++ ) {
                v2_t affine = affineTransform(transform, &strokeVerts[i]) * precisionMult;

                // for stroke we need to convert from a strip to triangle
                switch ( vertcnt ) {
                    case 0:
                        firstV[0] = static_cast<int32_t>(affine[0]);
                        firstV[1] = static_cast<int32_t>(affine[1]);
                        break;
                    case 1:
                        secondV[0] = static_cast<int32_t>(affine[0]);
                        secondV[1] = static_cast<int32_t>(affine[1]);
                        break;
                    default:
                    {
                        thirdV[0] = static_cast<int32_t>(affine[0]);
                        thirdV[1] = static_cast<int32_t>(affine[1]);

                        // Next will override what was in firstV.
                        std::swap(firstV, thirdV);
                        std::swap(firstV, secondV);

                        addTriangle(v, color);
                        break;
                    }
                }
            }
            finalizeTriangleBatch();
        }
    }
    
    void MKBatch::finalize() {
        // Move triangles to vertexes
        int numVertices = (int)(_b->trianglesDb.size() - _b->numDeletedId) * 3;
        
        std::vector<GLuint> ebo;
        ebo.reserve(numVertices);
        
        std::vector<gpuVertexData_t> vbo;
        vbo.reserve(numVertices); // Note : numVertices is the maximum ever. It will ALWAYS be less than this.

        std::unordered_map<vertexData_t, size_t> vertexToId;
        
        GLfloat xSize = _batchMaxX - _batchMinX;
        GLfloat ySize = _batchMaxY - _batchMinY;
        
        auto addVertex = [&](int32_t x, int32_t y, MonkVG::Color color) -> GLuint
        {
            vertexData_t toAdd(x, y, color);
            auto found = vertexToId.find(toAdd);
            if (found == vertexToId.end())
            {
                auto id = vbo.size();
                GLushort xNorm = (GLushort)( ((GLfloat)x / precisionMult - _batchMinX) / xSize * 65535 );
                GLushort yNorm = 65535 - (GLushort)( ((GLfloat)y / precisionMult - _batchMinY) / ySize * 65535 );
                vbo.push_back({{xNorm, yNorm}, color});
                vertexToId.insert({toAdd, id});
                return (GLuint)id;
            }
            else
            {
                return (GLuint)found->second;
            }
        };
        
        for (auto iter : _b->trianglesDb)
        {
            if (iter.id == -1)
            {
                continue;
            }
            ebo.push_back(addVertex(iter.p[0], iter.p[1], iter.color));
            ebo.push_back(addVertex(iter.q[0], iter.q[1], iter.color));
            ebo.push_back(addVertex(iter.r[0], iter.r[1], iter.color));
        }
        _numEboIndices = (GLsizei)(ebo.size());
        
        printf("numVertices = %d, numVbo = %d, numEbo = %d\n", (int)numVertices, (int)vbo.size(), (int)ebo.size());
        
        glGenVertexArraysOES(1, &_vao);
        glBindVertexArrayOES(_vao);
        
        glGenBuffers(1, &_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(GL_ARRAY_BUFFER, vbo.size() * sizeof(gpuVertexData_t), &vbo[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_UNSIGNED_SHORT, GL_TRUE, sizeof(gpuVertexData_t), (GLvoid*)offsetof(gpuVertexData_t, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(gpuVertexData_t), (GLvoid*)offsetof(gpuVertexData_t, color));
      
        glGenBuffers(1, &_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, ebo.size() * sizeof(GLuint), &ebo[0], GL_STATIC_DRAW);
        
        glBindVertexArrayOES(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
        printf("\n");
        for (auto iter : stat)
        {
            printf("%5d : %d\n", iter.first, iter.second);
        }
        delete _b; _b=nullptr;
        cleanupDefaultAlloc();
    }
    
    void MKBatch::draw() {
        // get the native OpenGL context
        _handler->beginRender();

        glBindVertexArrayOES(_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        glDrawElements(GL_TRIANGLES, _numEboIndices, GL_UNSIGNED_INT, NULL);

        glBindVertexArrayOES(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
/*        GL->glDisable( GL_TEXTURE_2D );
        GL->glEnableClientState( GL_VERTEX_ARRAY );
        GL->glEnableClientState( GL_COLOR_ARRAY );
        GL->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
        
        // draw
        GL->glBindBuffer( GL_ARRAY_BUFFER, _vbo );
        GL->glVertexPointer( 2, GL_FLOAT, sizeof(vertex_t), (GLvoid*)offsetof(vertex_t, v) );
        GL->glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertex_t), (GLvoid*)offsetof(vertex_t, color) );
        GL->glDrawArrays( GL_TRIANGLES, 0, (GLsizei)_vertexCount );
        GL->glBindBuffer( GL_ARRAY_BUFFER, 0 );
*/       
        _handler->endRender();
    }
    
    void MKBatch::reset()
    {
        if ( _vao != -1 ) {
            glDeleteVertexArraysOES(1, &_vao);
            _vao = -1;
        }
        if ( _vbo != -1 ) {
            glDeleteBuffers( 1, &_vbo );
            _vbo = -1;
        }
        if ( _ebo != -1 ) {
            glDeleteBuffers( 1, &_ebo );
            _ebo = -1;
        }
        
        if (_b)
        {
            delete _b;
            _b = nullptr;
        }

    }
    
}

