#include <iostream>
#include <fstream>
#include <sstream>
#include <ostream>

#include "../../src/mkSVG.h"
using namespace std;

class SVGHandler : public MonkSVG::ISVGHandler {
	virtual void onPathBegin() { cout << "path begin" << endl; }
	virtual void onPathEnd() { cout << "path end" << endl; }
	virtual void onPathMoveTo( float x, float y ) { cout << "\t move to: " << x << ", " << y << endl; }
	virtual void onPathLineTo( float x, float y ) { cout << "\t line to: " << x << ", " << y << endl; }
	virtual void onPathCubic( float x1, float y1, float x2, float y2, float x3, float y3 ) { cout << "\t cubic: " << x1 << ", " << y1 << " | " << x2 << ", " << y2 << " | " << x3 << ", " << y3 << endl; }
};

int main (int argc, char * const argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
	MonkSVG::SVG svg_parser;
	SVGHandler handler;
	svg_parser.initialize( &handler );

	fstream file( argv[1] );
	if ( file.is_open() ) {
		std::string line;
		std::string buf;
		while( std::getline( file, line) )
			buf += line;
		std::cout << "read: " << buf << "\n";
		svg_parser.read( buf );
	}
	
    return 0;
}
