/*
 *  glPath.h
 *  MonkVG-OpenGL
 *
 *  Created by Micah Pearlman on 8/6/10.
 *  Copyright 2010 MP Engineering. All rights reserved.
 *
 */

#ifndef __glPath_h__
#define __glPath_h__

#include "mkPath.h"
#include "glPlatform.h"
#include <list>
#include <vector>
#include "glPaint.h"
#include "vec.hpp"
#include "math.hpp"
#include <Tess2/tesselator.h>

namespace MonkVG {
	
	class OpenGLPath : public IPath {
	public:
	
		OpenGLPath( VGint pathFormat, VGPathDatatype datatype, VGfloat scale, VGfloat bias, VGint segmentCapacityHint, VGint coordCapacityHint, VGbitfield capabilities ) 
			:	IPath( pathFormat, datatype, scale, bias, segmentCapacityHint, coordCapacityHint, capabilities )
			,	_fillTesseleator( 0 )
			,	_strokeVBO(-1)
			,	_fillVBO(-1)
			,	_fillPaintForPath( 0 )
			,	_strokePaintForPath( 0 )
		{

		}
		virtual ~OpenGLPath();
		
		
		virtual bool draw( VGbitfield paintModes );
		virtual void clear( VGbitfield caps );
		virtual void buildFillIfDirty();

	private:
		
        typedef vec2<GLfloat> v2_t;
        
		typedef vec3<GLdouble> v3_t;
        
		struct textured_vertex_t {
			GLfloat		v[2];
			GLfloat		uv[2];
		};
		
	private:
		
		TESStesselator*		_fillTesseleator;
		vector<GLfloat>		_vertices;
		vector<v2_t>		_strokeVertices;
		list<v3_t>			_tessVertices;
		GLenum				_primType;
		GLuint				_fillVBO;
		GLuint				_strokeVBO;
        int					_numberFillVertices;
		int					_numberStrokeVertices;
		OpenGLPaint*		_fillPaintForPath;
		OpenGLPaint*		_strokePaintForPath;
		
		
	private:		// tesseleator callbacks
		void endOfTesselation( VGbitfield paintModes );
		
	private:	// utility methods
		
		GLenum primType() {
			return _primType;
		}
		void setPrimType( GLenum t ) {
			_primType = t;
		}
		
		GLdouble* tessVerticesBackPtr() {
			return &(_tessVertices.back().x);
		} 
		
		void updateBounds(float x, float y) {
			_minX = std::min(_minX, x);
			_width = std::max(_width, x);
			_minY = std::min(_minY, y);
			_height = std::max(_height, y);
		}

		void addVertex( GLdouble* v ) {
			VGfloat x = (VGfloat)v[0];
			VGfloat y = (VGfloat)v[1];
			updateBounds(x, y);
			_vertices.push_back(x);
			_vertices.push_back(y);
		}

		GLdouble * addTessVertex( const v3_t & v ) {
			//updateBounds(v.x, v.y);
			_tessVertices.push_back( v );
			return tessVerticesBackPtr();
		}
		
		void buildFill();
		void buildStroke();
		void buildFatLineSegment( vector<v2_t>& vertices, const v2_t& p0, const v2_t& p1, const float stroke_width );
        
        // stroke styles
        void applyLineStyles( vector<v2_t>& vertices, VGCapStyle style, VGJoinStyle join, VGfloat miter, VGfloat stroke_width );
        size_t numberOfvertices( vector<v2_t>& vertices );
        
        int32_t e1;
        int32_t e2;
        int32_t e3;

        struct TriangleElement {
            TriangleElement(uint16_t a_, uint16_t b_, uint16_t c_) : a(a_), b(b_), c(c_) {}
            uint16_t a, b, c;
        };
        void addCurrentVertex(const Coordinate& currentVertex, float flip, double distance,
                              const vec2<double>& normal, float endLeft, float endRight, bool round,
                              int32_t startVertex, std::vector<TriangleElement>& triangleStore, vector<v2_t> &vertices);
        void addPieSliceVertex(const Coordinate& currentVertex, float flip, double distance,
                               const vec2<double>& extrude, bool lineTurnsLeft, int32_t startVertex,
                               std::vector<TriangleElement>& triangleStore, vector<v2_t> &vertices);
        size_t addVertix(vector<v2_t> &vertices, int8_t x, int8_t y, float ex, float ey, int8_t tx, int8_t ty, int32_t linesofar);
        
        /*
         * Scale the extrusion vector so that the normal length is this value.
         * Contains the "texture" normals (-1..1). This is distinct from the extrude
         * normals for line joins, because the x-value remains 0 for the texture
         * normal array, while the extrude normal actually moves the vertex to create
         * the acute/bevelled line join.
         */
        static const int8_t extrudeScale = 63;
	};

}

#endif // __glPath_h__
