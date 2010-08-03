/*
 *  mkSVG.h
 *  MonkSVG
 *
 *  Created by Micah Pearlman on 8/2/10.
 *  Copyright 2010 Zero Vision. All rights reserved.
 *
 */

#ifndef __mkSVG_h__
#define __mkSVG_h__

#include <string>
#include <vector>

namespace MonkSVG {
	using namespace std;
	class IDisplayCommand;
	
	class SVG  {
	public:
		
		bool initialize();
		bool read( string file_path );

	private:
		
		vector<IDisplayCommand*> _displayCommands;

	};
}

#endif // __mkSVG_h__