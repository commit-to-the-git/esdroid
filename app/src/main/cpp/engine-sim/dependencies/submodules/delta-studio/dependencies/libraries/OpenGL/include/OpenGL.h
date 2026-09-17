// OpenGL.h - Android GLES3 replacement for delta-studio's Windows OpenGL.h
#ifndef OPENGL_H
#define OPENGL_H
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>

#ifndef PFNGLBINDFRAGDATALOCATIONPROC
typedef PFNGLBINDFRAGDATALOCATIONEXTPROC PFNGLBINDFRAGDATALOCATIONPROC;
#endif
#ifndef PFNGLGETACTIVEUNIFORMNAMEPROC
typedef void (GL_APIENTRYP PFNGLGETACTIVEUNIFORMNAMEPROC)(GLuint,GLuint,GLsizei,GLsizei*,GLchar*);
#endif
#ifndef PFNGLDRAWELEMENTSBASEVERTEXPROC
typedef void (GL_APIENTRYP PFNGLDRAWELEMENTSBASEVERTEXPROC)(GLenum,GLsizei,GLenum,const void*,GLint);
#endif
#ifndef PFNGLTEXIMAGE2DMULTISAMPLEPROC
typedef void (GL_APIENTRYP PFNGLTEXIMAGE2DMULTISAMPLEPROC)(GLenum,GLsizei,GLenum,GLsizei,GLsizei,GLboolean);
#endif
#ifndef PFNGLMAPBUFFERPROC
typedef void *(GL_APIENTRYP PFNGLMAPBUFFERPROC)(GLenum,GLenum);
#endif
#ifndef PFNWGLMAKECONTEXTCURRENTARBPROC
typedef void (*PFNWGLMAKECONTEXTCURRENTARBPROC)(void*,void*,void*);
#endif
#ifndef PFNWGLCREATECONTEXTATTRIBSARBPROC
typedef void* (*PFNWGLCREATECONTEXTATTRIBSARBPROC)(void*,void*,const int*);
#endif
#ifndef PFNWGLCHOOSEPIXELFORMATARBPROC
typedef int (*PFNWGLCHOOSEPIXELFORMATARBPROC)(void*,const int*,const float*,unsigned int,int*,unsigned int*);
#endif

#define glClearDepth(d) glClearDepthf((float)(d))
#define glGetActiveUniformName(prog,idx,bufSize,length,name) \
    glGetActiveUniform((prog),(idx),(bufSize),(length),nullptr,nullptr,(name))
#ifndef GL_4_BYTES
#define GL_4_BYTES 0x1409
#endif
#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif
#ifndef GL_RGBA32F_ARB
#define GL_RGBA32F_ARB 0x8814
#endif
#ifndef GL_FLAT
#define GL_FLAT 0x1D00
#endif
#ifndef GL_DEPTH_COMPONENT32
#define GL_DEPTH_COMPONENT32 0x81A7
#endif
#endif // OPENGL_H
