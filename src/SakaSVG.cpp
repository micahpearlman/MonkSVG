/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */

#include "SakaSVG.h"
#include "mk/mkSVG.h"
#include <fstream>
#include "mk/mkMath.h"
#include <glm/gtx/transform.hpp>

namespace Saka
{
    static SharedProgram makeDefaultProgram()
    {
        SharedProgram p = SharedProgram::make_shared();
        SharedShader v = SharedShader(new VertexShader);
        v->compile("attribute lowp vec2 position;"
                   "attribute lowp vec4 color;"
                   "uniform highp mat4 projection;"
                   "varying lowp vec4 exColor;"
                   "void main()"
                   "{"
                   "gl_Position = projection * vec4(position, 0.0, 1.0);"
                   "exColor = color;"
                   "}");
        p->attachShader(v);
        SharedShader f = SharedShader(new FragmentShader);
        f->compile(
                   "varying lowp vec4 exColor;"
                   "void main()"
                   "{"
                   "gl_FragColor = exColor;"
                   "}");
        p->attachShader(f);
        p->bindAttrib("position");
        p->bindAttrib("color");
        p->linkProgram();
        return p;
    }
    static const SharedProgram& defaultSVGProgram()
    {
        static SharedProgram instance = makeDefaultProgram();
        return instance;
    };
    
    SVG::SVG(const char* const pathname) :
    program(defaultSVGProgram()),
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
            assert(length <= UINT_MAX && length > 0);
            
            // allocate memory:
            buffer = new char[(unsigned int)length + 1];
            
            // read data as a block:
            is.read(buffer,(std::streamsize)length);
            
            buffer[(unsigned int)length] = 0;
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
    
    SVG::~SVG()
    {
    }
    
    void SVG::draw()
    {
        glm::mat4 m = glm::translate(glm::scale(glm::vec3(2.f, 2.f, 0.f)), glm::vec3(-.5f, -.5f, 0.f));
        
        GLMgr::instance().bind(vao).bind(ebo).bind(program);
        glUniformMatrix4fv(0, 1, GL_FALSE,  glm::value_ptr(m));
        ebo->draw();
    }
}
