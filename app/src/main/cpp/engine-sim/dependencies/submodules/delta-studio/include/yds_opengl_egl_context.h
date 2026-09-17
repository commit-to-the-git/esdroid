#ifndef YDS_OPENGL_EGL_CONTEXT_H
#define YDS_OPENGL_EGL_CONTEXT_H
#include "yds_opengl_context.h"
#include "OpenGL.h"
class ysOpenGLDevice;
class ysOpenGLEglContext : public ysOpenGLVirtualContext {
    friend ysOpenGLDevice;
public:
    ysOpenGLEglContext();
    virtual ~ysOpenGLEglContext();
    ysError CreateRenderingContext(ysOpenGLDevice *device);
    virtual ysError DestroyContext() override { return YDS_ERROR_RETURN(ysError::None); }
    virtual ysError TransferContext(ysOpenGLVirtualContext *) override { return YDS_ERROR_RETURN(ysError::None); }
    virtual ysError SetContextMode(ContextMode) override { return YDS_ERROR_RETURN(ysError::None); }
    virtual ysError SetContext(ysRenderingContext *) override { return YDS_ERROR_RETURN(ysError::None); }
    virtual ysError Present() override { return YDS_ERROR_RETURN(ysError::None); }
protected:
    void LoadAllExtensions();
    ysOpenGLDevice *m_device=nullptr;
};
#endif
