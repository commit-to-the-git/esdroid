#ifndef YDS_ERROR_CODES_H
#define YDS_ERROR_CODES_H

enum class ysError {
    None,

    //
    // programming errors
    //

    InvalidParameter,
    MultipleErrorSystems,
    MultipleSystems,

    OutOfMemory,

    InvalidGpuBufferType,
    IncompatiblePlatforms,
    NoPlatform,

    InvalidOperation,
    NotImplemented,

    UninitializedBuffer,
    OutOfBounds,

    // shaders
    ProgramAlreadyLinked,
    ProgramNotLinked,
    ProgramLinkError,

    // contexts
    ContextAlreadyHasRenderTarget,

    NoDevice,
    NoRenderTarget,
    NoContext,

    //
    // api errors
    //

    CouldNotCreateGraphicsDevice,
    CouldNotObtainDevice,

    ApiError,

    // directx specific
    CouldNotCreateSwapChain,

    CouldNotEnterFullscreen,
    CouldNotExitFullscreen,

    CouldNotCreateGpuBuffer,

    // opengl specific
    CouldNotActivateTemporaryContext,
    CouldNotActivateContext,
    CouldNotCreateTemporaryContext,
    CouldNotCreateContext,
    CouldNotDestroyContext,
    BufferSwapError,

    // vulkan specific
    NoQueueFamilyFound,

    // render targets
    CouldNotGetBackBuffer,
    CouldNotCreateRenderTarget,
    CouldNotCreateDepthBuffer,

    // shaders
    VertexShaderCompilationError,
    FragmentShaderCompilationError,
    CouldNotCreateShader, 

    // input formats
    IncompatibleInputFormat,

    // textures
    CouldNotOpenTexture,
    CouldNotMakeShaderResourceView,

    // testing
    TestError,

    // input system
    NoDeviceList,
    CouldNotRegisterForInput,
    NoWindowSystem,

    // audio system
    BufferAlreadyLocked,
    BufferNotLocked,

    // streaming audio
    NoFile,
    NoAudioBuffer,
    NoFileBuffer,
    IncompatibleBufferAndFile,

    //
    // file errors
    //

    CouldNotOpenFile,
    InvalidFileType,
    UnsupportedFileVersion,
    CorruptedFile,

    UnsupportedType,

    CouldNotCreateSamplerState,
    Unsupported,

    CouldNotEnumerateMonitors,
    CouldNotEnumerateAudioDevices,
    CouldNotCreateDS8Device,
    CouldNotSetDeviceCooperativeLevel,
    CouldNotCreateSoundBuffer,
    NoAudioDevice,
    CouldNotCreateDS8DeviceInvalidParam,
    CouldNotCreateDS8DeviceNoDriver,
    CouldNotCreateDS8DeviceOther,

    CouldNotLoadCompiledShader,

    CouldNotCreateDS8DeviceDeviceInUse,
    CouldNotCreateDS8DeviceNoAggregation,
    CouldNotCreateDS8DeviceOutOfMemory,
};

#endif /* YDS_ERROR_CODES_H  */
