/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */

#include "SakaSVG.h"
#include "mk/mkSVG.h"
#include <fstream>
#include "mk/mkMath.h"
#include "glm/glm/glm/gtx/transform.hpp"
#include "tinyxml2/tinyxml2.h"

namespace Saka
{
    static SharedProgram makeDefaultProgram()
    {
        SharedProgram p = SharedProgram::make_shared();
        SharedShader v = SharedShader(new VertexShader);
        v->compile("attribute lowp vec2 position;"
                   "attribute lowp vec2 quad;"
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
    
    SVG::SVG(tinyxml2::XMLDocument& doc) :
        program(defaultSVGProgram()),
        batchMinX(0),
        batchMaxX(0),
        batchMinY(0),
        batchMaxY(0)
    {
        // run it through the svg parser
        MonkSVG::MKSVGHandler handler;
        {
            MonkSVG::SVG svg_parser;
            svg_parser.initialize( &handler );
            svg_parser.read( doc );
        }
        
        handler.finalize(this);
    }
    
    SVG::~SVG()
    {
    }
    
    void SVG::draw()
    {
        glm::mat4 m = glm::translate(glm::scale(glm::vec3(1/300.f, 1/300.f, 0.f)), glm::vec3(-300.f, -300.f, 0.f));
        
        GLMgr::instance().bind(vao).bind(ebo).bind(program);
        glUniformMatrix4fv(0, 1, GL_FALSE,  glm::value_ptr(m));
        ebo->draw();
    }
}
