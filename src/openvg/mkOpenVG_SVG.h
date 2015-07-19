/*
 *  svg.h
 *  MonkVG-Test-iOS-OpenGL
 *
 *  Created by Micah Pearlman on 8/12/10.
 *  Copyright 2010 Zero Vision. All rights reserved.
 *
 */

#ifndef __SVG_H__
#define __SVG_H__
#include <MonkVG/openvg.h>
#include <MonkVG/vgu.h>
#include <MonkVG/vgext.h>
#include <vector>
#include <list>
#include <cmath>
#include <memory>
#include <boost/make_shared.hpp>
#include "mkSVG.h"
#include "mkTransform2d.h"


using namespace std;

namespace MonkSVG {
	class OpenVG_SVGHandler : public ISVGHandler {
		
	public:
		
		typedef boost::shared_ptr<OpenVG_SVGHandler> SmartPtr;
		
		static ISVGHandler::SmartPtr create( ) {
			return boost::make_shared<OpenVG_SVGHandler>( );
		}

		
		OpenVG_SVGHandler();	// now public so a Plain Old Pointer can be used instead of the SmartPtr
		virtual ~OpenVG_SVGHandler();
		
		void draw();
        void dump( void **vertices, size_t *size );
		void optimize();
		
		const Transform2d& rootTransform() { return _root_transform; }
		void setRootTransform( const Transform2d& t ) { _root_transform = t; }
		
        const bool hasTransparentColors() { return _has_transparent_colors; }
		
	private:	
		
		//friend boost::shared_ptr<OpenVG_SVGHandler> std::make_shared<>();

		
	private:
		
		// groups
		virtual void onGroupBegin();
		virtual void onGroupEnd();
		
		// use
		virtual void onUseBegin();
		virtual void onUseEnd();

		
		// paths
		virtual void onPathBegin();
		virtual void onPathEnd();
		virtual void onPathClose();
		virtual void onPathMoveTo( float x, float y );
		virtual void onPathLineTo( float x, float y );
		virtual void onPathCubic( float x1, float y1, float x2, float y2, float x3, float y3 );
		virtual void onPathSCubic( float x2, float y2, float x3, float y3 );
		virtual void onPathHorizontalLine( float x );
		virtual void onPathVerticalLine( float y ); 
		virtual void onPathArc( float rx, float ry, float x_axis_rotation, int large_arc_flag, int sweep_flag, float x, float y );
		virtual void onPathRect( float x, float y, float w, float h );
        
        virtual void onPathQuad( float x1, float y1, float x2, float y2);
		

		// paint
		virtual void onPathFillColor( unsigned int color );
		virtual void onPathFillOpacity( float o );
		virtual void onPathFillRule( const string& rule );

		
		// stroke
		virtual void onPathStrokeColor( unsigned int color );
		virtual void onPathStrokeWidth( float width );
		virtual void onPathStrokeOpacity( float o );

		
		// transforms 
		virtual void onTransformTranslate( float x, float y );
		virtual void onTransformScale( float s );
		virtual void onTransformRotate( float r );
		virtual void onTransformMatrix( float a, float b, float c, float d, float e, float f );
		
		// misc
		virtual void onId( const std::string& id_ );
		
		uint32_t openVGRelative() {
			if ( relative() ) {
				return VG_RELATIVE;
			}
			return VG_ABSOLUTE;
		}
		
		
		void pushTransform( const Transform2d& t ) {
			Transform2d top_transform;
			if ( _transform_stack.size() == 0 ) {	// nothing on the stack so push the identity onto the stack
				_transform_stack.push_back( t );
			} else {
				const Transform2d& current_tranform = topTransform();
				//Transform2d::multiply( top_transform, t, current_tranform );
				Transform2d::multiply( top_transform, current_tranform, t );
				_transform_stack.push_back( top_transform );
				//vgLoadMatrix( top_transform.m );
			}
		}
		
		void popTransform() {
			_transform_stack.pop_back();
			//Transform2d& top = _transform_stack.back();
			//vgLoadMatrix( top.m );
		}
		
		const Transform2d& topTransform() {
			return _transform_stack.back();
		}
		
		
	private:

		
		struct path_object_t {
			VGPath		path;
			VGPaint		fill;
			VGFillRule	fill_rule;
			VGPaint		stroke;
			VGfloat		stroke_width;
			Transform2d transform;
			std::string id;
            VGfloat     stroke_miterlimit;
            VGint       stroke_linejoin;
            VGint       stroke_capStyle;
            
			path_object_t() : path( 0 ), fill( 0 ), stroke( 0 ), stroke_width( -1 ), fill_rule( VG_NON_ZERO ) {
				
			}
			virtual ~path_object_t() {
				vgDestroyPaint( fill );
				vgDestroyPaint( stroke );
				vgDestroyPath( path );
			}
		};
		
		struct group_t {
			group_t() 
			:	parent( 0 )
			,	current_path( 0 )
			,	fill( 0 ), stroke( 0 ), stroke_width( -1 ), fill_rule( VG_NON_ZERO )
			{
				
			}
			Transform2d			transform;
			group_t*			parent;
			list<group_t>		children;
			list<path_object_t> path_objects;
			path_object_t*		current_path;
			std::string			id;
			
			VGPaint		fill;
			VGFillRule	fill_rule;
			VGPaint		stroke;
			VGfloat		stroke_width;

		};
		
		group_t		_root_group;
		group_t*	_current_group;
		
		
		vector<Transform2d>		_transform_stack;
		Transform2d				_root_transform;
		Transform2d				_use_transform;
		VGfloat					_use_opacity;
		
		enum mode {
			kGroupParseMode = 1,
			kPathParseMode = 2,
			kUseParseMode = 3
		};
		
		mode _mode;
		
		VGPaint	_blackBackFill;		// if a path doesn't have a stroke or a fill then use this fill
		
		
		/// optimized batch monkvg batch object
		VGBatchMNK			_batch;
		
        // flag indicating if any of the fills or strokes in the image use transparent colors 
        // if there are no transparent colors in the image, blending can be disabled in open gl 
        // to improve rendering performance
        bool _has_transparent_colors;
		
	private:
		
		void draw_recursive( OpenVG_SVGHandler::group_t& group );
	};
	
}

#endif