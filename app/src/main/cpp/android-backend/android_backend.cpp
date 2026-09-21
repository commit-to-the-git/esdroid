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
#include <array>
#include <algorithm>

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
    // recreate a lost surface keep the context so gl objects survive
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
    // destroy only the surface so gl objects survive a pause
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
        // back closes an open list first then the menu
        if(AKeyEvent_getKeyCode(event)==AKEYCODE_BACK
            &&AKeyEvent_getAction(event)==AKEY_EVENT_ACTION_DOWN) {
            if(b.settingsOpen()) { b.closeSettings(); return 1; }
            if(b.importMenuOpen()) {
                auto& menu=b.importMenu();
                if(menu.themeListOpen) menu.themeListOpen=false;
                else if(menu.engineListOpen) menu.engineListOpen=false;
                else b.closeImportMenu();
                return 1;
            }
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
        if(b.importMenuOpen()) {
            b.handleImportMotion(event,am);
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
    // typed values arrive from the java ui thread and apply here
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

// tap toggles the latch hold is momentary
#define FN_TAP_NS 300000000LL

void AndroidBackend::pressButton(TouchButton* btn,int pid) {
    if(!btn) return;
    btn->held=true; btn->edge=true; btn->pointerId=pid;
    if(btn->key==VirtualKey::Fn){ m_fnHeld=true; m_fnPressNs=now_ns(); return; }
    int k=(int)effectiveKey(*btn);
    // 1x drops any speed hold
    if(k==(int)VirtualKey::None){
        for(int i=(int)VirtualKey::N1;i<=(int)VirtualKey::N5;++i) m_keyState[i]=false;
        return;
    }
    if(k>0&&k<(int)VirtualKey::Count){m_keyState[k]=true;m_keyEdge[k]=true;}
    if(k==(int)VirtualKey::Insert) openImportMenu();
}

void AndroidBackend::releaseButton(TouchButton* btn) {
    if(!btn) return;
    if(btn->key==VirtualKey::Fn){
        if(now_ns()-m_fnPressNs<FN_TAP_NS) m_fnLatched=!m_fnLatched;
        m_fnHeld=false; btn->held=false; btn->pointerId=-1;
        return;
    }
    btn->held=false; btn->pointerId=-1;
    // fn may have switched between press and release
    int keys[2]={(int)btn->key,(int)btn->altKey};
    for(int k:keys) if(k>0&&k<(int)VirtualKey::Count) m_keyState[k]=false;
}

void AndroidBackend::releaseAllButtons(int pid, bool cancel) {
    for(auto& b:m_buttons) {
        if(b.pointerId!=pid) continue;
        // action_cancel is a system grab not a tap
        if(cancel&&b.key==VirtualKey::Fn){ m_fnHeld=false; b.held=false; b.pointerId=-1; }
        else releaseButton(&b);
    }
}

// first free finger drives the mouse second starts a pinch
// extra fingers are ignored
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
    // re-anchor the drag at the pinch midpoint
    m_restartPending=true; m_touchUpEdge=true;
}
void AndroidBackend::moveEngineTouch(int pid,float x,float y) {
    if(pid==m_touchPid){ m_touchX1=x; m_touchY1=y; }
    else if(pid==m_touchPid2){ m_touchX2=x; m_touchY2=y; }
    else return;
    if(m_touchPid2!=-1) {
        // zoom tracks the finger distance ratio
        float dx=m_touchX1-m_touchX2,dy=m_touchY1-m_touchY2;
        float d=sqrtf(dx*dx+dy*dy);
        if(m_pinchDist>1.0f) m_pinchWheel+=500.0f*log2f(d/m_pinchDist);
        m_pinchDist=d;
    }
    // hold the position until the drag restart lands on the new anchor
    if(m_restartPending) return;
    if(m_touchPid2==-1){ m_touchX=m_touchX1; m_touchY=m_touchY1; return; }
    m_touchX=(m_touchX1+m_touchX2)*0.5f;
    m_touchY=(m_touchY1+m_touchY2)*0.5f;
}
void AndroidBackend::releaseEngineTouch(int pid,bool cancel) {
    if(pid==m_touchPid2) {
        // pinch over the remaining finger keeps panning
        m_touchPid2=-1; m_pinchDist=0.0f;
        if(!cancel){ m_restartPending=true; m_touchUpEdge=true; }
        return;
    }
    if(pid!=m_touchPid) return;
    if(m_touchPid2!=-1) {
        // engine finger left the pinch partner takes over
        m_touchPid=m_touchPid2; m_touchPid2=-1;
        m_touchX1=m_touchX2; m_touchY1=m_touchY2;
        m_pinchDist=0.0f;
        if(!cancel){ m_restartPending=true; m_touchUpEdge=true; }
        return;
    }
    m_touchPid=-1; m_pinchDist=0.0f;
    // both fingers can lift in one frame drop any leftover restart
    m_restartPending=false;
    // a system grab must not fire a click at the last position
    if(!cancel) m_touchUpEdge=true;
}

//
// settings panel

static const char* kSettingLabels[esdroid::kSettingCount] = {
    "VOLUME", "CONVOLUTION", "HI FREQ GAIN", "LO FREQ NOISE",
    "HI FREQ NOISE", "SIM FREQUENCY", "DYNO SPEED", "THROTTLE"
};

const char* AndroidBackend::settingLabel(int idx) {
    if(idx<0||idx>=esdroid::kSettingCount) return "";
    return kSettingLabels[idx];
}

static const char* settingUnit(int idx) {
    switch((SettingIndex)idx) {
    case SettingIndex::SimFrequency: return "HZ";
    case SettingIndex::DynoSpeed: return "RPM";
    default: return "%";
    }
}

// bare number no unit suffix so the value dialog edits pure digits
static void formatSettingNumber(char* out,int len,int idx,const SettingsState& s) {
    switch((SettingIndex)idx) {
    case SettingIndex::Volume:
        snprintf(out,len,"%d",(int)(s.volume*100.0f+0.5f)); return;
    case SettingIndex::Convolution:
        snprintf(out,len,"%d",(int)(s.convolution*100.0f+0.5f)); return;
    case SettingIndex::HiFreqGain:
        snprintf(out,len,"%d",(int)(s.hiFreqGain*100.0f+0.5f)); return;
    case SettingIndex::LoFreqNoise:
        snprintf(out,len,"%d",(int)(s.loFreqNoise*100.0f+0.5f)); return;
    case SettingIndex::HiFreqNoise:
        snprintf(out,len,"%d",(int)(s.hiFreqNoise*100.0f+0.5f)); return;
    case SettingIndex::SimFrequency:
        snprintf(out,len,"%d",(int)(s.simFrequency+0.5)); return;
    case SettingIndex::DynoSpeed:
        snprintf(out,len,"%d",(int)(s.dynoSpeedRpm+0.5)); return;
    case SettingIndex::Throttle:
        snprintf(out,len,"%d",(int)(s.throttlePct+0.5f)); return;
    default:
        snprintf(out,len,"0"); return;
    }
}

void AndroidBackend::formatSettingValue(char* out,int len,int idx) const {
    if(idx<0||idx>=esdroid::kSettingCount){snprintf(out,len,"?");return;}
    char num[24];
    formatSettingNumber(num,sizeof(num),idx,m_settings);
    const char* u=settingUnit(idx);
    if(u[0]=='%') snprintf(out,len,"%s%%",num);
    else snprintf(out,len,"%s %s",num,u);
}

static float clamp01f(float v) { return v<0.0f?0.0f:(v>1.0f?1.0f:v); }
static double clampd(double v,double lo,double hi) { return v<lo?lo:(v>hi?hi:v); }

// sim frequency slider is logarithmic or the low half of the range
// sits under the knob
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

// typed input arrives in the units the panel shows percent hz rpm
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
    // end a drag under the settings button with a clean lift
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
            // the track is thin so the whole row band grabs the slider
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

//
// import menu

// every entry was compiled and run through piranha on the host
// node holds the theme call or the set_engine target for old format
// engine files
static const MrAsset kImportThemes[] = {
    {"themes/default.mr", "use_default_theme", "DEFAULT"},
    {"themes/amateur.mr", "use_amateur_theme", "AMATEUR"},
    {"themes/bubble_gum.mr", "use_bubble_gum_theme", "BUBBLE GUM"},
    {"themes/minimalistic.mr", "use_minimalistic_theme", "MINIMALISTIC"},
    {"themes/night_vision.mr", "use_night_vision_theme", "NIGHT VISION"},
    {"themes/paper.mr", "use_paper_theme", "PAPER"},
};

static const MrAsset kImportEngines[] = {
    {"engines/atg-video-2/01_subaru_ej25_eh.mr", "", "DEFAULT"},
    {"engines/atg-video-1/01_honda_trx520.mr", "", "HONDA TRX520 ATV"},
    {"engines/atg-video-1/02_kohler_ch750.mr", "", "KOHLER CH750"},
    {"engines/atg-video-1/03_harley_davidson_shovelhead.mr", "", "HARLEY DAVIDSON SHOVELHEAD"},
    {"engines/atg-video-1/04_hayabusa.mr", "", "SUZUKI HAYABUSA I4"},
    {"engines/atg-video-1/05_honda_vtec.mr", "", "HONDA B18C5 VTEC I4"},
    {"engines/atg-video-1/06_subaru_ej25.mr", "", "SUBARU EJ25"},
    {"engines/atg-video-1/07_audi_i5.mr", "", "AUDI 2.3 INLINE 5"},
    {"engines/atg-video-1/08_radial_5.mr", "", "RADIAL 5"},
    {"engines/atg-video-2/02_subaru_ej25_uh.mr", "", "SUBARU EJ25 UH"},
    {"engines/atg-video-2/03_2jz.mr", "", "2JZ I6"},
    {"engines/atg-video-2/04_60_degree_v6.mr", "", "GENERIC 60 DEG. V6"},
    {"engines/atg-video-2/05_odd_fire_v6.mr", "", "GENERIC ODD-FIRE V6"},
    {"engines/atg-video-2/06_even_fire_v6.mr", "", "GENERIC EVEN-FIRE V6"},
    {"engines/atg-video-2/07_gm_ls.mr", "", "GM LS"},
    {"engines/atg-video-2/08_ferrari_f136_v8.mr", "", "FERRARI F136"},
    {"engines/atg-video-2/09_radial_9.mr", "", "RADIAL 9"},
    {"engines/atg-video-2/10_lfa_v10.mr", "", "1LR-GUE V10"},
    {"engines/atg-video-2/11_merlin_v12.mr", "", "MERLIN V-1650-9 V12"},
    {"engines/atg-video-2/12_ferrari_412_t2.mr", "", "FERRARI 412 T2 V12"},
    {"engines/audi/i5.mr", "audi_i5_2_2L", "AUDI 2.2 INLINE 5"},
    {"engines/bmw/M52B28.mr", "M52B28", "BMW M52B28"},
    {"engines/chevrolet/chev_truck_454.mr", "chev_truck_454", "CHEV. 454 V8"},
    {"engines/chevrolet/engine_03_for_e1.mr", "engine_03_for_e1", "CHEV. 454 V8 2"},
    {"engines/kohler/kohler_ch750.mr", "kohler_ch750", "KOHLER CH750 2"},
};

const MrAsset* AndroidBackend::importThemes(int* count) {
    if(count) *count=(int)(sizeof(kImportThemes)/sizeof(kImportThemes[0]));
    return kImportThemes;
}

const MrAsset* AndroidBackend::importEngines(int* count) {
    if(count) *count=(int)(sizeof(kImportEngines)/sizeof(kImportEngines[0]));
    return kImportEngines;
}

void AndroidBackend::openImportMenu() {
    if(m_import.open) return;
    m_import.open=true;
    const bool engineTouch=(m_touchPid!=-1||m_touchPid2!=-1);
    clearAllKeys();
    if(engineTouch) m_touchUpEdge=true;
    esdroid_wtflog("import menu: opened");
}

void AndroidBackend::closeImportMenu() {
    if(!m_import.open) return;
    m_import.open=false;
    m_import.themeListOpen=false;
    m_import.engineListOpen=false;
    m_import.scrollPid=-1;
    esdroid_wtflog("import menu: closed");
}

void AndroidBackend::layoutImportPanel(int sw,int sh) {
    if(sw<=0||sh<=0) return;
    auto& L=m_importLayout;
    const float pad=18.0f, headerH=64.0f, gutter=24.0f;
    float panelW=(float)sw*0.72f;
    if(panelW>980.0f) panelW=980.0f;
    float titleH=36.0f, boxH=50.0f, btnH=50.0f, gap=12.0f;
    float bodyH=titleH+gap+boxH+gap+btnH+gap+btnH;
    float panelH=headerH+pad+bodyH+pad;
    if(panelH>(float)sh*0.94f) {
        const float s=((float)sh*0.94f)/panelH;
        titleH*=s; boxH*=s; btnH*=s; gap*=s;
        bodyH=titleH+gap+boxH+gap+btnH+gap+btnH;
        panelH=headerH+pad+bodyH+pad;
    }
    const float px=((float)sw-panelW)*0.5f;
    const float py=((float)sh-panelH)*0.5f;
    L.panel[0]=px; L.panel[1]=py; L.panel[2]=panelW; L.panel[3]=panelH;
    const float closeW=92.0f, closeH=42.0f;
    L.close[0]=px+panelW-pad-closeW; L.close[1]=py+pad*0.5f+(headerH-closeH)*0.5f;
    L.close[2]=closeW; L.close[3]=closeH;
    const float innerX=px+pad, innerW=panelW-2.0f*pad;
    const float colW=(innerW-gutter)*0.5f;
    const float col2X=innerX+colW+gutter;
    float y=py+headerH+pad;
    L.rowH=boxH;
    L.listTop=y+titleH+gap+boxH;
    // the open list may not run past the bottom of the screen
    L.listH=(float)sh*0.55f;
    if(L.listTop+L.listH>(float)sh-pad) L.listH=(float)sh-pad-L.listTop;
    if(L.listH<boxH*2.0f) L.listH=boxH*2.0f;
    auto place=[&](float* r,float x){
        r[0]=x; r[1]=y+titleH+gap; r[2]=colW; r[3]=boxH;
    };
    place(L.themeBox,innerX);
    place(L.engineBox,col2X);
    auto placeBtn=[&](float* r,float x,float row){
        r[0]=x; r[1]=y+titleH+gap+boxH+gap+(float)row*(btnH+gap);
        r[2]=colW; r[3]=btnH;
    };
    placeBtn(L.themeLoad,innerX,0);
    placeBtn(L.themeImport,innerX,1);
    placeBtn(L.engineLoad,col2X,0);
    placeBtn(L.engineImport,col2X,1);
}

// builds the wrapper script that pairs an engine with a theme and
// stages it for a full reload old format engine files get a main node
// with set_engine only
bool AndroidBackend::stageWrapperReload(const std::string& enginePath,
        const std::string& engineNode,const std::string& themePath,
        const std::string& themeNode) {
    std::string wrapperPath=filesDir()+"/assets/imported_main.mr";
    std::string body;
    body+="import \"engine_sim.mr\"\n";
    body+="import \""+themePath+"\"\n";
    body+="import \""+enginePath+"\"\n";
    body+="\n";
    body+=themeNode+"()\n";
    if(!engineNode.empty())
        body+="public node main {\n    set_engine("+engineNode+"())\n}\n";
    body+="main()\n";
    FILE* out=fopen(wrapperPath.c_str(),"wb");
    if(out==nullptr) {
        esdroid_wtflog("import: could not write %s",wrapperPath.c_str());
        return false;
    }
    size_t written=fwrite(body.data(),1,body.size(),out);
    fclose(out);
    if(written!=body.size()) {
        esdroid_wtflog("import: short write on %s",wrapperPath.c_str());
        return false;
    }
    {
        std::lock_guard<std::mutex> lk(m_mrMutex);
        m_pendingMrPath=wrapperPath;
    }
    m_scriptReloadPending=true;
    m_enginePath=enginePath; m_engineNode=engineNode;
    m_themePath=themePath; m_themeNode=themeNode;
    esdroid_wtflog("import: staged %s + %s (wrapper %s)",
        enginePath.c_str(),themePath.c_str(),wrapperPath.c_str());
    return true;
}

static float clampListScroll(float scroll,int count,float rowH,float listH) {
    const float full=(float)count*rowH;
    if(full<=listH) return 0.0f;
    if(scroll<0.0f) return 0.0f;
    if(scroll>full-listH) return full-listH;
    return scroll;
}

int AndroidBackend::handleImportMotion(AInputEvent* event,int32_t am) {
    auto contains=[](const float* r,float x,float y) {
        return x>=r[0]&&x<r[0]+r[2]&&y>=r[1]&&y<r[1]+r[3];
    };
    int themeCount=0, engineCount=0;
    importThemes(&themeCount);
    importEngines(&engineCount);
    const auto& L=m_importLayout;
    auto listRect=[&](const float* box)->std::array<float,4> {
        // open lists draw straight below their selector box
        return {box[0],box[1]+box[3],box[2],L.listH};
    };
    // on open bring the selected row into view like a desktop dropdown
    auto revealSel=[&](int sel,int count,float& scroll){
        scroll=clampListScroll(scroll,count,L.rowH,L.listH);
        if(sel<0||sel>=count) return;
        const float top=(float)sel*L.rowH;
        if(top<scroll) scroll=top;
        else if(top+L.rowH>scroll+L.listH) scroll=top+L.rowH-L.listH;
        scroll=clampListScroll(scroll,count,L.rowH,L.listH);
    };
    if(am==AMOTION_EVENT_ACTION_DOWN||am==AMOTION_EVENT_ACTION_POINTER_DOWN) {
        const int32_t action=AMotionEvent_getAction(event);
        const int idx=(action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
            >>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        const float x=AMotionEvent_getX(event,idx);
        const float y=AMotionEvent_getY(event,idx);
        const int pid=AMotionEvent_getPointerId(event,idx);
        if(m_import.themeListOpen||m_import.engineListOpen) {
            const bool themeList=m_import.themeListOpen;
            const float* lr=listRect(themeList?L.themeBox:L.engineBox).data();
            if(contains(lr,x,y)) {
                m_import.scrollPid=pid;
                m_import.scrollDownY=y;
                m_import.scrollStart=themeList?m_import.themeScroll:m_import.engineScroll;
                m_import.tapEntry=(int)((y-lr[1]+m_import.scrollStart)/L.rowH);
                m_import.tapMoved=false;
                return 1;
            }
            m_import.themeListOpen=false;
            m_import.engineListOpen=false;
            // a tap on the other selector switches lists in one touch
            if(contains(L.themeBox,x,y)&&!themeList) {
                m_import.themeListOpen=true;
                revealSel(m_import.themeSel,themeCount,m_import.themeScroll);
            }
            else if(contains(L.engineBox,x,y)&&themeList) {
                m_import.engineListOpen=true;
                revealSel(m_import.engineSel,engineCount,m_import.engineScroll);
            }
            return 1;
        }
        if(contains(L.close,x,y)) { closeImportMenu(); return 1; }
        if(contains(L.themeBox,x,y)) {
            m_import.themeListOpen=true;
            revealSel(m_import.themeSel,themeCount,m_import.themeScroll);
            return 1;
        }
        if(contains(L.engineBox,x,y)) {
            m_import.engineListOpen=true;
            revealSel(m_import.engineSel,engineCount,m_import.engineScroll);
            return 1;
        }
        if(contains(L.themeLoad,x,y)) {
            closeImportMenu();
            // entry 0 is default so this also covers a reset to stock
            const MrAsset* t=&kImportThemes[
                m_import.themeSel>0?m_import.themeSel:0];
            stageWrapperReload(m_enginePath,m_engineNode,t->path,t->node);
            return 1;
        }
        if(contains(L.engineLoad,x,y)) {
            closeImportMenu();
            const MrAsset* e=&kImportEngines[
                m_import.engineSel>0?m_import.engineSel:0];
            stageWrapperReload(e->path,e->node,m_themePath,m_themeNode);
            return 1;
        }
        if(contains(L.themeImport,x,y)) {
            closeImportMenu();
            requestThemeFilePicker();
            return 1;
        }
        if(contains(L.engineImport,x,y)) {
            closeImportMenu();
            requestMrFilePicker();
            return 1;
        }
        return 1;
    }
    if(am==AMOTION_EVENT_ACTION_MOVE) {
        if(m_import.scrollPid!=-1) {
            const int pc=AMotionEvent_getPointerCount(event);
            for(int i=0;i<pc;++i) {
                if(AMotionEvent_getPointerId(event,i)!=m_import.scrollPid) continue;
                const float y=AMotionEvent_getY(event,i);
                if(y-m_import.scrollDownY>12.0f
                    ||y-m_import.scrollDownY<-12.0f) m_import.tapMoved=true;
                if(m_import.themeListOpen)
                    m_import.themeScroll=clampListScroll(
                        m_import.scrollStart-(y-m_import.scrollDownY),
                        themeCount,L.rowH,L.listH);
                else if(m_import.engineListOpen)
                    m_import.engineScroll=clampListScroll(
                        m_import.scrollStart-(y-m_import.scrollDownY),
                        engineCount,L.rowH,L.listH);
            }
        }
        return 1;
    }
    if(am==AMOTION_EVENT_ACTION_UP||am==AMOTION_EVENT_ACTION_POINTER_UP) {
        const int32_t action=AMotionEvent_getAction(event);
        const int idx=(action&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
            >>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        const int pid=AMotionEvent_getPointerId(event,idx);
        if(pid==m_import.scrollPid) {
            const float x=AMotionEvent_getX(event,idx);
            const float y=AMotionEvent_getY(event,idx);
            if(!m_import.tapMoved&&m_import.tapEntry>=0) {
                if(m_import.themeListOpen) {
                    const float* lr=listRect(L.themeBox).data();
                    if(contains(lr,x,y)&&m_import.tapEntry<themeCount) {
                        m_import.themeSel=m_import.tapEntry;
                        m_import.themeListOpen=false;
                    }
                }
                else if(m_import.engineListOpen) {
                    const float* lr=listRect(L.engineBox).data();
                    if(contains(lr,x,y)&&m_import.tapEntry<engineCount) {
                        m_import.engineSel=m_import.tapEntry;
                        m_import.engineListOpen=false;
                    }
                }
            }
            m_import.scrollPid=-1;
            m_import.tapEntry=-1;
        }
        return 1;
    }
    if(am==AMOTION_EVENT_ACTION_CANCEL) {
        m_import.scrollPid=-1;
        m_import.tapEntry=-1;
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
    // shrink buttons on short landscape screens so 8 fit the right edge
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
    // resetengine maps to return in the shim which reloads the script
    add("RELOAD","DYNO HOLD",rX,lY-3*(bh+gap),VirtualKey::ResetEngine,VirtualKey::DynoHold);
    add("1x","1/200x",pad,pad,VirtualKey::None,VirtualKey::N3);
    add("1/10x","1/500x",pad,pad+(bh+gap),VirtualKey::N1,VirtualKey::N4);
    add("1/100x","1/1000x",pad,pad+2*(bh+gap),VirtualKey::N2,VirtualKey::N5);
    add("FN",nullptr,pad,pad+3*(bh+gap),VirtualKey::Fn,VirtualKey::None);
    add("IMPORT","STEP",rX,pad,VirtualKey::Insert,VirtualKey::Right);
    add("EXIT","THR 10%",rX,pad+(bh+gap),VirtualKey::Escape,VirtualKey::Throttle10);
    add("CAMERA","THR 20%",rX,pad+2*(bh+gap),VirtualKey::Camera,VirtualKey::Throttle20);
    // osc page has no desktop key so run consumes it directly
    add("OSC PAGE","ROT 0",rX,pad+3*(bh+gap),VirtualKey::OscPage,VirtualKey::F3);

    layoutSettingsPanel(sw,sh);
    layoutImportPanel(sw,sh);
}

static int audio_peek_thunk(void* ctx,int16_t* dst,int count) {
    return static_cast<esdroid::AndroidBackend*>(ctx)->peekAudioSamples(dst,count);
}

static void audio_consume_thunk(void* ctx,int count) {
    static_cast<esdroid::AndroidBackend*>(ctx)->consumeAudioSamples(count);
}

static int audio_fill_thunk(void* ctx) {
    return static_cast<esdroid::AndroidBackend*>(ctx)->getAudioFill();
}

bool AndroidBackend::initAudio(int sr,int ch) {
    if(m_audioInited) return true;
    m_sampleRate=sr; m_channels=ch;
    {
        // safe to reset here the stream has not started yet
        // so no callback can be reading the ring
        m_audioRing.assign(kAudioRingSamples,0);
        m_audioWritePos.store(0,std::memory_order_relaxed);
        m_audioReadPos.store(0,std::memory_order_relaxed);
        m_audioDropPending.store(0,std::memory_order_relaxed);
    }
    AudioStreamHost host;
    host.ctx=this;
    host.peek=audio_peek_thunk;
    host.consume=audio_consume_thunk;
    host.fill=audio_fill_thunk;
    if(!m_audioStream.start(host,sr,ch)){
        return false;
    }
    // start at 120ms of margin the regulator tunes it from there
    m_fillTarget=(int)(sr*0.12);
    m_stableFrames=0;
    m_audioRetryFrames=0;
    m_audioStream.takeUnderrunCount();
    m_audioInited=true;
    return true;
}

void AndroidBackend::destroyAudio() {
    if(!m_audioInited) return;
    m_audioStream.stop();
    m_audioInited=false;
}

void AndroidBackend::resetAudioRing() {
    // ask the callback to drop the backlog then fade in from silence
    // the read pos stays owned by the callback so nothing races
    m_audioDropPending.store(0x7fffffff,std::memory_order_relaxed);
    m_audioStream.flush();
}

void AndroidBackend::updateAudioRegulator() {
    if(!m_audioInited) {
        // the first try can fail while the audio service is still booting
        // so give it another shot every few seconds
        if(++m_audioRetryFrames<240) return;
        m_audioRetryFrames=0;
        initAudio(m_sampleRate,m_channels);
        return;
    }
    // rebuild the stream when the device disconnected it
    m_audioStream.tick();
    const unsigned dry=m_audioStream.takeUnderrunCount();
    if(dry>0){
        // the audio thread ran dry so grow the margin 10ms per dry callback
        // no more than 40ms per frame so a storm cannot overshoot
        const int perDry=(int)(m_sampleRate*0.01);
        const int capped=dry>4u?4u:dry;
        m_fillTarget=std::min(m_fillTarget+perDry*capped,(int)(m_sampleRate*0.16));
        m_stableFrames=0;
        return;
    }
    if(m_stableFrames<240){ ++m_stableFrames; return; }
    // four seconds clean so drift back toward the 120ms floor
    m_fillTarget=std::max(m_fillTarget-4,(int)(m_sampleRate*0.12));
}

bool AndroidBackend::writeAudioSamples(const int16_t* samples,int count,int* written) {
    if(!m_audioInited){if(written)*written=0;return false;}
    if(written)*written=count;
    if(count<=0) return true;
    // a block larger than the ring keeps only its tail
    if((uint32_t)count>kAudioRingSamples){
        samples+=count-(int)kAudioRingSamples;
        count=(int)kAudioRingSamples;
    }
    const uint32_t w=m_audioWritePos.load(std::memory_order_relaxed);
    const uint32_t r=m_audioReadPos.load(std::memory_order_acquire);
    const uint32_t space=kAudioRingSamples-(w-r);
    // keep the newest samples when the ring is short on room
    if((uint32_t)count>space){
        samples+=count-(int)space;
        count=(int)space;
    }
    if(count<=0) return true;
    const uint32_t start=w&kAudioRingMask;
    const uint32_t first=
        (uint32_t)count<kAudioRingSamples-start
            ?(uint32_t)count
            :kAudioRingSamples-start;
    memcpy(&m_audioRing[start],samples,(size_t)first*sizeof(int16_t));
    if((uint32_t)count>first)
        memcpy(&m_audioRing[0],samples+first,
            (size_t)((uint32_t)count-first)*sizeof(int16_t));
    // the pos goes up after the data so the reader never sees empty bytes
    m_audioWritePos.store(w+(uint32_t)count,std::memory_order_release);
    return true;
}

int AndroidBackend::getAudioFill() const {
    // samples waiting in the fifo
    const uint32_t w=m_audioWritePos.load(std::memory_order_acquire);
    const uint32_t r=m_audioReadPos.load(std::memory_order_acquire);
    return (int)(uint32_t)(w-r);
}

void AndroidBackend::dropOldestAudio(int count) {
    // the callback applies the drop on its next pass
    // so the read pos is only ever moved by the audio thread
    if(count<=0) return;
    m_audioDropPending.fetch_add(count,std::memory_order_relaxed);
}

int AndroidBackend::peekAudioSamples(int16_t* dst,int count) {
    // copy from the read pos without moving it
    if(count<=0||!dst) return 0;
    // fold a pending drop request in before the copy
    const int drop=m_audioDropPending.exchange(0,std::memory_order_relaxed);
    if(drop>0){
        const uint32_t wd=m_audioWritePos.load(std::memory_order_acquire);
        const uint32_t rd=m_audioReadPos.load(std::memory_order_relaxed);
        const uint32_t availd=wd-rd;
        const uint32_t d=
            (uint32_t)drop>availd?availd:(uint32_t)drop;
        if(d>0) m_audioReadPos.fetch_add(d,std::memory_order_release);
    }
    const uint32_t w=m_audioWritePos.load(std::memory_order_acquire);
    const uint32_t r=m_audioReadPos.load(std::memory_order_relaxed);
    const uint32_t avail=w-r;
    if(avail==0) return 0;
    if((uint32_t)count>avail) count=(int)avail;
    const uint32_t start=r&kAudioRingMask;
    const uint32_t first=
        (uint32_t)count<kAudioRingSamples-start
            ?(uint32_t)count
            :kAudioRingSamples-start;
    memcpy(dst,&m_audioRing[start],(size_t)first*sizeof(int16_t));
    if((uint32_t)count>first)
        memcpy(dst+first,&m_audioRing[0],
            (size_t)((uint32_t)count-first)*sizeof(int16_t));
    return count;
}

void AndroidBackend::consumeAudioSamples(int count) {
    if(count<=0) return;
    const uint32_t w=m_audioWritePos.load(std::memory_order_acquire);
    const uint32_t r=m_audioReadPos.load(std::memory_order_relaxed);
    const uint32_t avail=w-r;
    if((uint32_t)count>avail) count=(int)avail;
    if(count>0)
        m_audioReadPos.fetch_add((uint32_t)count,std::memory_order_release);
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
    esdroid_wtflog("import: opening the engine file picker");
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
        // never detach with a pending exception it is undefined behavior
        if (env->ExceptionCheck()) {
            esdroid_wtflog("openFilePicker threw a Java exception");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    }
    env->DeleteLocalRef(cls);
    if (attached) vm->DetachCurrentThread();
}

void AndroidBackend::requestThemeFilePicker() {
    if (s_app == nullptr || s_app->activity == nullptr) return;
    esdroid_wtflog("import: opening the theme file picker");
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
    jmethodID method = env->GetMethodID(cls, "openThemePicker", "()V");
    if (method != nullptr) {
        env->CallVoidMethod(activity, method);
        if (env->ExceptionCheck()) {
            esdroid_wtflog("openThemePicker threw a Java exception");
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
    }
    env->DeleteLocalRef(cls);
    if (attached) vm->DetachCurrentThread();
}

// builds a main script for an imported engine community files only
// contain the engine definition so the boilerplate gets wrapped around
// them a file that already calls use_default_theme loads as is
static std::string buildImportedMainScript(const std::string& enginePath,
        const std::string& themePath, const std::string& themeNode) {
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
    // strip a mr suffix the import line re-adds it
    if (base.size() > 3 && base.compare(base.size() - 3, 3, ".mr") == 0)
        base = base.substr(0, base.size() - 3);

    std::string wrapperPath = dir + "/imported_main.mr";
    std::string body =
        "import \"engine_sim.mr\"\n"
        "import \"" + themePath + "\"\n"
        "import \"" + base + ".mr\"\n"
        "\n"
        + themeNode + "()\n"
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

    // stage the script and raise the flag the render thread stops the
    // game so android_main can rebuild it with the new script
    std::string mainScript = buildImportedMainScript(path, m_themePath, m_themeNode);
    {
        std::lock_guard<std::mutex> lk(m_mrMutex);
        m_pendingMrPath = mainScript;
    }
    m_scriptReloadPending = true;
    if (mainScript != path) {
        // wrapped import remember it as the current engine so a later
        // load theme keeps it
        m_enginePath = "imported";
        m_engineNode = "";
    }
    esdroid_wtflog("onMrFilePicked: %s (main script: %s)", path.c_str(), mainScript.c_str());
}

// theme files publish a use_*_theme node the name is parsed out of
// the file for the wrapper call
void AndroidBackend::onThemeFilePicked(const std::string& path) {
    if (path.empty()) return;
    std::string node = "use_default_theme";
    FILE* fp = fopen(path.c_str(), "rb");
    if (fp != nullptr) {
        char probe[65536];
        size_t n = fread(probe, 1, sizeof(probe) - 1, fp);
        fclose(fp);
        probe[n] = 0;
        const char* at = strstr(probe, "node use_");
        if (at != nullptr) {
            char name[96];
            size_t i = 0;
            for (at += 5; i < sizeof(name) - 1; ++i) {
                char c = at[i];
                if (c == 0) break;
                if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                    || (c >= '0' && c <= '9') || c == '_')) break;
                name[i] = c;
            }
            name[i] = 0;
            if (strstr(name, "_theme") != nullptr && i > 5) node = name;
        }
    }
    size_t slash = path.rfind('/');
    size_t nameStart = (slash == std::string::npos) ? 0 : slash + 1;
    std::string base = path.substr(nameStart);
    if (base.size() > 3 && base.compare(base.size() - 3, 3, ".mr") == 0)
        base = base.substr(0, base.size() - 3);
    esdroid_wtflog("onThemeFilePicked: %s (theme node %s)", path.c_str(), node.c_str());
    stageWrapperReload(m_enginePath, m_engineNode, base, node);
}

bool AndroidBackend::consumeScriptReloadPending() {
    if (!m_scriptReloadPending.exchange(false)) return false;
    // the only place the active script path changes
    std::lock_guard<std::mutex> lk(m_mrMutex);
    if (!m_pendingMrPath.empty()) m_activeMrPath = m_pendingMrPath;
    esdroid_wtflog("script reload consumed, active script: %s", m_activeMrPath.c_str());
    return true;
}

// a real dialog owns its own window so its buttons and the ime work
// over the native input queue
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
        formatSettingNumber(current, sizeof(current), idx, m_settings);
        char labelBuf[64];
        snprintf(labelBuf, sizeof(labelBuf), "%s (%s)",
            settingLabel(idx), settingUnit(idx));
        jstring label = env->NewStringUTF(labelBuf);
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
Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnThemePicked(JNIEnv* env, jobject thiz, jstring path);

extern "C" JNIEXPORT void JNICALL
Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnValueInput(JNIEnv* env, jobject thiz, jint index, jdouble value);

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    g_javaVM=vm;
    // register natives by name the package underscore needs the _1
    // mangling in exported symbols
    JNIEnv* env=nullptr;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6)==JNI_OK && env!=nullptr) {
        jclass cls=env->FindClass("com/esdroid/engine_sim/ESDroidActivity");
        if (cls!=nullptr) {
            JNINativeMethod methods[]={
                {"nativeOnFilePicked","(Ljava/lang/String;)V",
                 (void*)Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnFilePicked},
                {"nativeOnThemePicked","(Ljava/lang/String;)V",
                 (void*)Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnThemePicked},
                {"nativeOnValueInput","(ID)V",
                 (void*)Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnValueInput},
            };
            env->RegisterNatives(cls,methods,3);
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

// jni callback the _1 encodes the underscore in the package name
extern "C" JNIEXPORT void JNICALL
Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnFilePicked(JNIEnv* env, jobject thiz, jstring path) {
    if (path == nullptr) return;
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    if (pathStr != nullptr) {
        esdroid::AndroidBackend::instance().onMrFilePicked(std::string(pathStr));
        env->ReleaseStringUTFChars(path, pathStr);
    }
}

// same callback for the theme picker both run on the java ui thread
extern "C" JNIEXPORT void JNICALL
Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnThemePicked(JNIEnv* env, jobject thiz, jstring path) {
    if (path == nullptr) return;
    const char* pathStr = env->GetStringUTFChars(path, nullptr);
    if (pathStr != nullptr) {
        esdroid::AndroidBackend::instance().onThemeFilePicked(std::string(pathStr));
        env->ReleaseStringUTFChars(path, pathStr);
    }
}

// typed value from the settings dialog staged for the render thread
extern "C" JNIEXPORT void JNICALL
Java_com_esdroid_engine_1sim_ESDroidActivity_nativeOnValueInput(JNIEnv* env, jobject thiz, jint index, jdouble value) {
    esdroid::AndroidBackend::instance().stageValueInput((int)index, (double)value);
}
