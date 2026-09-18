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

#if defined(__ANDROID__)
extern "C" void esdroid_wtflog(const char*,...);
#endif
#include "android_backend.h"
JavaVM* g_javaVM=nullptr;
#include "touch_ui.h"
#include <android/log.h>
#include <android/window.h>
#include <android/native_window.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <cstdio>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"ESDroid",__VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN,"ESDroid",__VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR,"ESDroid",__VA_ARGS__))

namespace esdroid {
android_app* AndroidBackend::s_app=nullptr;
AndroidBackend::AndroidBackend() {}
AndroidBackend& AndroidBackend::instance() { static AndroidBackend i; return i; }

void AndroidBackend::setAndroidApp(android_app* app) {
    s_app=app;
    instance().m_assetManager=(app&&app->activity)?app->activity->assetManager:nullptr;
    if(app&&app->activity&&app->activity->internalDataPath)
        instance().m_filesDir=app->activity->internalDataPath;
}
void AndroidBackend::setAssetManager(AAssetManager* mgr) { instance().m_assetManager=mgr; }

static EGLConfig pickEglConfig(EGLDisplay d) {
    const EGLint a[]={EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,
        EGL_DEPTH_SIZE,0,EGL_STENCIL_SIZE,0,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_NONE};
    EGLConfig c=nullptr; EGLint n=0;
    if(!eglChooseConfig(d,a,&c,1,&n)||n<1){LOGE("eglChooseConfig failed");return nullptr;}
    return c;
}

bool AndroidBackend::initWindow(int w,int h) {
    if(s_app==nullptr||s_app->window==nullptr){LOGE("initWindow: ANativeWindow null");esdroid_wtflog("initWindow: ANativeWindow is null");return false;}
    if(m_eglDisplay==EGL_NO_DISPLAY) {
        m_eglDisplay=eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if(m_eglDisplay==EGL_NO_DISPLAY){LOGE("eglGetDisplay failed");esdroid_wtflog("initWindow: eglGetDisplay failed");return false;}
        EGLint major=0,minor=0;
        if(!eglInitialize(m_eglDisplay,&major,&minor)){LOGE("eglInitialize failed");esdroid_wtflog("initWindow: eglInitialize failed");return false;}
        LOGI("EGL %d.%d",major,minor);
        m_eglConfig=pickEglConfig(m_eglDisplay);
        if(!m_eglConfig){esdroid_wtflog("initWindow: eglChooseConfig failed");return false;}
        EGLint ctx[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
        m_eglContext=eglCreateContext(m_eglDisplay,m_eglConfig,EGL_NO_CONTEXT,ctx);
        if(m_eglContext==EGL_NO_CONTEXT){LOGE("eglCreateContext: 0x%x",eglGetError());esdroid_wtflog("initWindow: eglCreateContext failed 0x%x",eglGetError());return false;}
    }
    // Recreate the surface if it was lost (app pause/resume). The context is
    // kept alive so GL objects survive.
    if(m_eglSurface==EGL_NO_SURFACE) {
        EGLint format=0;
        eglGetConfigAttrib(m_eglDisplay,m_eglConfig,EGL_NATIVE_VISUAL_ID,&format);
        ANativeWindow_setBuffersGeometry(s_app->window,0,0,format);
        m_eglSurface=eglCreateWindowSurface(m_eglDisplay,m_eglConfig,s_app->window,nullptr);
        if(m_eglSurface==EGL_NO_SURFACE){LOGE("eglCreateWindowSurface: 0x%x",eglGetError());esdroid_wtflog("initWindow: eglCreateWindowSurface failed 0x%x",eglGetError());return false;}
    }
    if(!eglMakeCurrent(m_eglDisplay,m_eglSurface,m_eglSurface,m_eglContext)){LOGE("eglMakeCurrent: 0x%x",eglGetError());esdroid_wtflog("initWindow: eglMakeCurrent failed 0x%x",eglGetError());return false;}
    eglQuerySurface(m_eglDisplay,m_eglSurface,EGL_WIDTH,&m_screenWidth);
    eglQuerySurface(m_eglDisplay,m_eglSurface,EGL_HEIGHT,&m_screenHeight);
    LOGI("EGL surface: %dx%d",m_screenWidth,m_screenHeight);
    esdroid_wtflog("initWindow: EGL surface ready %dx%d",m_screenWidth,m_screenHeight);
    layoutButtons(m_screenWidth,m_screenHeight);
    return true;
}

void AndroidBackend::destroyWindow() {
    // Only destroy the surface. Keeping the display and context alive means
    // GL objects survive an app pause so rendering can continue on resume.
    if(m_eglDisplay!=EGL_NO_DISPLAY) {
        eglMakeCurrent(m_eglDisplay,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
        if(m_eglSurface!=EGL_NO_SURFACE) eglDestroySurface(m_eglDisplay,m_eglSurface);
        m_eglSurface=EGL_NO_SURFACE;
    }
}

void AndroidBackend::swapBuffers() {
    if(m_eglDisplay!=EGL_NO_DISPLAY&&m_eglSurface!=EGL_NO_SURFACE)
        eglSwapBuffers(m_eglDisplay,m_eglSurface);
}

static int32_t handle_input_event(android_app* app, AInputEvent* event) {
    int32_t type=AInputEvent_getType(event);
    auto& b=esdroid::AndroidBackend::instance();
    if(type==AINPUT_EVENT_TYPE_KEY) {
        // While the settings panel is up, BACK closes it instead of the app.
        if(AKeyEvent_getKeyCode(event)==AKEYCODE_BACK
            &&AKeyEvent_getAction(event)==AKEY_EVENT_ACTION_DOWN
            &&b.settingsOpen()) {
            b.closeSettings();
            return 1;
        }
        return 0;
    }
    if(type==AINPUT_EVENT_TYPE_MOTION) {
        int32_t action=AMotionEvent_getAction(event);
        int32_t am=action&AMOTION_EVENT_ACTION_MASK;
        if(b.settingsOpen()) {
            b.handleSettingsMotion(event,am);
            return 1;
        }
        switch(am) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN: {
            int idx=(action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            float x=AMotionEvent_getX(event,idx), y=AMotionEvent_getY(event,idx);
            int pid=AMotionEvent_getPointerId(event,idx);
            if(b.settingsButtonValid()) {
                const float* r=b.settingsButtonRect();
                if(x>=r[0]&&x<r[0]+r[2]&&y>=r[1]&&y<r[1]+r[3]) {
                    b.openSettings();
                    return 1;
                }
            }
            auto* btn=b.hitTestButton(x,y);
            if(btn){b.pressButton(btn,pid);return 1;}
            b.pressEngineTouch(pid,x,y);
            return 1;
        }
        case AMOTION_EVENT_ACTION_MOVE: {
            int pc=AMotionEvent_getPointerCount(event);
            for(int i=0;i<pc;++i) {
                int pid=AMotionEvent_getPointerId(event,i);
                b.moveEngineTouch(pid,AMotionEvent_getX(event,i),AMotionEvent_getY(event,i));
            }
            return 1;
        }
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP: {
            int idx=(action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            int pid=AMotionEvent_getPointerId(event,idx);
            b.releaseEngineTouch(pid,false);
            b.releaseAllButtons(pid); return 1;
        }
        case AMOTION_EVENT_ACTION_CANCEL: {
            int pc=AMotionEvent_getPointerCount(event);
            for(int i=0;i<pc;++i) {
                int pid=AMotionEvent_getPointerId(event,i);
                b.releaseEngineTouch(pid,true);
                b.releaseAllButtons(pid,true);
            }
            return 1;
        }
        }
    }
    return 0;
}

static const char* appCmdName(int32_t cmd) {
    switch (cmd) {
    case APP_CMD_INPUT_CHANGED: return "INPUT_CHANGED";
    case APP_CMD_INIT_WINDOW: return "INIT_WINDOW";
    case APP_CMD_TERM_WINDOW: return "TERM_WINDOW";
    case APP_CMD_WINDOW_RESIZED: return "WINDOW_RESIZED";
    case APP_CMD_WINDOW_REDRAW_NEEDED: return "WINDOW_REDRAW_NEEDED";
    case APP_CMD_CONTENT_RECT_CHANGED: return "CONTENT_RECT_CHANGED";
    case APP_CMD_GAINED_FOCUS: return "GAINED_FOCUS";
    case APP_CMD_LOST_FOCUS: return "LOST_FOCUS";
    case APP_CMD_CONFIG_CHANGED: return "CONFIG_CHANGED";
    case APP_CMD_LOW_MEMORY: return "LOW_MEMORY";
    case APP_CMD_START: return "START";
    case APP_CMD_RESUME: return "RESUME";
    case APP_CMD_SAVE_STATE: return "SAVE_STATE";
    case APP_CMD_PAUSE: return "PAUSE";
    case APP_CMD_STOP: return "STOP";
    case APP_CMD_DESTROY: return "DESTROY";
    default: return "OTHER";
    }
}

static void handle_app_cmd(android_app* app, int32_t cmd) {
    auto& b=esdroid::AndroidBackend::instance();
    esdroid_wtflog("app cmd: %s (%d)", appCmdName(cmd), (int)cmd);
    switch(cmd) {
    case APP_CMD_INIT_WINDOW:
        if(app->window) {
            if(!b.initWindow(0,0)) esdroid_wtflog("initWindow FAILED during INIT_WINDOW");
        }
        break;
    case APP_CMD_TERM_WINDOW: b.destroyWindow(); break;
    case APP_CMD_DESTROY: b.requestQuit(); break;
    }
}

bool AndroidBackend::pollEvents() {
    if(s_app==nullptr) return false;
    int events=0; android_poll_source* source=nullptr;
    while(ALooper_pollAll(0,nullptr,&events,reinterpret_cast<void**>(&source))>=0) {
        if(source) source->process(s_app,source);
        if(s_app->destroyRequested!=0){m_shouldQuit=true;break;}
    }
    // Input arrives through app->onInputEvent inside source->process().
    // Typed values came in on the Java UI thread and are applied here.
    consumePendingValueInput();
    return !m_shouldQuit;
}

TouchButton* AndroidBackend::hitTestButton(float x,float y) {
    for(auto& b:m_buttons) if(x>=b.x&&x<b.x+b.w&&y>=b.y&&y<b.y+b.h) return &b;
    return nullptr;
}

static int64_t now_ns() {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts);
    return (int64_t)ts.tv_sec*1000000000LL+(int64_t)ts.tv_nsec;
}

// Tap toggles the latch, hold is momentary.
#define FN_TAP_NS 300000000LL

void AndroidBackend::pressButton(TouchButton* btn,int pid) {
    if(!btn) return;
    btn->held=true; btn->edge=true; btn->pointerId=pid;
    if(btn->key==VirtualKey::Fn){ m_fnHeld=true; m_fnPressNs=now_ns(); return; }
    int k=(int)effectiveKey(*btn);
    // "1x": no key held means full speed, so drop any speed hold instead.
    if(k==(int)VirtualKey::None){
        for(int i=(int)VirtualKey::N1;i<=(int)VirtualKey::N5;++i) m_keyState[i]=false;
        return;
    }
    if(k>0&&k<(int)VirtualKey::Count){m_keyState[k]=true;m_keyEdge[k]=true;}
    if(k==(int)VirtualKey::Insert) requestMrFilePicker();
}

void AndroidBackend::releaseButton(TouchButton* btn) {
    if(!btn) return;
    if(btn->key==VirtualKey::Fn){
        if(now_ns()-m_fnPressNs<FN_TAP_NS) m_fnLatched=!m_fnLatched;
        m_fnHeld=false; btn->held=false; btn->pointerId=-1;
        return;
    }
    btn->held=false; btn->pointerId=-1;
    // FN may have switched between press and release.
    int keys[2]={(int)btn->key,(int)btn->altKey};
    for(int k:keys) if(k>0&&k<(int)VirtualKey::Count) m_keyState[k]=false;
}

void AndroidBackend::releaseAllButtons(int pid, bool cancel) {
    for(auto& b:m_buttons) {
        if(b.pointerId!=pid) continue;
        // ACTION_CANCEL is a system grab, not a finger lift, so FN must not
        // count it as a tap.
        if(cancel&&b.key==VirtualKey::Fn){ m_fnHeld=false; b.held=false; b.pointerId=-1; }
        else releaseButton(&b);
    }
}

// The first finger that misses every button drives the mouse, a second
// one starts a pinch. Extra fingers are ignored so a resting thumb cannot
// yank the view.
void AndroidBackend::pressEngineTouch(int pid,float x,float y) {
    if(m_touchPid==-1) {
        m_touchPid=pid;
        m_touchX1=x; m_touchY1=y;
        m_touchX=x; m_touchY=y;
        m_touchDownEdge=true; m_touchUpEdge=false;
        return;
    }
    if(m_touchPid2!=-1) return;
    m_touchPid2=pid;
    m_touchX2=x; m_touchY2=y;
    float dx=x-m_touchX1,dy=y-m_touchY1;
    m_pinchDist=sqrtf(dx*dx+dy*dy);
    // The reported position jumps to the midpoint, so the drag has to be
    // re-anchored there or the view would lurch.
    m_restartPending=true; m_touchUpEdge=true;
}
void AndroidBackend::moveEngineTouch(int pid,float x,float y) {
    if(pid==m_touchPid){ m_touchX1=x; m_touchY1=y; }
    else if(pid==m_touchPid2){ m_touchX2=x; m_touchY2=y; }
    else return;
    if(m_touchPid2!=-1) {
        // Zoom tracks the finger distance ratio: spreading the fingers
        // doubles the zoom when the distance doubles.
        float dx=m_touchX1-m_touchX2,dy=m_touchY1-m_touchY2;
        float d=sqrtf(dx*dx+dy*dy);
        if(m_pinchDist>1.0f) m_pinchWheel+=500.0f*log2f(d/m_pinchDist);
        m_pinchDist=d;
    }
    // Hold the position until a pending drag restart lands it on the new
    // anchor; reporting the raw jump would yank the view.
    if(m_restartPending) return;
    if(m_touchPid2==-1){ m_touchX=m_touchX1; m_touchY=m_touchY1; return; }
    m_touchX=(m_touchX1+m_touchX2)*0.5f;
    m_touchY=(m_touchY1+m_touchY2)*0.5f;
}
void AndroidBackend::releaseEngineTouch(int pid,bool cancel) {
    if(pid==m_touchPid2) {
        // Pinch over, the remaining finger keeps panning.
        m_touchPid2=-1; m_pinchDist=0.0f;
        if(!cancel){ m_restartPending=true; m_touchUpEdge=true; }
        return;
    }
    if(pid!=m_touchPid) return;
    if(m_touchPid2!=-1) {
        // Engine finger left, the pinch partner takes over.
        m_touchPid=m_touchPid2; m_touchPid2=-1;
        m_touchX1=m_touchX2; m_touchY1=m_touchY2;
        m_pinchDist=0.0f;
        if(!cancel){ m_restartPending=true; m_touchUpEdge=true; }
        return;
    }
    m_touchPid=-1; m_pinchDist=0.0f;
    // Both fingers can lift inside one frame; a leftover restart would
    // leave a phantom drag behind.
    m_restartPending=false;
    // A system grab must not fire a click at the last position.
    if(!cancel) m_touchUpEdge=true;
}

// ------------------------------------------------------------------
// Settings panel

static const char* kSettingLabels[esdroid::kSettingCount] = {
    "VOLUME", "CONVOLUTION", "HI FREQ GAIN", "LO FREQ NOISE",
    "HI FREQ NOISE", "SIM FREQUENCY", "DYNO SPEED", "THROTTLE"
};

const char* AndroidBackend::settingLabel(int idx) {
    if(idx<0||idx>=esdroid::kSettingCount) return "";
    return kSettingLabels[idx];
}

void AndroidBackend::formatSettingValue(char* out,int len,int idx) const {
    switch((SettingIndex)idx) {
    case SettingIndex::SimFrequency:
        snprintf(out,len,"%d HZ",(int)(m_settings.simFrequency+0.5));
        return;
    case SettingIndex::DynoSpeed:
        snprintf(out,len,"%d RPM",(int)(m_settings.dynoSpeedRpm+0.5));
        return;
    case SettingIndex::Volume:
        snprintf(out,len,"%d%%",(int)(m_settings.volume*100.0f+0.5f));
        return;
    case SettingIndex::Convolution:
        snprintf(out,len,"%d%%",(int)(m_settings.convolution*100.0f+0.5f));
        return;
    case SettingIndex::HiFreqGain:
        snprintf(out,len,"%d%%",(int)(m_settings.hiFreqGain*100.0f+0.5f));
        return;
    case SettingIndex::LoFreqNoise:
        snprintf(out,len,"%d%%",(int)(m_settings.loFreqNoise*100.0f+0.5f));
        return;
    case SettingIndex::HiFreqNoise:
        snprintf(out,len,"%d%%",(int)(m_settings.hiFreqNoise*100.0f+0.5f));
        return;
    case SettingIndex::Throttle:
        snprintf(out,len,"%d%%",(int)(m_settings.throttlePct+0.5f));
        return;
    default:
        snprintf(out,len,"?");
        return;
    }
}

static float clamp01f(float v) { return v<0.0f?0.0f:(v>1.0f?1.0f:v); }
static double clampd(double v,double lo,double hi) { return v<lo?lo:(v>hi?hi:v); }

// The 400 Hz..400 kHz sim frequency range is logarithmic on the slider,
// or the whole low half of the range would sit under the knob.
static const double kSimFreqMin=400.0, kSimFreqMax=400000.0;

float AndroidBackend::settingSliderT(int idx) const {
    switch((SettingIndex)idx) {
    case SettingIndex::Volume: return clamp01f(m_settings.volume);
    case SettingIndex::Convolution: return clamp01f(m_settings.convolution);
    case SettingIndex::HiFreqGain: return clamp01f(m_settings.hiFreqGain);
    case SettingIndex::LoFreqNoise: return clamp01f(m_settings.loFreqNoise);
    case SettingIndex::HiFreqNoise: return clamp01f(m_settings.hiFreqNoise);
    case SettingIndex::SimFrequency: {
        const double v=clampd(m_settings.simFrequency,kSimFreqMin,kSimFreqMax);
        return (float)(log10(v/kSimFreqMin)/log10(kSimFreqMax/kSimFreqMin));
    }
    case SettingIndex::DynoSpeed: {
        const double lo=m_settings.dynoMinRpm, hi=m_settings.dynoMaxRpm;
        if(hi<=lo) return 0.0f;
        return (float)clampd((m_settings.dynoSpeedRpm-lo)/(hi-lo),0.0,1.0);
    }
    case SettingIndex::Throttle:
        return clamp01f(m_settings.throttlePct/100.0f);
    default:
        return 0.0f;
    }
}

void AndroidBackend::setSettingFromSlider(int idx,float t) {
    t=clamp01f(t);
    switch((SettingIndex)idx) {
    case SettingIndex::Volume: m_settings.volume=t; break;
    case SettingIndex::Convolution: m_settings.convolution=t; break;
    case SettingIndex::HiFreqGain: m_settings.hiFreqGain=t; break;
    case SettingIndex::LoFreqNoise: m_settings.loFreqNoise=t; break;
    case SettingIndex::HiFreqNoise: m_settings.hiFreqNoise=t; break;
    case SettingIndex::SimFrequency:
        m_settings.simFrequency=kSimFreqMin*pow(1000.0,(double)t);
        break;
    case SettingIndex::DynoSpeed:
        m_settings.dynoSpeedRpm=m_settings.dynoMinRpm
            +(double)t*(m_settings.dynoMaxRpm-m_settings.dynoMinRpm);
        break;
    case SettingIndex::Throttle:
        m_settings.throttlePct=t*100.0f;
        break;
    default:
        return;
    }
    m_settings.dirty|= (1u<<idx);
}

// Typed input arrives in the units the panel shows: percent, Hz, RPM.
void AndroidBackend::setSettingTyped(int idx,double typed) {
    switch((SettingIndex)idx) {
    case SettingIndex::Volume: m_settings.volume=(float)clampd(typed/100.0,0.0,1.0); break;
    case SettingIndex::Convolution: m_settings.convolution=(float)clampd(typed/100.0,0.0,1.0); break;
    case SettingIndex::HiFreqGain: m_settings.hiFreqGain=(float)clampd(typed/100.0,0.0,1.0); break;
    case SettingIndex::LoFreqNoise: m_settings.loFreqNoise=(float)clampd(typed/100.0,0.0,1.0); break;
    case SettingIndex::HiFreqNoise: m_settings.hiFreqNoise=(float)clampd(typed/100.0,0.0,1.0); break;
    case SettingIndex::SimFrequency:
        m_settings.simFrequency=clampd(typed,kSimFreqMin,kSimFreqMax);
        break;
    case SettingIndex::DynoSpeed:
        m_settings.dynoSpeedRpm=clampd(typed,m_settings.dynoMinRpm,m_settings.dynoMaxRpm);
        break;
    case SettingIndex::Throttle:
        m_settings.throttlePct=(float)clampd(typed,0.0,100.0);
        break;
    default:
        return;
    }
    m_settings.dirty|= (1u<<idx);
}

void AndroidBackend::openSettings() {
    if(m_settings.open) return;
    m_settings.open=true;
    const bool engineTouch=(m_touchPid!=-1||m_touchPid2!=-1);
    clearAllKeys();
    // A drag that was running under the SETTINGS button has to end with a
    // clean lift, or the engine mouse would stay pressed forever.
    if(engineTouch) m_touchUpEdge=true;
    esdroid_wtflog("settings: opened");
}

void AndroidBackend::closeSettings() {
    if(!m_settings.open) return;
    m_settings.open=false;
    m_sliderPid=-1; m_sliderIdx=-1;
    requestHideValueInput();
    esdroid_wtflog("settings: closed");
}

void AndroidBackend::layoutSettingsPanel(int sw,int sh) {
    if(sw<=0||sh<=0) return;
    auto& L=m_settingsLayout;
    const float pad=18.0f, gap=8.0f, headerH=64.0f;
    float panelW=(float)sw*0.72f;
    if(panelW>980.0f) panelW=980.0f;
    float rowH=64.0f;
    const float availH=(float)sh*0.94f-headerH-2.0f*pad-(float)(esdroid::kSettingCount-1)*gap;
    if(availH<(float)esdroid::kSettingCount*rowH)
        rowH=availH/(float)esdroid::kSettingCount;
    if(rowH<30.0f) rowH=30.0f;
    const float panelH=headerH+2.0f*pad+(float)esdroid::kSettingCount*rowH
        +(float)(esdroid::kSettingCount-1)*gap;
    const float px=((float)sw-panelW)*0.5f;
    const float py=((float)sh-panelH)*0.5f;
    L.panel[0]=px; L.panel[1]=py; L.panel[2]=panelW; L.panel[3]=panelH;
    L.rowH=rowH;
    const float closeW=92.0f, closeH=42.0f;
    L.close[0]=px+panelW-pad-closeW; L.close[1]=py+pad*0.5f+ (headerH-closeH)*0.5f;
    L.close[2]=closeW; L.close[3]=closeH;
    const float labelW=214.0f, valueW=152.0f, trackH=10.0f, cgap=14.0f;
    const float innerX=px+pad, innerW=panelW-2.0f*pad;
    const float trackW=innerW-labelW-valueW-3.0f*cgap;
    float y=py+headerH+pad;
    for(int i=0;i<esdroid::kSettingCount;++i,y+=rowH+gap) {
        const float cy=y+rowH*0.5f;
        L.track[i][0]=innerX+labelW+cgap; L.track[i][1]=cy-trackH*0.5f;
        L.track[i][2]=trackW; L.track[i][3]=trackH;
        float valueH=rowH*0.72f;
        if(valueH>46.0f) valueH=46.0f;
        if(valueH<28.0f) valueH=28.0f;
        L.value[i][0]=innerX+labelW+cgap+trackW+cgap;
        L.value[i][1]=y+(rowH-valueH)*0.5f;
        L.value[i][2]=valueW; L.value[i][3]=valueH;
    }
}

int AndroidBackend::handleSettingsMotion(AInputEvent* event,int32_t am) {
    auto contains=[](const float* r,float x,float y) {
        return x>=r[0]&&x<r[0]+r[2]&&y>=r[1]&&y<r[1]+r[3];
    };
    if(am==AMOTION_EVENT_ACTION_DOWN||am==AMOTION_EVENT_ACTION_POINTER_DOWN) {
        const int32_t action=AMotionEvent_getAction(event);
        const int idx=(action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
            >>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        const float x=AMotionEvent_getX(event,idx);
        const float y=AMotionEvent_getY(event,idx);
        const int pid=AMotionEvent_getPointerId(event,idx);
        const auto& L=m_settingsLayout;
        if(contains(L.close,x,y)) { closeSettings(); return 1; }
        for(int i=0;i<esdroid::kSettingCount;++i)
            if(contains(L.value[i],x,y)) { requestValueInput(i); return 1; }
        for(int i=0;i<esdroid::kSettingCount;++i) {
            const float* t=L.track[i];
            // The track is thin, so the whole row band grabs the slider.
            if(x>=t[0]-24.0f&&x<t[0]+t[2]+24.0f
                &&y>=t[1]-L.rowH*0.5f&&y<t[1]+t[3]+L.rowH*0.5f) {
                m_sliderPid=pid; m_sliderIdx=i;
                float tt=(x-t[0])/t[2];
                setSettingFromSlider(i,clamp01f(tt));
                return 1;
            }
        }
        return 1;
    }
    if(am==AMOTION_EVENT_ACTION_MOVE) {
        if(m_sliderPid!=-1) {
            const int pc=AMotionEvent_getPointerCount(event);
            for(int i=0;i<pc;++i) {
                if(AMotionEvent_getPointerId(event,i)!=m_sliderPid) continue;
                const float* t=m_settingsLayout.track[m_sliderIdx];
                float tt=(AMotionEvent_getX(event,i)-t[0])/t[2];
                setSettingFromSlider(m_sliderIdx,clamp01f(tt));
            }
        }
        return 1;
    }
    if(am==AMOTION_EVENT_ACTION_UP||am==AMOTION_EVENT_ACTION_POINTER_UP) {
        const int32_t action=AMotionEvent_getAction(event);
        const int idx=(action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
            >>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        const int pid=AMotionEvent_getPointerId(event,idx);
        if(pid==m_sliderPid){ m_sliderPid=-1; m_sliderIdx=-1; }
        return 1;
    }
    if(am==AMOTION_EVENT_ACTION_CANCEL) {
        m_sliderPid=-1; m_sliderIdx=-1;
        return 1;
    }
    return 1;
}

void AndroidBackend::stageValueInput(int idx,double value) {
    std::lock_guard<std::mutex> lk(m_valueInputMutex);
    m_pendingValueIdx=idx;
    m_pendingValue=value;
}

void AndroidBackend::consumePendingValueInput() {
    int idx=-1; double value=0.0;
    {
        std::lock_guard<std::mutex> lk(m_valueInputMutex);
        if(m_pendingValueIdx<0) return;
        idx=m_pendingValueIdx; value=m_pendingValue;
        m_pendingValueIdx=-1; m_pendingValue=0.0;
    }
    setSettingTyped(idx,value);
    esdroid_wtflog("settings: typed %s = %f",settingLabel(idx),value);
}

bool AndroidBackend::isKeyDown(VirtualKey k) const {
    int i=(int)k; if(i<=0||i>=(int)VirtualKey::Count) return false;
    return m_keyState[i];
}

bool AndroidBackend::processKeyDown(VirtualKey k) {
    int i=(int)k; if(i<=0||i>=(int)VirtualKey::Count) return false;
    if(m_keyEdge[i]){m_keyEdge[i]=false;return true;}
    return false;
}

void AndroidBackend::clearAllKeys() {
    for(int i=0;i<(int)VirtualKey::Count;++i){m_keyState[i]=false;m_keyEdge[i]=false;}
    for(auto&b:m_buttons){b.held=false;b.edge=false;b.pointerId=-1;}
    m_fnLatched=false; m_fnHeld=false;
    m_touchPid=-1; m_touchPid2=-1;
    m_touchDownEdge=false; m_touchUpEdge=false;
    m_restartPending=false; m_pinchDist=0.0f; m_pinchWheel=0.0f;
    m_sliderPid=-1; m_sliderIdx=-1;
}

void AndroidBackend::layoutButtons(int sw,int sh) {
    m_buttons.clear();
    if(sw<=0||sh<=0) return;
    const float pad=18;
    float bw=165,bh=90,gap=12;
    // The right edge stacks 4 buttons on top of 4 buttons. On short landscape
    // screens (< 828px) that would overlap, so buttons and gaps shrink to fit.
    const float needed=8*bh+7*gap+2*pad;
    if(needed>(float)sh) {
        const float scale=((float)sh-2*pad)/(needed-2*pad);
        bh*=scale; gap*=scale;
    }
    auto add=[&](const char* l,const char* al,float x,float y,VirtualKey k,VirtualKey ak){
        m_buttons.push_back({l,al,x,y,bw,bh,k,ak,false,false,-1});
    };
    float lY=sh-pad-bh;
    add("STARTER","ZOOM IN",pad,lY,VirtualKey::Starter,VirtualKey::ZoomIn);
    add("IGNITION","ZOOM OUT",pad,lY-(bh+gap),VirtualKey::Ignition,VirtualKey::ZoomOut);
    add("THROTTLE","ROT +",pad,lY-2*(bh+gap),VirtualKey::Throttle,VirtualKey::F1);
    add("CLUTCH","ROT -",pad,lY-3*(bh+gap),VirtualKey::Clutch,VirtualKey::F2);
    float rX=sw-pad-bw;
    add("SHIFT +","VLYR UP",rX,lY,VirtualKey::ShiftUp,VirtualKey::ViewLayerUp);
    add("SHIFT -","VLYR DWN",rX,lY-(bh+gap),VirtualKey::ShiftDown,VirtualKey::ViewLayerDown);
    add("PAUSE","DYNO",rX,lY-2*(bh+gap),VirtualKey::Pause,VirtualKey::Dyno);
    // ResetEngine maps to ysKey::Code::Return in the shim: run() reloads the
    // engine script on its edge (loadScript()).
    add("RELOAD","DYNO HOLD",rX,lY-3*(bh+gap),VirtualKey::ResetEngine,VirtualKey::DynoHold);
    add("1x","1/200x",pad,pad,VirtualKey::None,VirtualKey::N3);
    add("1/10x","1/500x",pad,pad+(bh+gap),VirtualKey::N1,VirtualKey::N4);
    add("1/100x","1/1000x",pad,pad+2*(bh+gap),VirtualKey::N2,VirtualKey::N5);
    add("FN",nullptr,pad,pad+3*(bh+gap),VirtualKey::Fn,VirtualKey::None);
    add("IMPORT","STEP",rX,pad,VirtualKey::Insert,VirtualKey::Right);
    add("EXIT","THR 10%",rX,pad+(bh+gap),VirtualKey::Escape,VirtualKey::Throttle10);
    add("CAMERA","THR 20%",rX,pad+2*(bh+gap),VirtualKey::Camera,VirtualKey::Throttle20);
    // No desktop key pages the oscilloscope focus, so this button is consumed
    // directly in EngineSimApplication::run().
    add("OSC PAGE","ROT 0",rX,pad+3*(bh+gap),VirtualKey::OscPage,VirtualKey::F3);

    layoutSettingsPanel(sw,sh);
}

static void sl_buffer_callback(SLAndroidSimpleBufferQueueItf bq, void* ctx) {
    auto* backend = static_cast<esdroid::AndroidBackend*>(ctx);
    if (!backend) return;

    int bufSamps = backend->m_sampleRate / 20 * backend->m_channels; // 50ms buffer
    std::vector<int16_t>* buf = &backend->m_slBuffers[backend->m_slNextBuffer];

    // Copy from ring buffer. Like desktop DirectSound: always read from ring
    // buffer, even if underrun. Old data gets replayed briefly until new audio
    // arrives. This is better than silence. Two memcpys instead of a per
    // sample modulo loop: this runs on the audio thread.
    {
        std::lock_guard<std::mutex> lk(backend->m_audioMutex);
        const int rs = (int)backend->m_audioRing.size();
        if (rs <= 0) {
            buf->assign(bufSamps, 0);
        } else {
            int& rp = backend->m_audioReadPos;
            int n = bufSamps < rs ? bufSamps : rs;
            int first = rs - rp; if (first > n) first = n;
            memcpy(buf->data(), &backend->m_audioRing[rp],
                (size_t)first * sizeof(int16_t));
            if (n > first) memcpy(buf->data() + first, &backend->m_audioRing[0],
                (size_t)(n - first) * sizeof(int16_t));
            if (bufSamps > n) memset(buf->data() + n, 0,
                (size_t)(bufSamps - n) * sizeof(int16_t));
            rp = (rp + n) % rs;
        }
    }

    (*bq)->Enqueue(bq, buf->data(), bufSamps * sizeof(int16_t));
    backend->m_slNextBuffer = 1 - backend->m_slNextBuffer;
}

bool AndroidBackend::initAudio(int sr,int ch) {
    if(m_audioInited) return true;
    m_sampleRate=sr; m_channels=ch;
    SLresult r;
    r=slCreateEngine(&m_slEngine,0,nullptr,0,nullptr,nullptr);
    if(r!=SL_RESULT_SUCCESS){LOGE("slCreateEngine failed");return false;}
    (*m_slEngine)->Realize(m_slEngine,SL_BOOLEAN_FALSE);
    (*m_slEngine)->GetInterface(m_slEngine,SL_IID_ENGINE,&m_slEngineItf);
    (*m_slEngineItf)->CreateOutputMix(m_slEngineItf,&m_slOutputMix,0,nullptr,nullptr);
    (*m_slOutputMix)->Realize(m_slOutputMix,SL_BOOLEAN_FALSE);
    SLDataLocator_AndroidSimpleBufferQueue lbq={SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};
    SLDataFormat_PCM fmt={SL_DATAFORMAT_PCM,(SLuint32)ch,(SLuint32)(sr*1000),
        SL_PCMSAMPLEFORMAT_FIXED_16,SL_PCMSAMPLEFORMAT_FIXED_16,
        (ch==1)?SL_SPEAKER_FRONT_CENTER:(SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT),
        SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource src={&lbq,&fmt};
    SLDataLocator_OutputMix lom={SL_DATALOCATOR_OUTPUTMIX,m_slOutputMix};
    SLDataSink sink={&lom,nullptr};
    SLInterfaceID ids[]={SL_IID_BUFFERQUEUE,SL_IID_VOLUME};
    SLboolean req[]={SL_BOOLEAN_TRUE,SL_BOOLEAN_FALSE};
    (*m_slEngineItf)->CreateAudioPlayer(m_slEngineItf,&m_slPlayer,&src,&sink,2,ids,req);
    (*m_slPlayer)->Realize(m_slPlayer,SL_BOOLEAN_FALSE);
    (*m_slPlayer)->GetInterface(m_slPlayer,SL_IID_PLAY,&m_slPlayItf);
    (*m_slPlayer)->GetInterface(m_slPlayer,SL_IID_BUFFERQUEUE,&m_slQueue);
    (*m_slQueue)->RegisterCallback(m_slQueue,sl_buffer_callback,this);
    int bufSamps=sr/20*ch;
    m_slBuffers[0].assign(bufSamps,0);
    m_slBuffers[1].assign(bufSamps,0);
    m_slNextBuffer=0;
    (*m_slQueue)->Enqueue(m_slQueue,m_slBuffers[0].data(),bufSamps*sizeof(int16_t));
    m_slNextBuffer=1;
    (*m_slQueue)->Enqueue(m_slQueue,m_slBuffers[1].data(),bufSamps*sizeof(int16_t));
    m_slNextBuffer=0;
    m_audioRing.assign(sr*ch,0);
    m_audioWritePos=0; m_audioReadPos=0;
    (*m_slPlayItf)->SetPlayState(m_slPlayItf,SL_PLAYSTATE_PLAYING);
    m_audioInited=true;
    LOGI("OpenSL ES audio: %dHz %dch",sr,ch);
    return true;
}

void AndroidBackend::destroyAudio() {
    if(m_slPlayItf) (*m_slPlayItf)->SetPlayState(m_slPlayItf,SL_PLAYSTATE_STOPPED);
    if(m_slPlayer) (*m_slPlayer)->Destroy(m_slPlayer);
    if(m_slOutputMix) (*m_slOutputMix)->Destroy(m_slOutputMix);
    if(m_slEngine) (*m_slEngine)->Destroy(m_slEngine);
    m_slEngine=nullptr;m_slEngineItf=nullptr;m_slOutputMix=nullptr;
    m_slPlayer=nullptr;m_slPlayItf=nullptr;m_slQueue=nullptr;
    m_audioInited=false;
}

void AndroidBackend::resetAudioRing() {
    // Silence the ring buffer and line the write head up with the read head.
    // Called when pausing so the last second of audio does not loop forever,
    // and when resuming so fresh audio starts playing right away.
    std::lock_guard<std::mutex> lk(m_audioMutex);
    if(!m_audioRing.empty()) memset(m_audioRing.data(),0,m_audioRing.size()*sizeof(int16_t));
    m_audioWritePos=m_audioReadPos;
}

bool AndroidBackend::writeAudioSamples(const int16_t* samples,int count,int* written) {
    if(!m_audioInited){if(written)*written=0;return false;}
    std::lock_guard<std::mutex> lk(m_audioMutex);
    const int rs=(int)m_audioRing.size();
    if(written)*written=count;
    if(rs<=0||count<=0) return true;
    // A block larger than the ring keeps only its tail.
    if(count>=rs){ samples+=count-rs; count=rs; m_audioWritePos=m_audioReadPos; }
    // The writer may lap the reader; the reader then advances past exactly
    // the overwritten samples, same as the old per-sample chase.
    const int first=count<rs-m_audioWritePos?count:rs-m_audioWritePos;
    memcpy(&m_audioRing[m_audioWritePos],samples,(size_t)first*sizeof(int16_t));
    const int rest=count-first;
    if(rest>0) memcpy(&m_audioRing[0],samples+first,(size_t)rest*sizeof(int16_t));
    const int valid=(m_audioWritePos-m_audioReadPos+rs)%rs;
    int drop=count-(rs-valid)+1;
    if(drop<0) drop=0;
    m_audioWritePos=(m_audioWritePos+count)%rs;
    if(drop>0) m_audioReadPos=(m_audioReadPos+drop)%rs;
    return true;
}

bool AndroidBackend::readAsset(const char* path,void** outBuf,long* outSize) {
    if(!m_assetManager) return false;
    AAsset* a=AAssetManager_open(m_assetManager,path,AASSET_MODE_BUFFER);
    if(!a){LOGW("readAsset: not found: %s",path);return false;}
    off_t len=AAsset_getLength(a);
    void* buf=malloc(len+1);
    AAsset_read(a,buf,len);
    ((char*)buf)[len]=0;
    AAsset_close(a);
    *outBuf=buf; *outSize=(long)len;
    return true;
}

bool AndroidBackend::readFile(const char* path,void** outBuf,long* outSize) {
    int fd=open(path,O_RDONLY);
    if(fd<0) return false;
    struct stat st; if(fstat(fd,&st)!=0){close(fd);return false;}
    off_t len=st.st_size;
    void* buf=malloc(len+1);
    ssize_t n=read(fd,buf,len);
    close(fd);
    if(n!=len){free(buf);return false;}
    ((char*)buf)[len]=0;
    *outBuf=buf; *outSize=(long)len;
    return true;
}

void AndroidBackend::requestMrFilePicker() {
    if (s_app == nullptr || s_app->activity == nullptr) return;
    esdroid_wtflog("IMPORT pressed, opening the file picker");
    JNIEnv* env = nullptr;
    JavaVM* vm = g_javaVM;
    if (vm == nullptr) return;
    bool attached = false;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    jobject activity = s_app->activity->clazz;
    if (activity == nullptr) { if (attached) vm->DetachCurrentThread(); return; }
    jclass cls = env->GetObjectClass(activity);
    jmethodID method = env->GetMethodID(cls, "openFilePicker", "()V");
    if (method != nullptr) {
        env->CallVoidMethod(activity, method);
        // Never detach with a pending exception, it is undefined behavior.
        if (env->ExceptionCheck()) {
            esdroid_wtflog("openFilePicker threw a Java exception");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    }
    env->DeleteLocalRef(cls);
    if (attached) vm->DetachCurrentThread();
}

// Builds the main script for an imported engine. Community engine files
// only contain the engine definition, so the same boilerplate main.mr uses
// (language import, theme import, use_default_theme(), main()) is wrapped
// around them. A file that already calls use_default_theme() is treated as
// a self contained main script and loaded as is.
static std::string buildImportedMainScript(const std::string& enginePath) {
    bool selfContained = false;
    FILE* fp = fopen(enginePath.c_str(), "rb");
    if (fp != nullptr) {
        char probe[65536];
        size_t n = fread(probe, 1, sizeof(probe) - 1, fp);
        fclose(fp);
        probe[n] = 0;
        if (strstr(probe, "use_default_theme") != nullptr) selfContained = true;
    }

    if (selfContained) {
        esdroid_wtflog("import: file already has use_default_theme, loading it directly");
        return enginePath;
    }

    size_t slash = enginePath.rfind('/');
    std::string dir = (slash == std::string::npos) ? "." : enginePath.substr(0, slash);
    size_t nameStart = (slash == std::string::npos) ? 0 : slash + 1;
    std::string base = enginePath.substr(nameStart);
    // Strip a .mr suffix; the import line re-adds it.
    if (base.size() > 3 && base.compare(base.size() - 3, 3, ".mr") == 0)
        base = base.substr(0, base.size() - 3);

    std::string wrapperPath = dir + "/imported_main.mr";
    std::string body =
        "import \"engine_sim.mr\"\n"
        "import \"themes/default.mr\"\n"
        "import \"" + base + ".mr\"\n"
        "\n"
        "use_default_theme()\n"
        "main()\n";

    FILE* out = fopen(wrapperPath.c_str(), "wb");
    if (out == nullptr) {
        esdroid_wtflog("import: could not write %s, loading the engine file directly",
                       wrapperPath.c_str());
        return enginePath;
    }
    size_t written = fwrite(body.data(), 1, body.size(), out);
    fclose(out);
    if (written != body.size()) {
        esdroid_wtflog("import: short write on %s, loading the engine file directly",
                       wrapperPath.c_str());
        return enginePath;
    }

    esdroid_wtflog("import: wrote wrapper %s", wrapperPath.c_str());
    return wrapperPath;
}

void AndroidBackend::onMrFilePicked(const std::string& path) {
    if (path.empty()) return;

    // Stage the main script and raise the flag. The render thread picks both
    // up on its next pass, which stops the game so android_main can rebuild
    // it with the new script. A flag is used instead of a synthetic key press
    // because the button list is rebuilt when the window comes back, which
    // would eat the key edge.
    std::string mainScript = buildImportedMainScript(path);
    {
        std::lock_guard<std::mutex> lk(m_mrMutex);
        m_pendingMrPath = mainScript;
    }
    m_scriptReloadPending = true;
    esdroid_wtflog("onMrFilePicked: %s (main script: %s)", path.c_str(), mainScript.c_str());
}

bool AndroidBackend::consumeScriptReloadPending() {
    if (!m_scriptReloadPending.exchange(false)) return false;
    // Runs on the render thread only, this is the single place the active
    // script path changes after startup.
    std::lock_guard<std::mutex> lk(m_mrMutex);
    if (!m_pendingMrPath.empty()) m_activeMrPath = m_pendingMrPath;
    esdroid_wtflog("script reload consumed, active script: %s", m_activeMrPath.c_str());
    return true;
}

// The value entry dialog is a real Android Dialog: it owns its own window,
// so its buttons and the IME work on top of the native input queue.
void AndroidBackend::requestValueInput(int idx) {
    if (s_app == nullptr || s_app->activity == nullptr) return;
    JNIEnv* env = nullptr;
    JavaVM* vm = g_javaVM;
    if (vm == nullptr) return;
    bool attached = false;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    jobject activity = s_app->activity->clazz;
    if (activity == nullptr) { if (attached) vm->DetachCurrentThread(); return; }
    jclass cls = env->GetObjectClass(activity);
    jmethodID method = env->GetMethodID(cls, "showValueInput", "(ILjava/lang/String;Ljava/lang/String;)V");
    if (method != nullptr) {
        char current[32];
        formatSettingValue(current, sizeof(current), idx);
        jstring label = env->NewStringUTF(settingLabel(idx));
        jstring value = env->NewStringUTF(current);
        if (label != nullptr && value != nullptr) {
            env->CallVoidMethod(activity, method, (jint)idx, label, value);
            if (env->ExceptionCheck()) {
                esdroid_wtflog("showValueInput threw a Java exception");
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
        }
        if (label != nullptr) env->DeleteLocalRef(label);
        if (value != nullptr) env->DeleteLocalRef(value);
    }
    env->DeleteLocalRef(cls);
    if (attached) vm->DetachCurrentThread();
}

void AndroidBackend::requestHideValueInput() {
    if (s_app == nullptr || s_app->activity == nullptr) return;
    JNIEnv* env = nullptr;
    JavaVM* vm = g_javaVM;
    if (vm == nullptr) return;
    bool attached = false;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        attached = true;
    }
    jobject activity = s_app->activity->clazz;
    if (activity == nullptr) { if (attached) vm->DetachCurrentThread(); return; }
    jclass cls = env->GetObjectClass(activity);
    jmethodID method = env->GetMethodID(cls, "hideValueInput", "()V");
    if (method != nullptr) {
        env->CallVoidMethod(activity, method);
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    }
    env->DeleteLocalRef(cls);
    if (attached) vm->DetachCurrentThread();
}

extern "C" void esdroid_install_app_callbacks(struct android_app* app) {
    app->onAppCmd=handle_app_cmd;
    app->onInputEvent=handle_input_event;
}

} // namespace esdroid

extern "C" void esdroid_pre_main(struct android_app* app) {
    esdroid::esdroid_install_app_callbacks(app);
}

extern "C" JNIEXPORT void JNICALL
Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnFilePicked(JNIEnv* env, jobject thiz, jstring path);

extern "C" JNIEXPORT void JNICALL
Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnValueInput(JNIEnv* env, jobject thiz, jint index, jdouble value);

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    g_javaVM=vm;
    // Bind the java facing methods explicitly. The package name contains an
    // underscore, which JNI name mangling encodes as _1, and a plain symbol
    // name with a raw underscore is invisible to the VM. Registering by
    // method name here does not depend on symbol name mangling at all.
    JNIEnv* env=nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6)==JNI_OK && env!=nullptr) {
        jclass cls=env->FindClass("com/esdroid/engine_sim/ESDroidActivity");
        if (cls!=nullptr) {
            JNINativeMethod methods[]={
                {"nativeOnFilePicked","(Ljava/lang/String;)V",
                 (void*)Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnFilePicked},
                {"nativeOnValueInput","(ID)V",
                 (void*)Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnValueInput},
            };
            env->RegisterNatives(cls,methods,2);
            if (env->ExceptionCheck()) env->ExceptionClear();
            env->DeleteLocalRef(cls);
        } else if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }
    }
    return JNI_VERSION_1_6;
}

extern "C" const char* esdroid_get_files_dir() {
    static std::string dir=esdroid::AndroidBackend::instance().filesDir();
    return dir.c_str();
}

extern "C" void esdroid_swap_buffers() {
    if (esdroid::AndroidBackend::instance().isWindowReady())
        esdroid::AndroidBackend::instance().renderTouchUI();
    esdroid::AndroidBackend::instance().swapBuffers();
}

void esdroid::AndroidBackend::initTouchUI() {
    if (m_touchUI) return;
    m_touchUI = new TouchUI();
    m_touchUI->initialize(m_screenWidth, m_screenHeight);
}

void esdroid::AndroidBackend::renderTouchUI() {
    if (m_touchUI) m_touchUI->render();
}

void esdroid::AndroidBackend::resizeTouchUI() {
    if (m_touchUI) m_touchUI->resize(m_screenWidth, m_screenHeight);
}

// JNI callback from Java when a file is picked. The package name contains
// an underscore so the exported name must encode it as _1, otherwise the
// VM cannot find the symbol (JNI_OnLoad also registers it by name).
extern "C" JNIEXPORT void JNICALL
Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnFilePicked(JNIEnv* env, jobject thiz, jstring path) {
    if (path == nullptr) return;
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    if (pathStr != nullptr) {
        esdroid::AndroidBackend::instance().onMrFilePicked(std::string(pathStr));
        env->ReleaseStringUTFChars(path, pathStr);
    }
}

// Typed value from the settings dialog. Runs on the Java UI thread, so it
// is staged and applied by the render thread on its next poll.
extern "C" JNIEXPORT void JNICALL
Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnValueInput(JNIEnv* env, jobject thiz, jint index, jdouble value) {
    esdroid::AndroidBackend::instance().stageValueInput((int)index, (double)value);
}
