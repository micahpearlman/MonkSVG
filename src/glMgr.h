/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */
#ifndef glMgr_h
#define glMgr_h

#include <OpenGLES/ES2/gl.h>
#include <memory>
#include <vector>
#include <deque>

namespace Saka
{
    class VBO
    {
    public:
        const GLuint vbo;
        
        struct Attrib
        {
            GLint size;
            GLenum type;
            GLboolean normalized;
            GLsizei stride;
            const GLvoid* ptr;
        };
        std::vector<Attrib> attribs;
        
        VBO();
        ~VBO();
        static GLuint makeVbo();
        
        void bind();
        void unbind();
        void bufferData(GLsizeiptr size, const GLvoid* data, GLenum usage);
        void setNumAttribs(int num) { attribs.reserve(num); }
        void addAttrib(GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr) { attribs.push_back({size, type, normalized, stride, ptr}); }
        void setupAttribs();
    };
    typedef std::shared_ptr<VBO> SharedVBO;
    
    class VAO
    {
    public:
        const GLuint vao;
        std::deque<SharedVBO> vbos;
        
        VAO();
        ~VAO();
        static GLuint makeVao();
        
        void bind();
        void unbind();
        void addVbo(const SharedVBO& vbo);
    };
    typedef std::shared_ptr<VAO> SharedVAO;
    
    class EBO
    {
    public:
        const GLuint ebo;
        GLenum beginMode;
        GLsizei numIndices;
        GLenum dataType;
        
        EBO();
        ~EBO();
        static GLuint makeEbo();
        
        void bind();
        void unbind();
        void bufferData(GLsizeiptr size, const GLvoid* data, GLenum usage, GLenum beginMode, GLsizei numIndices, GLenum dataType);
        void draw();
    };
    typedef std::shared_ptr<EBO> SharedEBO;
    class Shader
    {
    public:
        GLuint shader;
        
        Shader(GLenum type);
        ~Shader();
        static GLuint makeShader(GLenum type);
        
        void compile(const char* program);
        
    private:
        void doCompile(const char* program);
    };
    typedef std::shared_ptr<Shader> SharedShader;
    class VertexShader : public Shader
    {
    public:
        VertexShader() : Shader(GL_VERTEX_SHADER) {}
    };
    class FragmentShader : public Shader
    {
    public:
        FragmentShader() : Shader(GL_FRAGMENT_SHADER) {}
    };
    
    class Program
    {
    public:
        const GLuint program;
        GLuint numAttrib;
        
        Program();
        ~Program();
        static GLuint makeProgram();
        
        void attachShader(SharedShader shader);
        void bindAttrib(const char* const name);
        void linkProgram();
        
        void bind();
        void unbind();
    };
    typedef std::shared_ptr<Program> SharedProgram;
    
    class GLMgr
    {
    public:
        static GLMgr& instance();
        ~GLMgr();
        
        class Do
        {
        private:
            GLMgr& glMgr;
        public:
            Do(GLMgr& _mgr) : glMgr(_mgr) { glMgr.inc(); }
            ~Do() { glMgr.dec(); }
            
            Do bind(const SharedVAO& vao) { glMgr.nextVao = vao; return Do(glMgr); }
            Do bind(const SharedVBO& vbo) { glMgr.nextVbo = vbo; return Do(glMgr); }
            Do bind(const SharedEBO& ebo) { glMgr.nextEbo = ebo; return Do(glMgr); }
            Do bind(const SharedProgram& program) { glMgr.nextProgram = program; return Do(glMgr); }
        };
        Do bind(const SharedVAO& vao) { return Do(*this).bind(vao); }
        Do bind(const SharedVBO& vbo) { return Do(*this).bind(vbo); }
        Do bind(const SharedEBO& ebo) { return Do(*this).bind(ebo); }
        Do bind(const SharedProgram& program) { return Do(*this).bind(program); }
        void reset();
        
        inline VAO* getVao() { return vao.get(); }
        inline VBO* getVbo() { return vbo.get(); }
        inline EBO* getEbo() { return ebo.get(); }
        inline Program* getProgram() { return program.get(); }
        
    private:
        GLMgr();
        
        SharedVAO nextVao;
        SharedVBO nextVbo;
        SharedEBO nextEbo;
        SharedProgram nextProgram;
        
        SharedVAO vao;
        SharedVBO vbo;
        SharedEBO ebo;
        SharedProgram program;
        
        int refCount;
        void inc();
        void dec();
    };
}

#endif /* glMgr_h */
