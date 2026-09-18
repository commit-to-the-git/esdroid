/*
 * Copyright 2026 F² Cyanic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ESDROID_ANDROID_BACKEND_H
#define ESDROID_ANDROID_BACKEND_H
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <string>
#include <vector>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <thread>

namespace esdroid {

class TouchUI;

enum class VirtualKey : int {
    None=0, Escape, Return, Tab, Insert,
    N1,N2,N3,N4,N5, F1,F2,F3, F, Right,
    Starter, Ignition, Throttle, Clutch,
    ShiftUp, ShiftDown, HalfSpeed, QuarterSpeed,
    Pause, ResetEngine, Camera, OscPage,
    ViewLayerUp, ViewLayerDown, Dyno, DynoHold,
    Throttle10, Throttle20, ZoomIn, ZoomOut, Fn, Count
};

struct TouchButton {
    const char* label;
    const char* altLabel;
    float x,y,w,h;
    VirtualKey key;
    VirtualKey altKey;
    bool held;
    bool edge;
    int pointerId;
};

class AndroidBackend {
public:
    static AndroidBackend& instance();
    static void setAndroidApp(struct android_app* app);
    static void setAssetManager(AAssetManager* mgr);
    AAssetManager* assetManager() const { return m_assetManager; }
    static android_app* getAndroidApp() { return s_app; }
    bool initWindow(int w,int h);
    void destroyWindow();
    void makeContextCurrent();
    void swapBuffers();
    bool isWindowReady() const { return m_eglSurface!=EGL_NO_SURFACE; }
    int screenWidth() const { return m_screenWidth; }
    int screenHeight() const { return m_screenHeight; }
    bool pollEvents();
    bool isKeyDown(VirtualKey k) const;
    bool processKeyDown(VirtualKey k);
    void clearAllKeys();
    float mouseWheel() const { return m_mouseWheel; }
    TouchButton* hitTestButton(float x,float y);
    void pressButton(TouchButton* btn,int pid);
    void releaseButton(TouchButton* btn);
    void releaseAllButtons(int pid, bool cancel=false);
    std::vector<TouchButton>& buttons() { return m_buttons; }
    bool fnActive() const { return m_fnLatched||m_fnHeld; }
    VirtualKey effectiveKey(const TouchButton& b) const {
        if(b.key==VirtualKey::Fn) return VirtualKey::None;
        if(fnActive()&&b.altKey!=VirtualKey::None) return b.altKey;
        return b.key;
    }
    void layoutButtons(int sw,int sh);
    bool initAudio(int sr,int ch);
    void destroyAudio();
    void resetAudioRing();
    bool writeAudioSamples(const int16_t* samples,int count,int* written);
    int getCurrentWritePosition() const;
    int getAudioReadPos() const { return m_audioReadPos; }
    bool readAsset(const char* path,void** outBuf,long* outSize);
    bool readFile(const char* path,void** outBuf,long* outSize);
    std::string filesDir() const { return m_filesDir; }
    void setFilesDir(const std::string& d) { m_filesDir=d; }
    void requestMrFilePicker();
    void onMrFilePicked(const std::string& uri);
    bool consumeScriptReloadPending();
    std::string activeMrPath() const { return m_activeMrPath; }
    void resetToDefaultMr() { m_activeMrPath="assets/main.mr"; }
    double getFrameLength() const;
    double getAverageFramerate() const;
    bool shouldQuit() const { return m_shouldQuit; }
    void requestQuit() { m_shouldQuit=true; }
    void initTouchUI();
    void renderTouchUI();
    void resizeTouchUI();
    // Public for audio callback access
    int m_sampleRate=44100, m_channels=1;
    std::vector<int16_t> m_slBuffers[2];
    int m_slNextBuffer=0;
    std::vector<int16_t> m_audioRing;
    int m_audioWritePos=0, m_audioReadPos=0;
    std::mutex m_audioMutex;
    // Public for frame timing updates
    int64_t m_frameTimes[60] = {};
    int m_frameTimeIdx = 0;
    int m_frameTimeCount = 0;
private:
    AndroidBackend();
    static android_app* s_app;
    AAssetManager* m_assetManager=nullptr;
    std::string m_filesDir;
    EGLDisplay m_eglDisplay=EGL_NO_DISPLAY;
    EGLContext m_eglContext=EGL_NO_CONTEXT;
    EGLSurface m_eglSurface=EGL_NO_SURFACE;
    EGLConfig m_eglConfig=nullptr;
    int m_screenWidth=0, m_screenHeight=0;
    bool m_keyState[(int)VirtualKey::Count]={};
    bool m_keyEdge[(int)VirtualKey::Count]={};
    float m_mouseWheel=0.0f;
    std::vector<TouchButton> m_buttons;
    bool m_fnLatched=false;
    bool m_fnHeld=false;
    int64_t m_fnPressNs=0;
    SLObjectItf m_slEngine=nullptr, m_slOutputMix=nullptr, m_slPlayer=nullptr, m_slBufferQueue=nullptr;
    SLEngineItf m_slEngineItf=nullptr;
    SLPlayItf m_slPlayItf=nullptr;
    SLAndroidSimpleBufferQueueItf m_slQueue=nullptr;
    bool m_audioInited=false;
    std::atomic<bool> m_shouldQuit{false};
    std::atomic<bool> m_scriptReloadPending{false};
    std::string m_activeMrPath="assets/main.mr";
    // The path picked in Java is staged here and only moved into
    // m_activeMrPath by the render thread when it consumes the reload flag.
    std::mutex m_mrMutex;
    std::string m_pendingMrPath;
    TouchUI* m_touchUI=nullptr;
};

} // namespace esdroid
#endif // ESDROID_ANDROID_BACKEND_H
