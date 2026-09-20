#ifndef YDS_DEVICE_H
#define YDS_DEVICE_H

#include "yds_base.h"
#include "yds_context_object.h"
#include "yds_gpu_buffer.h"
#include "yds_input_layout.h"
#include "yds_rendering_context.h"
#include "yds_shader.h"
#include "yds_shader_program.h"
#include "yds_texture.h"
#include "yds_window.h"

#if defined(_DEBUG)
#define YDS_SUPPORTS_SHADER_COMPILATION 1
#else
#define YDS_SUPPORTS_SHADER_COMPILATION 0
#endif

struct ysTextureSlot {
    ysRenderTarget *RenderTarget;
    ysTexture *Texture;
};

class ysDevice : public ysContextObject {
protected:
    ysDevice();
    ysDevice(ysContextObject::DeviceAPI API);
    virtual ~ysDevice();

    static constexpr int MaxRenderTargets = 2;

public:
    enum class CullMode { Front, Back, None };

public:
    static ysError CreateDevice(ysDevice **device, DeviceAPI API);

    inline void SetVerticalSyncEnable(bool enable) {
        m_verticalSyncEnabled = enable;
    }

    /* main device interface */

    // initialize graphics device
    virtual ysError InitializeDevice() = 0;

    // destroy graphics device
    virtual ysError DestroyDevice() = 0;

    // check support for this device
    virtual bool CheckSupport() = 0;


    /* rendering contexts */

    // create a new rendering context
    virtual ysError
    CreateRenderingContext(ysRenderingContext **renderingContext,
                           ysWindow *window) = 0;

    // update a rendering context
    virtual ysError UpdateRenderingContext(ysRenderingContext *context) = 0;

    // destroy rendering context
    virtual ysError DestroyRenderingContext(ysRenderingContext *&context);

    // set the mode of a rendering context
    virtual ysError SetContextMode(ysRenderingContext *context,
                                   ysRenderingContext::ContextMode mode);

    // get the number of created rendering contexts
    int GetRenderingContextCount() {
        return m_renderingContexts.GetNumObjects();
    }


    /* state */

    // enable/disable face culling
    virtual ysError SetFaceCulling(bool faceCulling) = 0;

    // set face culling mode
    virtual ysError SetFaceCullingMode(CullMode cullMode) = 0;


    /* render targets */

    // create an on-screen render target
    virtual ysError CreateOnScreenRenderTarget(ysRenderTarget **newTarget,
                                               ysRenderingContext *context,
                                               bool depthBuffer) = 0;

    // create an off-screen render target
    virtual ysError CreateOffScreenRenderTarget(ysRenderTarget **newTarget,
                                                int width, int height,
                                                ysRenderTarget::Format format,
                                                bool colorData = true,
                                                bool depthBuffer = true) = 0;

    // create a off-screen copy
    virtual ysError
    CreateOffScreenRenderTarget(ysRenderTarget **newTarget,
                                const ysRenderTarget *reference);

    // create a sub render target
    virtual ysError CreateSubRenderTarget(ysRenderTarget **newTarget,
                                          ysRenderTarget *parent, int x, int y,
                                          int width, int height) = 0;

    // resize a render target
    virtual ysError ResizeRenderTarget(ysRenderTarget *target, int width,
                                       int height, int pwidth, int pheight);

    // reposition a render target
    virtual ysError RepositionRenderTarget(ysRenderTarget *target, int x,
                                           int y);

    // enable/disable depth testing
    virtual ysError SetDepthTestEnabled(ysRenderTarget *target, bool enable);

    // destroy a render target
    virtual ysError DestroyRenderTarget(ysRenderTarget *&target);

    // set the active rendering target
    virtual ysError SetRenderTarget(ysRenderTarget *target, int slot = 0);

    // read render target
    virtual ysError ReadRenderTarget(ysRenderTarget *src, uint8_t *target);

    /* scene start/end */

    // clear the current render target
    virtual ysError ClearBuffers(const float *clearColor) = 0;

    // present the current on-screen render target
    virtual ysError Present() = 0;


    /* buffers */

    // create vertex buffer
    virtual ysError CreateVertexBuffer(ysGPUBuffer **newBuffer, int size,
                                       char *data,
                                       bool mirrorToRam = false) = 0;

    // create index buffer
    virtual ysError CreateIndexBuffer(ysGPUBuffer **newBuffer, int size,
                                      char *data, bool mirrorToRam = false) = 0;

    // create constant buffer
    virtual ysError CreateConstantBuffer(ysGPUBuffer **newBuffer, int size,
                                         char *data,
                                         bool mirrorToRam = false) = 0;

    // enable a vertex buffer
    virtual ysError UseVertexBuffer(ysGPUBuffer *buffer, int stride,
                                    int offset);

    // enable an instance buffer
    virtual ysError UseInstanceBuffer(ysGPUBuffer *buffer, int stride,
                                      int offset);

    // enable an index buffer
    virtual ysError UseIndexBuffer(ysGPUBuffer *buffer, int offset);

    // enable a constant buffer
    virtual ysError UseConstantBuffer(ysGPUBuffer *buffer, int slot);

    // get the active buffer in any slot
    ysGPUBuffer *GetActiveBuffer(ysGPUBuffer::GPU_BUFFER_TYPE bufferType);

    // edit the data in a section of a buffer
    virtual ysError EditBufferDataRange(ysGPUBuffer *buffer, char *data,
                                        int size, int offset);

    // replace all data in a buffer
    virtual ysError EditBufferData(ysGPUBuffer *buffer, char *data);

    // delete a gpu buffer
    virtual ysError DestroyGPUBuffer(ysGPUBuffer *&buffer);


    /* shaders */

    // create a vertex shader from a file
    virtual ysError CreateVertexShader(ysShader **newShader,
                                       const wchar_t *shaderFilename,
                                       const wchar_t *compiledFilename,
                                       const char *shaderName,
                                       bool compile) = 0;
    ysError CreateVertexShader(ysShader **newShader,
                               const wchar_t *shaderFilename,
                               const char *shaderName, bool compile = true);

    // create a pixel shader from a file
    virtual ysError CreatePixelShader(ysShader **newShader,
                                      const wchar_t *shaderFilename,
                                      const wchar_t *compiledFilename,
                                      const char *shaderName, bool compile) = 0;
    ysError CreatePixelShader(ysShader **newShader,
                              const wchar_t *shaderFilename,
                              const char *shaderName, bool compile = true);

    // destroy a shader
    virtual ysError DestroyShader(ysShader *&shader);


    /* shader programs */

    // create a shader program
    virtual ysError CreateShaderProgram(ysShaderProgram **newProgram) = 0;

    // destroy a shader program
    virtual ysError DestroyShaderProgram(ysShaderProgram *&shader,
                                         bool destroyShaders = false);

    // attach a shader to a shader program
    virtual ysError AttachShader(ysShaderProgram *targetProgram,
                                 ysShader *shader);

    // link a program
    virtual ysError LinkProgram(ysShaderProgram *program);

    // enable a shader program
    virtual ysError UseShaderProgram(ysShaderProgram *);


    /* input layouts */

    // create an input layout for a shader and format
    virtual ysError CreateInputLayout(
            ysInputLayout **newLayout, ysShader *shader,
            const ysRenderGeometryFormat *format,
            const ysRenderGeometryFormat *instanceFormat = nullptr) = 0;

    // enable an input layout
    virtual ysError UseInputLayout(ysInputLayout *layout);

    // destroy an input layout
    virtual ysError DestroyInputLayout(ysInputLayout *&layout);


    /* textures */

    // create a texture from a file
    virtual ysError CreateTexture(ysTexture **texture,
                                  const wchar_t *fname) = 0;

    // create an rgb texture from an in-memory buffer
    virtual ysError CreateTexture(ysTexture **texture, int width, int height,
                                  const unsigned char *buffer) = 0;

    // create an alpha texture from an in-memory buffer
    virtual ysError CreateAlphaTexture(ysTexture **texture, int width,
                                       int height,
                                       const unsigned char *buffer) = 0;

    // update a texture
    virtual ysError UpdateTexture(ysTexture *texture,
                                  const unsigned char *buffer) = 0;

    // destroy a texture
    virtual ysError DestroyTexture(ysTexture *&texture);

    // enable a texture
    virtual ysError UseTexture(ysTexture *texture, int slot);

    // enable a texture in the form of a render target
    virtual ysError UseRenderTargetAsTexture(ysRenderTarget *texture, int slot);

    // initialize texture slots
    ysError InitializeTextureSlots(int maxSlots);

    /* debug */

    // temp
    virtual void Draw(int numFaces, int indexOffset, int vertexOffset) {
        (void) numFaces;
        (void) indexOffset;
        (void) vertexOffset;
    }

    virtual void DrawInstanced(int numIndices, int indexOffset,
                               int vertexOffset, int instanceCount,
                               int instanceOffset);

    virtual void DrawLines(int numIndices, int indexOffset, int vertexOffset) {
        (void) numIndices;
        (void) indexOffset;
        (void) vertexOffset;
    }

    ysRenderTarget *GetActiveRenderTarget(int slot = 0) const {
        return m_activeRenderTarget[slot];
    }

    void SetDebugFlag(int flag, bool state);
    bool GetDebugFlag(int flag) const;

protected:
    ysRenderTarget *GetActualRenderTarget(int slot);

protected:
    // object holders
    ysDynamicArray<ysRenderingContext, 4> m_renderingContexts;
    ysDynamicArray<ysRenderTarget, 4> m_renderTargets;
    ysDynamicArray<ysGPUBuffer, 16> m_gpuBuffers;
    ysDynamicArray<ysShader, 16> m_shaders;
    ysDynamicArray<ysShaderProgram, 8> m_shaderPrograms;
    ysDynamicArray<ysInputLayout, 16> m_inputLayouts;
    ysDynamicArray<ysTexture, 32> m_textures;

    // active objects
    ysRenderTarget *m_activeRenderTarget[MaxRenderTargets];
    ysRenderingContext *m_activeContext;

    ysGPUBuffer *m_activeVertexBuffer;
    ysGPUBuffer *m_activeInstanceBuffer;
    ysGPUBuffer *m_activeIndexBuffer;
    ysGPUBuffer *m_activeConstantBuffer;

    ysShaderProgram *m_activeShaderProgram;

    ysInputLayout *m_activeInputLayout;

    ysTextureSlot *m_activeTextures;

    bool m_verticalSyncEnabled;

    // debug
    unsigned int m_debugFlags;

    // platform dependant constants
    int m_maxTextureSlots;
};

#endif /* YDS_DEVICE_H  */
