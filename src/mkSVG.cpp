/*
 *  mkSVG.cpp
 *  MonkSVG
 *
 *  Created by Micah Pearlman on 8/2/10.
 *  Copyright 2010 Zero Vision. All rights reserved.
 *
 */

#include "mkSVG.h"
#include "tinyxml.h"
#include "VG/openvg.h"
#include "VG/vgu.h"

namespace MonkSVG {
	
	
	enum CommandType {
		kCmdTransform,
		kCmdStyle,
		kCmdPath
	};
	
	class IDisplayCommand {
	public:
		virtual void execute() = 0;
		virtual CommandType type() = 0;
	protected:
		
		
		IDisplayCommand()
		{}
		virtual ~IDisplayCommand()
		{}
	};
	class CommandTransform : public IDisplayCommand {
	public:
		CommandTransform( ) : IDisplayCommand() 
		{
			_translate[0] = _translate[1] = 0;
		}
		virtual void execute() {
			vgTranslate( _translate[0], _translate[1] );
		}
		virtual CommandType type() { return kCmdTransform; }
		
		void setTranslate( float x, float y ) {
			_translate[0] = x;
			_translate[1] = y;
		}
		float* translate() {
			return _translate;
		}
		
		void parse( string transform ) {
			
		}
		
	private:
		float _translate[2];
		
	};
	
	
	bool SVG::initialize() {
		
	}
	
	bool SVG::read( string file_path ) {
		TiXmlDocument doc;
		
		
		// load the main swfmill file
		
		if( doc.LoadFile( file_path ) ) {
			const TiXmlElement* root = doc.FirstChild( "svg" )->ToElement();
			const TiXmlElement* g = root->FirstChildElement( "g" );
			
		} else {
			return false;
		}
		
		return true;
	}	

};