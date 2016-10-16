//
//  mkBatch.cpp
//  MonkVG-iOS
//
//  Created by Micah Pearlman on 6/27/11.
//  Copyright 2011 Zero Vision. All rights reserved.
//

#include "mkBatch.h"
#include "mkContext.h"

namespace MonkVG {	// Internal Implementation
	VGint IBatch::getParameteri( const VGint p ) const {
		switch (p) {
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				return -1;	//todo: set error
				break;
		}
	}
	
	VGfloat IBatch::getParameterf( const VGint p ) const {
		switch (p) {
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				return -1;	//todo: set error
				break;
		}
	}
	
	void IBatch::getParameterfv( const VGint p, VGfloat *fv ) const {
		switch (p) {
				
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				break;
		}
		
	}
	
	void IBatch::setParameter( const VGint p, const VGint v ) {
		switch (p) {
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				break;
		}
	}
	
	void IBatch::setParameter( const VGint p, const VGfloat v ) 
	{
		switch (p) {
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				break;
		}
	}
	
	void IBatch::setParameter( const VGint p, const VGfloat* fv, const VGint cnt ) {
		switch (p) {
				
			default:
				SetError( VG_ILLEGAL_ARGUMENT_ERROR );
				break;
		}
	}
	
	
	
}


///// OpenVG API Implementation /////

using namespace MonkVG;

VG_API_CALL VGBatchMNK VG_API_ENTRY vgCreateBatchMNK() VG_API_EXIT {
	return (VGBatchMNK)IContext::instance().createBatch();
}

VG_API_CALL void VG_API_ENTRY vgDestroyBatchMNK( VGBatchMNK batch ) VG_API_EXIT {
	IContext::instance().destroyBatch( (IBatch*)batch );
}
VG_API_CALL void VG_API_ENTRY vgBeginBatchMNK( VGBatchMNK batch ) VG_API_EXIT {
	IContext::instance().startBatch( (IBatch*)batch );	
}
VG_API_CALL void VG_API_ENTRY vgEndBatchMNK( VGBatchMNK batch ) VG_API_EXIT {
	IContext::instance().endBatch( (IBatch*)batch );	
}
VG_API_CALL void VG_API_ENTRY vgDrawBatchMNK( VGBatchMNK batch ) VG_API_EXIT {
	((IBatch*)batch)->draw();
}
VG_API_CALL void VG_API_ENTRY vgDumpBatchMNK( VGBatchMNK batch, void **vertices, size_t * size ) VG_API_EXIT {
    IContext::instance().dumpBatch( (IBatch *)batch, vertices, size );
}


#include <cstring> // for std::memcpy
#include <deque>
#include "mkContext.h"

namespace MonkVG {
    
    IBatch::IBatch() :
        BaseObject()
    ,	_vbo(-1)
    ,	_vertexCount(0)
    ,   _verticesdb(nullptr)
    {
        int rc = sqlite3_open(":memory:", &_verticesdb);
        assert(!rc);
        static const char* const createTable =
        "CREATE TEMPORARY TABLE triangles ( "
        " id integer PRIMARY KEY AUTOINCREMENT NOT NULL,"
        " xmin integer NOT NULL,"
        " ymin integer NOT NULL,"
        " xmax integer NOT NULL,"
        " ymax integer NOT NULL,"
        " x0 integer NOT NULL,"
        " y0 integer NOT NULL,"
        " x1 integer NOT NULL,"
        " y1 integer NOT NULL,"
        " x2 integer NOT NULL,"
        " y2 integer NOT NULL,"
        " color integer NOT NULL"
        ");";
        static const char* const createIndex =
        "CREATE INDEX ix ON triangles (xmin,ymin,xmax,ymax);";
        
        char* errmsg;
        rc = sqlite3_exec(_verticesdb, createTable, nullptr, nullptr, &errmsg);
        assert(!rc);
        rc = sqlite3_exec(_verticesdb, createIndex, nullptr, nullptr, &errmsg);
        assert(!rc);
        
        // <x, <y, >x, >y
        static const char* const getPotentialTriangles =
        "SELECT id,x0,y0,x1,y1,x2,y2 FROM triangles WHERE xmin <= ?3 AND xmax >= ?1 AND ymin <= ?4 AND ymax >= ?2;";
        rc = sqlite3_prepare_v2(_verticesdb, getPotentialTriangles, -1, &_getPotentialTriangles, nullptr);
        assert(!rc);
        
        static const char* const insertTriangle =
        "INSERT INTO triangles(xmin,ymin,xmax,ymax,x0,y0,x1,y1,x2,y2,color) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        rc = sqlite3_prepare_v2(_verticesdb, insertTriangle, -1, &_insertTriangle, nullptr);
        assert(!rc);
        
        static const char* const deleteTriangle =
        "DELETE FROM triangles WHERE id = ?;";
        rc = sqlite3_prepare_v2(_verticesdb, deleteTriangle, -1, &_deleteTriangle, nullptr);
        assert(!rc);
        
        static const char* const getNumTriangles =
        "SELECT COUNT(*) FROM triangles;";
        rc = sqlite3_prepare_v2(_verticesdb, getNumTriangles, -1, &_getNumTriangles, nullptr);
        assert(!rc);
        
        static const char* const getAllTriangles =
        "SELECT x0,y0,x1,y1,x2,y2,color FROM triangles;";
        rc = sqlite3_prepare_v2(_verticesdb, getAllTriangles, -1, &_getAllTriangles, nullptr);
        assert(!rc);
    }
    IBatch::~IBatch() {
        if ( _vbo != -1 ) {
            GL->glDeleteBuffers( 1, &_vbo );
            _vbo = -1;
        }
        
        if (_verticesdb)
        {
            sqlite3_finalize(_getPotentialTriangles);
            sqlite3_finalize(_insertTriangle);
            sqlite3_close(_verticesdb);
        }
    }
    
    // Based on https://hal.archives-ouvertes.fr/inria-00072100/document
    static inline float orientation(const int32_t* a, const int32_t* b, const int32_t* c)
    {
        return (a[0]-c[0]) * (b[1]-c[1]) - (a[1]-c[1]) * (b[0]-c[0]);
    }
    static inline int intersectionTestVertex(const int32_t* p1, const int32_t* q1, const int32_t* r1, const int32_t* p2, const int32_t* q2, const int32_t* r2)
    {
        if (orientation(r2,p2,q1) >= 0)
            if (orientation(r2,q2,q1) <= 0)
                if (orientation(p1,p2,q1) > 0) return (orientation(p1,q2,q1) <= 0) ? 1 : false;
                else if (orientation(p1,p2,r1) < 0) return false;
                else return (orientation(q1,r1,p2) >= 0) ? 2 : false;
                else if (orientation(p1,q2,q1) > 0) return false;
                else if (orientation(r2,q2,r1) <= 0) return false;
                else return (orientation(q1,r1,q2) >= 0) ? 3 : false;
                else if (orientation(r2,p2,r1) < 0) return false;
                else if (orientation(q1,r1,r2) >= 0) return (orientation(p1,p2,r1) >= 0) ? 4 : false;
                else if (orientation(q1,r1,q2) < 0) return false;
                else return (orientation(r2,r1,q2) >= 0) ? 5 : false;
    }
    static inline int intersectionTestEdge(const int32_t* p1, const int32_t* q1, const int32_t* r1, const int32_t* p2, const int32_t* q2, const int32_t* r2)
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
    static inline int intersection(const int32_t* p1, const int32_t* q1, const int32_t* r1, const int32_t* p2, const int32_t* q2, const int32_t* r2)
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
    void IBatch::addPathVertexData( GLfloat* fillVerts, size_t fillVertCnt, GLfloat* strokeVerts, size_t strokeVertCnt, VGbitfield paintModes ) {
        
        // get the current transform
        Matrix33& transform = *IContext::instance().getActiveMatrix();
        
        if ( paintModes & VG_FILL_PATH) {
            // get the paint color
            IPaint* paint = IContext::instance().getFillPaint();
            const VGfloat* fc = paint->getPaintColor();
            
            const auto colorId =
            _colorMap.emplace(( uint32_t(fc[3] * 255.0f) << 24 )	// a
                              |	( uint32_t(fc[2] * 255.0f) << 16 )	// b
                              |	( uint32_t(fc[1] * 255.0f) << 8 )	// g
                              |	( uint32_t(fc[0] * 255.0f) << 0 ),	// r
                              _colorMap.size()).first->second;      // Returns either just added size() or stored value
            
            // get vertices and transform them
            int32_t v[6];
            int32_t* a;
            int32_t* b;
            int32_t* c;
            for ( int i = 0; i < fillVertCnt * 2 - 4; i+=6 ) {
                GLfloat affine[2];
                affineTransform(affine, transform, &fillVerts[i + 0] );
                v[0] = static_cast<int32_t>(affine[0]*precisionMult);
                v[1] = static_cast<int32_t>(affine[1]*precisionMult);
                affineTransform(affine, transform, &fillVerts[i + 2] );
                v[2] = static_cast<int32_t>(affine[0]*precisionMult);
                v[3] = static_cast<int32_t>(affine[1]*precisionMult);
                affineTransform(affine, transform, &fillVerts[i + 4] );
                v[4] = static_cast<int32_t>(affine[0]*precisionMult);
                v[5] = static_cast<int32_t>(affine[1]*precisionMult);
                
                a = &v[0];
                b = &v[2];
                c = &v[4];
                
                // Remove null triangles
                if ((a[0] == b[0] && a[0] == c[0]) || (a[1] == b[1] && a[1] == c[1]))
                {
                    continue;
                }
                
                // Put leftmost first
                if (b[0] < a[0] && b[0] <= c[0])
                {
                    std::swap(a,b); // b to the left
                }
                else if (c[0] < a[0] && c[0] <= b[0])
                {
                    std::swap(a,c); // c to the left
                }
                
                // Make sure triangles are counterclockwise
                if (orientation(a, b, c) < 0.f)
                {
                    std::swap(b,c);
                }
                
                int rc;
                // Find box
                const VGfloat xmin = a[0];
                const VGfloat xmax = std::max(b[0], c[0]);
                const VGfloat ymin = std::min(a[1], std::min(b[1], c[1]));
                const VGfloat ymax = std::max(a[1], std::max(b[1], c[1]));
                
                /*                rc = sqlite3_bind_int(_getPotentialTriangles, 1, xmin);
                 assert(!rc);
                 rc = sqlite3_bind_int(_getPotentialTriangles, 2, ymin);
                 assert(!rc);
                 rc = sqlite3_bind_int(_getPotentialTriangles, 3, xmax);
                 assert(!rc);
                 rc = sqlite3_bind_int(_getPotentialTriangles, 4, ymax);
                 assert(!rc);
                 
                 // Get all potential triangles
                 std::deque<int> toDelete;
                 while(SQLITE_ROW == (rc = sqlite3_step(_getPotentialTriangles)))
                 {
                 const int id = sqlite3_column_int(_getPotentialTriangles, 0);
                 const int32_t d[2] = { sqlite3_column_int(_getPotentialTriangles, 1), sqlite3_column_int(_getPotentialTriangles, 2)};
                 const int32_t e[2] = { sqlite3_column_int(_getPotentialTriangles, 3), sqlite3_column_int(_getPotentialTriangles, 4)};
                 const int32_t f[2] = { sqlite3_column_int(_getPotentialTriangles, 5), sqlite3_column_int(_getPotentialTriangles, 6)};
                 
                 if (1 == intersection(a, b, c, d, e, f))
                 {
                 toDelete.push_back(id);
                 }
                 }
                 
                 for (auto idToDelete : toDelete)
                 {
                 rc = sqlite3_bind_int(_deleteTriangle, 1, idToDelete);
                 assert(!rc);
                 rc = sqlite3_step(_deleteTriangle);
                 assert(rc == SQLITE_DONE);
                 sqlite3_reset(_deleteTriangle);
                 }
                 */
                rc = sqlite3_bind_int(_insertTriangle, 1, xmin);
                assert(!rc);
                rc = sqlite3_bind_int(_insertTriangle, 2, ymin);
                assert(!rc);
                rc = sqlite3_bind_int(_insertTriangle, 3, xmax);
                assert(!rc);
                rc = sqlite3_bind_int(_insertTriangle, 4, ymax);
                assert(!rc);
                rc = sqlite3_bind_int(_insertTriangle, 5, a[0]);
                assert(!rc);
                rc = sqlite3_bind_int(_insertTriangle, 6, a[1]);
                assert(!rc);
                rc = sqlite3_bind_int(_insertTriangle, 7, b[0]);
                assert(!rc);
                rc = sqlite3_bind_int(_insertTriangle, 8, b[1]);
                assert(!rc);
                rc = sqlite3_bind_int(_insertTriangle, 9, c[0]);
                assert(!rc);
                rc = sqlite3_bind_int(_insertTriangle, 10, c[1]);
                assert(!rc);
                rc = sqlite3_bind_int(_insertTriangle, 11, colorId);
                assert(!rc);
                rc = sqlite3_step(_insertTriangle);
                assert(rc == SQLITE_DONE);
                
                sqlite3_reset(_getPotentialTriangles);
                sqlite3_reset(_insertTriangle);
            }
        }
        
        /*		if ( paintModes & VG_STROKE_PATH) {
         vertex_t vert, startVertex, lastVertex;
         
         // get the paint color
         IPaint* paint = IContext::instance().getStrokePaint();
         const VGfloat* fc = paint->getPaintColor();
         
         vert.color =	( uint32_t(fc[3] * 255.0f) << 24 )	// a
         |	( uint32_t(fc[2] * 255.0f) << 16 )	// b
         |	( uint32_t(fc[1] * 255.0f) << 8 )	// g
         |	( uint32_t(fc[0] * 255.0f) << 0 );	// r
         
         // get vertices and transform them
         VGfloat v[2];
         int vertcnt = 0;
         for ( int i = 0; i < strokeVertCnt * 2; i+=2, vertcnt++ ) {
         v[0] = strokeVerts[i];
         v[1] = strokeVerts[i + 1];
         affineTransform( vert.v, transform, v );
         
         // for stroke we need to convert from a strip to triangle
         switch ( vertcnt ) {
         case 0:
         _vertices.push_back( vert );
         break;
         case 1:
         startVertex = vert;
         _vertices.push_back( vert );
         break;
         case 2:
         lastVertex = vert;
         _vertices.push_back( vert );
         break;
         
         default:
         _vertices.push_back( startVertex );
         _vertices.push_back( lastVertex );
         _vertices.push_back( vert );
         startVertex = lastVertex;
         lastVertex = vert;
         break;
         }
         }
         } */
        
        
        
    }
    
    void IBatch::finalize() {
        // build the vbo
        if ( _vbo != -1 ) {
            glDeleteBuffers( 1, &_vbo );
            _vbo = -1;
        }
        
        // Move color map to color vector
        std::vector<uint32_t> colors(_colorMap.size());
        for (auto color : _colorMap)
        {
            colors[color.second] = color.first;
        }
        
        std::vector<vertex_t> vertices;
        int rc = sqlite3_step(_getNumTriangles);
        int numTriangles = sqlite3_column_int(_getNumTriangles, 0);
        vertices.reserve(numTriangles * 3);
        sqlite3_reset(_getNumTriangles);
        
        while(SQLITE_ROW == (rc = sqlite3_step(_getAllTriangles)))
        {
            const int colorId = sqlite3_column_int(_getAllTriangles, 6);
            const auto color = colors[colorId];
            vertices.push_back({{static_cast<VGfloat>(sqlite3_column_int(_getAllTriangles, 0))/precisionMult, static_cast<VGfloat>(sqlite3_column_int(_getAllTriangles, 1))/precisionMult}, color});
            vertices.push_back({{static_cast<VGfloat>(sqlite3_column_int(_getAllTriangles, 2))/precisionMult, static_cast<VGfloat>(sqlite3_column_int(_getAllTriangles, 3))/precisionMult}, color});
            vertices.push_back({{static_cast<VGfloat>(sqlite3_column_int(_getAllTriangles, 4))/precisionMult, static_cast<VGfloat>(sqlite3_column_int(_getAllTriangles, 5))/precisionMult}, color});
        }
        
        GL->glGenBuffers( 1, &_vbo );
        GL->glBindBuffer( GL_ARRAY_BUFFER, _vbo );
        GL->glBufferData( GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_t), &vertices[0], GL_STATIC_DRAW );
        _vertexCount = vertices.size();
        
        if (_verticesdb)
        {
            sqlite3_close(_verticesdb);
            _verticesdb = nullptr;
        }
    }
    
    void IBatch::dump( void **vertices, size_t *size ) {
        
        //*size = _vertices.size() * sizeof( vertex_t );
        //*vertices = malloc( *size );
        
        //std::memcpy( *vertices, &_vertices[0], *size );
        
    }
    
    void IBatch::draw() {
        // get the native OpenGL context
        IContext& glContext = (MonkVG::IContext&)IContext::instance();
        glContext.beginRender();
        
        GL->glDisable( GL_TEXTURE_2D );
        GL->glEnableClientState( GL_VERTEX_ARRAY );
        GL->glEnableClientState( GL_COLOR_ARRAY );
        GL->glDisableClientState( GL_TEXTURE_COORD_ARRAY );
        
        // draw
        GL->glBindBuffer( GL_ARRAY_BUFFER, _vbo );
        GL->glVertexPointer( 2, GL_FLOAT, sizeof(vertex_t), (GLvoid*)offsetof(vertex_t, v) );
        GL->glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(vertex_t), (GLvoid*)offsetof(vertex_t, color) );
        GL->glDrawArrays( GL_TRIANGLES, 0, (GLsizei)_vertexCount );
        GL->glBindBuffer( GL_ARRAY_BUFFER, 0 );
        
        glContext.endRender();
    }
    
}

