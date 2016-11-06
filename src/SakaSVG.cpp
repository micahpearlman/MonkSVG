/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */

#include "SakaSVG.h"
#include "mkSVG.h"
#include <fstream>
#include <OpenGLES/ES2/glext.h>

namespace Saka
{
    VAO::VAO() :
    vao(makeVao())
    {}
    VAO::~VAO()
    {
        glDeleteVertexArraysOES(1, &vao);
    }
    GLuint VAO::makeVao()
    {
        GLuint vao;
        glGenVertexArraysOES(1, &vao);
        return vao;
    }
    void VAO::bind()
    {
        glBindVertexArrayOES(vao);
    }
    void VAO::unbind()
    {
        assert(GLMgr::instance().getVao() == this);
        glBindVertexArrayOES(0);
    }
    void VAO::addVbo(const SharedVBO& vbo)
    {
        assert(GLMgr::instance().getVao() == this);
        vbos.push_back(vbo);
        vbo->setupAttribs();
    }

    VBO::VBO() :
    vbo(makeVbo())
    {
    }
    
    VBO::~VBO()
    {
        glDeleteBuffers( 1, &vbo );
    }
    GLuint VBO::makeVbo()
    {
        GLuint vbo;
        glGenBuffers(1, &vbo);
        return vbo;
    }
    void VBO::bind()
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
    }
    void VBO::unbind()
    {
        assert(GLMgr::instance().getVbo() == this);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void VBO::bufferData(GLsizeiptr size, const GLvoid* data, GLenum usage)
    {
        assert(GLMgr::instance().getVbo() == this);
        glBufferData(GL_ARRAY_BUFFER, size, data, usage);
    }
    void VBO::setupAttribs()
    {
        GLuint index = 0;
        for (const auto& attrib : attribs)
        {
            glEnableVertexAttribArray(index);
            glVertexAttribPointer(index, attrib.size, attrib.type, attrib.normalized, attrib.stride, attrib.ptr);
            ++index;
        }
    }
    
    EBO::EBO() :
    ebo(makeEbo()),
    beginMode(0),
    numIndices(0),
    dataType(0)
    {
        
    }
    EBO::~EBO()
    {
        glDeleteBuffers( 1, &ebo );
    }
    GLuint EBO::makeEbo()
    {
        GLuint ebo;
        glGenBuffers(1, &ebo);
        return ebo;
    }
    void EBO::bind()
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    }
    void EBO::unbind()
    {
        assert(GLMgr::instance().getEbo() == this);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    void EBO::bufferData(GLsizeiptr size, const GLvoid* data, GLenum usage, GLenum _beginMode, GLsizei _numIndices, GLenum _dataType)
    {
        assert(GLMgr::instance().getEbo() == this);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
        beginMode = _beginMode;
        numIndices = _numIndices;
        dataType = _dataType;
    }
    void EBO::draw()
    {
        assert(GLMgr::instance().getEbo() == this);
        glDrawElements(beginMode, numIndices, dataType, NULL);
    }
    
    GLMgr& GLMgr::instance()
    {
        static GLMgr instance;
        return instance;
    }

    GLMgr::GLMgr() :
    refCount(0)
    {}
    GLMgr::~GLMgr()
    {
        
    }
    void GLMgr::inc()
    {
        refCount++;
    }
    void GLMgr::dec()
    {
        if (--refCount == 0)
        {
            if (vao != nextVao)
            {
                if (!nextVao)
                {
                    vao->unbind();
                }
                else
                {
                    nextVao->bind();
                }
            }
            if (vbo != nextVbo)
            {
                if (!nextVbo)
                {
                    vbo->unbind();
                }
                else
                {
                    nextVbo->bind();
                }
            }
            if (ebo != nextEbo)
            {
                if (!nextEbo)
                {
                    ebo->unbind();
                }
                else
                {
                    nextEbo->bind();
                }
            }
            vao = nextVao;
            vbo = nextVbo;
            ebo = nextEbo;
            nextVao.reset();
            nextVbo.reset();
            nextEbo.reset();
        }
    }
    void GLMgr::reset()
    {
        inc(); dec();
    }
    
    SVG::SVG(const char* const pathname) :
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
    
    SVG::~SVG()
    {
    }
    
    void SVG::draw()
    {
        GLMgr::instance().bind(vao).bind(ebo);
        ebo->draw();
    }
}
