/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */

#include "SakaSVG.h"
#include "mkSVG.h"
#include <fstream>
#include <OpenGLES/ES2/glext.h>

SakaSVG::SakaSVG(const char* const pathname) :
    vao(-1),
    vbo(-1),
    ebo(-1),
    numEboIndices(0),
    batchMinX(0),
    batchMaxX(0),
    batchMinY(0),
    batchMaxY(0)
{
    char* buffer;
    {
        std::fstream is( pathname, std::fstream::in );
        
        // get length of file:
        is.seekg (0, std::ios::end);
        std::fpos_t length = is.tellg();
        is.seekg (0, std::ios::beg);
        
        // allocate memory:
        buffer = new char[length];
        
        // read data as a block:
        is.read(buffer,(std::streamsize)length);
    }

    // run it through the svg parser
    MonkSVG::MKSVGHandler handler;
    {
        MonkSVG::SVG svg_parser;
        svg_parser.initialize( &handler );
        svg_parser.read( buffer );
    }
    delete [] buffer;
    
    handler.finalize(this);
}

SakaSVG::~SakaSVG()
{
    if ( vao != -1 ) {
        glDeleteVertexArraysOES(1, &vao);
        vao = -1;
    }
    if ( vbo != -1 ) {
        glDeleteBuffers( 1, &vbo );
        vbo = -1;
    }
    if ( ebo != -1 ) {
        glDeleteBuffers( 1, &ebo );
        ebo = -1;
    }
}

void SakaSVG::draw() {
    glBindVertexArrayOES(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glDrawElements(GL_TRIANGLES, numEboIndices, GL_UNSIGNED_INT, NULL);

    glBindVertexArrayOES(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
