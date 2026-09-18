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

namespace esdroid {

class TouchUI;

enum class VirtualKey : int {
    None=0, Escape, Return, Tab, Insert,
    N1,N2,N3,N4,N5, F1,F2,F3, F, Right,
    Starter, Ignition, Throttle, Clutch,
    ShiftUp, ShiftDown,
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

// One row in the settings panel. The desktop wheel combos tune these
// (Z/X/C/V/B/N/G + wheel) plus the touch THROTTLE button's percentage.
enum class SettingIndex : int {
    Volume=0, Convolution, HiFreqGain, LoFreqNoise, HiFreqNoise,
    SimFrequency, DynoSpeed, Throttle, Count
};
constexpr int kSettingCount=(int)SettingIndex::Count;

struct SettingsState {
    bool open=false;
    // audio mixer, 0..1
    float volume=1.0f, convolution=1.0f, hiFreqGain=0.01f;
    float loFreqNoise=1.0f, hiFreqNoise=0.5f;
    double simFrequency=44100.0;   // Hz, 400..400000
    double dynoSpeedRpm=0.0;       // engine range
    double dynoMinRpm=0.0, dynoMaxRpm=8000.0;
    float throttlePct=100.0f;      // what the THROTTLE button does
    unsigned dirty=0;              // bit per SettingIndex, app consumes
};

// Screen-space rects shared by the renderer and the touch hit-tests.
struct SettingsLayout {
    float panel[4]={0,0,0,0};
    float close[4]={0,0,0,0};
    float track[kSettingCount][4];
    float value[kSettingCount][4];
    float rowH=0.0f;
};

// One pickable entry in the import menu. Every entry in both tables below
// was compiled and executed through piranha on the host.
struct MrAsset {
    const char* path;   // import path relative to the assets root
    const char* node;   // theme node, or the engine node for old format files
    const char* name;   // dropdown label
};

struct ImportMenuState {
    bool open=false;
    int themeSel=0, engineSel=0;      // 0 = DEFAULT (stock pair)
    bool themeListOpen=false, engineListOpen=false;
    float themeScroll=0.0f, engineScroll=0.0f;
    int scrollPid=-1;                 // finger driving an open list
    float scrollDownY=0.0f, scrollStart=0.0f;
    int tapEntry=-1;                  // entry under that finger at down
    bool tapMoved=false;
};

struct ImportLayout {
    float panel[4]={0,0,0,0};
    float close[4]={0,0,0,0};
    float themeBox[4]={0,0,0,0}, engineBox[4]={0,0,0,0};
    float themeLoad[4]={0,0,0,0}, engineLoad[4]={0,0,0,0};
    float themeImport[4]={0,0,0,0}, engineImport[4]={0,0,0,0};
    float listTop=0.0f, listH=0.0f, rowH=0.0f;
};

class AndroidBackend {
public:
    static AndroidBackend& instance();
    static void setAndroidApp(struct android_app* app);
    static void setAssetManager(AAssetManager* mgr);
    static android_app* getAndroidApp() { return s_app; }
    bool initWindow(int w,int h);
    void destroyWindow();
    void swapBuffers();
    bool isWindowReady() const { return m_eglSurface!=EGL_NO_SURFACE; }
    int screenWidth() const { return m_screenWidth; }
    int screenHeight() const { return m_screenHeight; }
    bool pollEvents();
    bool isKeyDown(VirtualKey k) const;
    bool processKeyDown(VirtualKey k);
    void clearAllKeys();
    TouchButton* hitTestButton(float x,float y);
    void pressButton(TouchButton* btn,int pid);
    void releaseButton(TouchButton* btn);
    void releaseAllButtons(int pid, bool cancel=false);
    // Engine touch: the first finger that misses every button drives the
    // mouse, a second free finger turns the drag into a pinch. Button
    // touches never reach these.
    void pressEngineTouch(int pid,float x,float y);
    void moveEngineTouch(int pid,float x,float y);
    void releaseEngineTouch(int pid,bool cancel);
    float engineTouchX() const { return m_touchX; }
    float engineTouchY() const { return m_touchY; }
    bool consumeEngineTouchDown() { bool e=m_touchDownEdge; m_touchDownEdge=false; return e; }
    bool consumeEngineTouchUp() {
        bool e=m_touchUpEdge; m_touchUpEdge=false;
        // The reported position teleported (pinch started or ended), so the
        // drag restarts at the new anchor or the view would jump.
        if(e&&m_restartPending){
            m_restartPending=false;
            if(m_touchPid2!=-1){ m_touchX=(m_touchX1+m_touchX2)*0.5f; m_touchY=(m_touchY1+m_touchY2)*0.5f; }
            else{ m_touchX=m_touchX1; m_touchY=m_touchY1; }
            m_touchDownEdge=true;
        }
        return e;
    }
    // Pinch zoom as wheel scroll; whole units out, the fraction stays.
    int consumePinchScroll() { int s=(int)m_pinchWheel; m_pinchWheel-=(float)s; return s; }
    std::vector<TouchButton>& buttons() { return m_buttons; }
    bool fnActive() const { return m_fnLatched||m_fnHeld; }
    VirtualKey effectiveKey(const TouchButton& b) const {
        if(b.key==VirtualKey::Fn) return VirtualKey::None;
        if(fnActive()&&b.altKey!=VirtualKey::None) return b.altKey;
        return b.key;
    }
    void layoutButtons(int sw,int sh);
    // Settings panel: values live here, the app applies them through the
    // dirty bits, the UI (and only the UI) mutates the values.
    SettingsState& settings() { return m_settings; }
    const SettingsState& settings() const { return m_settings; }
    const SettingsLayout& settingsLayout() const { return m_settingsLayout; }
    bool settingsOpen() const { return m_settings.open; }
    void openSettings();
    void closeSettings();
    void layoutSettingsPanel(int sw,int sh);
    int handleSettingsMotion(AInputEvent* event,int32_t actionMasked);
    static const char* settingLabel(int idx);
    void formatSettingValue(char* out,int len,int idx) const;
    float settingSliderT(int idx) const;
    void setSettingFromSlider(int idx,float t);
    void setSettingTyped(int idx,double typed);
    void stageValueInput(int idx,double value);
    // Import menu: engine and theme picks from the bundled assets plus the
    // custom file pickers. Runs on the render thread like the settings.
    ImportMenuState& importMenu() { return m_import; }
    const ImportLayout& importLayout() const { return m_importLayout; }
    bool importMenuOpen() const { return m_import.open; }
    void openImportMenu();
    void closeImportMenu();
    void layoutImportPanel(int sw,int sh);
    int handleImportMotion(AInputEvent* event,int32_t actionMasked);
    static const MrAsset* importThemes(int* count);
    static const MrAsset* importEngines(int* count);
    // The info cluster publishes the title box every rendered frame; the
    // SETTINGS button lives in its bottom-right corner.
    void setSettingsButtonRect(float x,float y,float w,float h) {
        m_settingsButtonRect[0]=x;m_settingsButtonRect[1]=y;
        m_settingsButtonRect[2]=w;m_settingsButtonRect[3]=h;
        m_settingsButtonValid=(w>0&&h>0);
    }
    const float* settingsButtonRect() const { return m_settingsButtonRect; }
    bool settingsButtonValid() const { return m_settingsButtonValid; }
    void invalidateUiRects() { m_settingsButtonValid=false; }
    bool initAudio(int sr,int ch);
    void destroyAudio();
    void resetAudioRing();
    bool writeAudioSamples(const int16_t* samples,int count,int* written);
    int getAudioReadPos() const { return m_audioReadPos; }
    bool readAsset(const char* path,void** outBuf,long* outSize);
    bool readFile(const char* path,void** outBuf,long* outSize);
    std::string filesDir() const { return m_filesDir; }
    void setFilesDir(const std::string& d) { m_filesDir=d; }
    void requestMrFilePicker();
    void onMrFilePicked(const std::string& uri);
    void requestThemeFilePicker();
    void onThemeFilePicked(const std::string& uri);
    void requestValueInput(int idx);
    void requestHideValueInput();
    void consumePendingValueInput();
    bool consumeScriptReloadPending();
    std::string activeMrPath() const { return m_activeMrPath; }
    void resetToDefaultMr() { m_activeMrPath="assets/main.mr"; }
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
    std::vector<TouchButton> m_buttons;
    bool m_fnLatched=false;
    bool m_fnHeld=false;
    int64_t m_fnPressNs=0;
    int m_touchPid=-1;
    int m_touchPid2=-1;
    float m_touchX=0.0f,m_touchY=0.0f;
    float m_touchX1=0.0f,m_touchY1=0.0f;
    float m_touchX2=0.0f,m_touchY2=0.0f;
    bool m_touchDownEdge=false,m_touchUpEdge=false;
    bool m_restartPending=false;
    float m_pinchDist=0.0f;
    float m_pinchWheel=0.0f;
    SettingsState m_settings;
    SettingsLayout m_settingsLayout;
    ImportMenuState m_import;
    ImportLayout m_importLayout;
    // What is currently loaded, so the next pick only swaps one half.
    std::string m_themePath="themes/default.mr";
    std::string m_themeNode="use_default_theme";
    std::string m_enginePath="engines/atg-video-2/01_subaru_ej25_eh.mr";
    std::string m_engineNode;   // non empty only for old format files
    bool stageWrapperReload(const std::string& enginePath,
        const std::string& engineNode,const std::string& themePath,
        const std::string& themeNode);
    float m_settingsButtonRect[4]={0.0f,0.0f,0.0f,0.0f};
    bool m_settingsButtonValid=false;
    int m_sliderPid=-1;
    int m_sliderIdx=-1;
    // Typed values arrive on the Java UI thread and are staged for the
    // render thread, which owns the settings state.
    std::mutex m_valueInputMutex;
    int m_pendingValueIdx=-1;
    double m_pendingValue=0.0;
    SLObjectItf m_slEngine=nullptr, m_slOutputMix=nullptr, m_slPlayer=nullptr;
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
