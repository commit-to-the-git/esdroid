#include "../include/yds_opengl_egl_context.h"
#include "../include/yds_opengl_device.h"
#include "../include/yds_rendering_context.h"
#include <android/log.h>
#include <android/native_window.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"ESDroid",__VA_ARGS__))

// Forward-declare the AndroidBackend swapBuffers
namespace esdroid { class AndroidBackend; }
extern "C" void esdroid_swap_buffers();

template<typename T> static T load_gl(const char* name, T fb) {
    T p=reinterpret_cast<T>(eglGetProcAddress(name));
    return p?p:fb;
}

ysOpenGLEglContext::ysOpenGLEglContext() : ysOpenGLVirtualContext(ysWindowSystemObject::Platform::Android) { m_isRealContext=false; m_device=nullptr; }
ysOpenGLEglContext::~ysOpenGLEglContext() {}

ysError ysOpenGLEglContext::CreateRenderingContext(ysOpenGLDevice *device) {
    YDS_ERROR_DECLARE("CreateRenderingContext");
    m_device=device;
    LoadAllExtensions();
    m_isRealContext=true;
    device->UpdateContext();
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysOpenGLEglContext::Present() {
    YDS_ERROR_DECLARE("Present");
    // Swap the EGL surface buffer to the screen.
    esdroid_swap_buffers();
    return YDS_ERROR_RETURN(ysError::None);
}

void ysOpenGLEglContext::LoadAllExtensions() {
    glGenBuffers=load_gl("glGenBuffers",&::glGenBuffers);
    glDeleteBuffers=load_gl("glDeleteBuffers",&::glDeleteBuffers);
    glDeleteVertexArrays=load_gl("glDeleteVertexArrays",&::glDeleteVertexArrays);
    glBindBuffer=load_gl("glBindBuffer",&::glBindBuffer);
    glBindBufferRange=load_gl("glBindBufferRange",&::glBindBufferRange);
    glBufferData=load_gl("glBufferData",&::glBufferData);
    glGenVertexArrays=load_gl("glGenVertexArrays",&::glGenVertexArrays);
    glBindVertexArray=load_gl("glBindVertexArray",&::glBindVertexArray);
    glEnableVertexAttribArray=load_gl("glEnableVertexAttribArray",&::glEnableVertexAttribArray);
    glVertexAttribPointer=load_gl("glVertexAttribPointer",&::glVertexAttribPointer);
    glVertexAttribIPointer=load_gl("glVertexAttribIPointer",&::glVertexAttribIPointer);
    glVertexAttrib3f=load_gl("glVertexAttrib3f",&::glVertexAttrib3f);
    glVertexAttrib4f=load_gl("glVertexAttrib4f",&::glVertexAttrib4f);
    glDeleteProgram=load_gl("glDeleteProgram",&::glDeleteProgram);
    glDeleteShader=load_gl("glDeleteShader",&::glDeleteShader);
    glCreateShader=load_gl("glCreateShader",&::glCreateShader);
    glShaderSource=load_gl("glShaderSource",&::glShaderSource);
    glCompileShader=load_gl("glCompileShader",&::glCompileShader);
    glCreateProgram=load_gl("glCreateProgram",&::glCreateProgram);
    glAttachShader=load_gl("glAttachShader",&::glAttachShader);
    glDetachShader=load_gl("glDetachShader",&::glDetachShader);
    glLinkProgram=load_gl("glLinkProgram",&::glLinkProgram);
    glUseProgram=load_gl("glUseProgram",&::glUseProgram);
    glBindAttribLocation=load_gl("glBindAttribLocation",&::glBindAttribLocation);
    glBindFragDataLocation=reinterpret_cast<PFNGLBINDFRAGDATALOCATIONPROC>(eglGetProcAddress("glBindFragDataLocation"));
    if(!glBindFragDataLocation) glBindFragDataLocation=reinterpret_cast<PFNGLBINDFRAGDATALOCATIONPROC>(eglGetProcAddress("glBindFragDataLocationEXT"));
    glGetFragDataLocation=reinterpret_cast<PFNGLGETFRAGDATALOCATIONPROC>(eglGetProcAddress("glGetFragDataLocation"));
    if(!glGetFragDataLocation) glGetFragDataLocation=reinterpret_cast<PFNGLGETFRAGDATALOCATIONPROC>(eglGetProcAddress("glGetFragDataLocationEXT"));
    glGetUniformLocation=load_gl("glGetUniformLocation",&::glGetUniformLocation);
    glGetShaderiv=load_gl("glGetShaderiv",&::glGetShaderiv);
    glGetShaderInfoLog=load_gl("glGetShaderInfoLog",&::glGetShaderInfoLog);
    glDrawBuffers=load_gl("glDrawBuffers",&::glDrawBuffers);
    glUniform4f=load_gl("glUniform4f",&::glUniform4f);
    glUniform4fv=load_gl("glUniform4fv",&::glUniform4fv);
    glUniform3fv=load_gl("glUniform3fv",&::glUniform3fv);
    glUniform2fv=load_gl("glUniform2fv",&::glUniform2fv);
    glUniform3f=load_gl("glUniform3f",&::glUniform3f);
    glUniform2f=load_gl("glUniform2f",&::glUniform2f);
    glUniform1f=load_gl("glUniform1f",&::glUniform1f);
    glUniform1i=load_gl("glUniform1i",&::glUniform1i);
    glUniformMatrix4fv=load_gl("glUniformMatrix4fv",&::glUniformMatrix4fv);
    glUniformMatrix3fv=load_gl("glUniformMatrix3fv",&::glUniformMatrix3fv);
    glGetProgramiv=load_gl("glGetProgramiv",&::glGetProgramiv);
    glGetActiveUniformName=+[](GLuint p,GLuint i,GLsizei b,GLsizei*l,GLchar*n){::glGetActiveUniform(p,i,b,l,nullptr,nullptr,n);};
    glGetActiveUniformsiv=load_gl("glGetActiveUniformsiv",&::glGetActiveUniformsiv);
    glGetActiveUniform=load_gl("glGetActiveUniform",&::glGetActiveUniform);
    // Load the uniform block binding entry points so the per-stage UBO
    // slots (0/1/3) can be remapped after linking.
    glGetActiveUniformBlockName=load_gl("glGetActiveUniformBlockName",&::glGetActiveUniformBlockName);
    glUniformBlockBinding=load_gl("glUniformBlockBinding",&::glUniformBlockBinding);
    glDrawElementsBaseVertex=reinterpret_cast<PFNGLDRAWELEMENTSBASEVERTEXPROC>(eglGetProcAddress("glDrawElementsBaseVertex"));
    if(!glDrawElementsBaseVertex) glDrawElementsBaseVertex=reinterpret_cast<PFNGLDRAWELEMENTSBASEVERTEXPROC>(eglGetProcAddress("glDrawElementsBaseVertexOES"));
    if(!glDrawElementsBaseVertex) glDrawElementsBaseVertex=reinterpret_cast<PFNGLDRAWELEMENTSBASEVERTEXPROC>(eglGetProcAddress("glDrawElementsBaseVertexEXT"));
    glTexImage2DMultisample=reinterpret_cast<PFNGLTEXIMAGE2DMULTISAMPLEPROC>(eglGetProcAddress("glTexImage2DMultisample"));
    glActiveTexture=load_gl("glActiveTexture",&::glActiveTexture);
    glGenerateMipmap=load_gl("glGenerateMipmap",&::glGenerateMipmap);
    glMapBuffer=+[](GLenum t,GLenum){GLint sz=0;::glGetBufferParameteriv(t,GL_BUFFER_SIZE,&sz);return ::glMapBufferRange(t,0,sz,GL_MAP_READ_BIT|GL_MAP_WRITE_BIT);};
    glMapBufferRange=load_gl("glMapBufferRange",&::glMapBufferRange);
    glUnmapBuffer=load_gl("glUnmapBuffer",&::glUnmapBuffer);
    glGenRenderbuffers=load_gl("glGenRenderbuffers",&::glGenRenderbuffers);
    glDeleteRenderbuffers=load_gl("glDeleteRenderbuffers",&::glDeleteRenderbuffers);
    glBindRenderbuffer=load_gl("glBindRenderbuffer",&::glBindRenderbuffer);
    glRenderbufferStorage=load_gl("glRenderbufferStorage",&::glRenderbufferStorage);
    glRenderbufferStorageMultisample=reinterpret_cast<PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC>(eglGetProcAddress("glRenderbufferStorageMultisample"));
    if(!glRenderbufferStorageMultisample) glRenderbufferStorageMultisample=reinterpret_cast<PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC>(eglGetProcAddress("glRenderbufferStorageMultisampleEXT"));
    glCopyBufferSubData=load_gl("glCopyBufferSubData",&::glCopyBufferSubData);
    glBufferSubData=load_gl("glBufferSubData",&::glBufferSubData);
    glGenFramebuffers=load_gl("glGenFramebuffers",&::glGenFramebuffers);
    glDeleteFramebuffers=load_gl("glDeleteFramebuffers",&::glDeleteFramebuffers);
    glBindFramebuffer=load_gl("glBindFramebuffer",&::glBindFramebuffer);
    glFramebufferTexture2D=load_gl("glFramebufferTexture2D",&::glFramebufferTexture2D);
    glFramebufferRenderbuffer=load_gl("glFramebufferRenderbuffer",&::glFramebufferRenderbuffer);
    glCheckFramebufferStatus=load_gl("glCheckFramebufferStatus",&::glCheckFramebufferStatus);
    glBlitFramebuffer=load_gl("glBlitFramebuffer",&::glBlitFramebuffer);
    glBlendEquation=load_gl("glBlendEquation",&::glBlendEquation);
    wglMakeContextCurrent=nullptr; wglCreateContextAttribsARB=nullptr; wglChoosePixelFormatARB=nullptr;
    LOGI("GL ES pointers loaded");
    // Log at startup whether glDrawElementsBaseVertex loaded. If null, the
    // GLES 3.0 driver lacks GL_OES_draw_elements_base_vertex and the
    // absolute-index fallback is used instead.
    LOGI("ESDroid: glDrawElementsBaseVertex = %p", (void*)glDrawElementsBaseVertex);
    LOGI("ESDroid: glUniformBlockBinding = %p", (void*)glUniformBlockBinding);
}
