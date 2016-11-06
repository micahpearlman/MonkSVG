/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */

#ifndef SakaSVG_h
#define SakaSVG_h

#include <OpenGLES/ES2/gl.h>

class SakaSVG
{
public:
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLsizei numEboIndices;
    GLfloat batchMinX;
    GLfloat batchMaxX;
    GLfloat batchMinY;
    GLfloat batchMaxY;

    SakaSVG(const char* const pathname);
    ~SakaSVG();
    
    void draw();
};

#endif // SakaSVG_h
