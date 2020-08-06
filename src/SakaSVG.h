/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */

#ifndef SakaSVG_h
#define SakaSVG_h

#include "glMgr.h"

namespace tinyxml2
{
    class XMLDocument;
}

namespace Saka
{
    class SVG
    {
    public:
        SharedVAO vao;
        SharedEBO ebo;
        SharedProgram program;
        GLfloat batchMinX;
        GLfloat batchMaxX;
        GLfloat batchMinY;
        GLfloat batchMaxY;
        
        SVG(tinyxml2::XMLDocument& doc);
        ~SVG();
        
        void draw();
    };
}
#endif // SakaSVG_h
