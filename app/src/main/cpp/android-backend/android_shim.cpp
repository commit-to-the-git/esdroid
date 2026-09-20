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

#include "android_shim.h"
#include "android_backend.h"
#include "yds_audio_parameters.h"
#include "yds_input_device.h"
#include "yds_monitor.h"
#include "yds_window_event_handler.h"
#include "yds_error_handler.h"
#include <android/log.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,"ESDroid",__VA_ARGS__))

static int vkToYsKey(esdroid::VirtualKey vk) {
    using K=esdroid::VirtualKey; using Y=ysKey::Code;
    switch(vk) {
    case K::Escape: return (int)Y::Escape;
    case K::Return: return (int)Y::Return;
    case K::Tab: return (int)Y::Tab;
    case K::Insert: return (int)Y::Insert;
    case K::N1: return (int)Y::N1;
    case K::N2: return (int)Y::N2;
    case K::N3: return (int)Y::N3;
    case K::N4: return (int)Y::N4;
    case K::N5: return (int)Y::N5;
    case K::F1: return (int)Y::F1;
    case K::F2: return (int)Y::F2;
    case K::F3: return (int)Y::F3;
    case K::F: return (int)Y::F;
    case K::Right: return (int)Y::Right;
    case K::ViewLayerUp: return (int)Y::M;
    case K::ViewLayerDown: return (int)Y::OEM_Comma;
    case K::Dyno: return (int)Y::D;
    case K::DynoHold: return (int)Y::H;
    case K::Throttle10: return (int)Y::W;
    case K::Throttle20: return (int)Y::E;
    case K::Starter: return (int)Y::S;
    case K::Ignition: return (int)Y::A;
    case K::Throttle: return (int)Y::R;
    case K::Clutch: return (int)Y::Shift;      // clutch fully depressed
    case K::ShiftUp: return (int)Y::Up;         // shift gear up
    case K::ShiftDown: return (int)Y::Down;     // shift gear down
    case K::Camera: return (int)Y::Tab;          // cycle camera angle
    case K::Pause: return (int)Y::Pause;         // pause the simulation
    case K::ResetEngine: return (int)Y::Return; // reload/reset engine
    default: return 0;
    }
}

ysAndroidWindowSystem *ysAndroidWindowSystem::s_instance=nullptr;
ysAndroidWindowSystem::ysAndroidWindowSystem() : ysWindowSystem(Platform::Android) { s_instance=this; }
ysAndroidWindowSystem::~ysAndroidWindowSystem() {
    s_instance=nullptr;
    // deleteallwindows only marks windows closed free what is left or
    // restarts leak
    while (GetWindowCount() > 0) {
        DeleteWindow(GetWindow(GetWindowCount() - 1));
    }
}
ysError ysAndroidWindowSystem::NewWindow(ysWindow **window) { *window=new ysAndroidWindow(); return YDS_ERROR_RETURN(ysError::None); }

ysAndroidWindow::ysAndroidWindow() : ysWindow(Platform::Android) { m_open=true; }
ysAndroidWindow::~ysAndroidWindow() {}
bool ysAndroidWindow::IsOpen() const { return m_open && !esdroid::AndroidBackend::instance().shouldQuit(); }
int ysAndroidWindow::GetScreenWidth() const { return esdroid::AndroidBackend::instance().screenWidth(); }
int ysAndroidWindow::GetScreenHeight() const { return esdroid::AndroidBackend::instance().screenHeight(); }
const int ysAndroidWindow::GetGameWidth() const { return esdroid::AndroidBackend::instance().screenWidth(); }
const int ysAndroidWindow::GetGameHeight() const { return esdroid::AndroidBackend::instance().screenHeight(); }
// flip touch y-down to the ui y-up like the desktop client-to-local
void ysAndroidWindow::ScreenToLocal(int &x, int &y) const {
    y = GetScreenHeight() - y;
}
void ysAndroidWindow::SetScreenSize(int w,int h) { m_width=w; m_height=h; }
void ysAndroidWindow::SetWindowSize(int w,int h) { m_width=w; m_height=h; }

namespace { class ysAndroidMonitor : public ysMonitor { public: ysAndroidMonitor() : ysMonitor(Platform::Android) { m_originx=0;m_originy=0; } }; }
ysMonitor *ysAndroidWindowSystem::MonitorFromWindow(ysWindow *) { static ysAndroidMonitor s; return &s; }
ysMonitor *ysAndroidWindowSystem::NewMonitor() { return new ysAndroidMonitor(); }
void ysAndroidWindowSystem::ProcessMessages() { esdroid::AndroidBackend::instance().pollEvents(); }

ysAndroidAudioSystem *ysAndroidAudioSystem::s_instance=nullptr;
ysAndroidAudioSystem::ysAndroidAudioSystem() : ysAudioSystem(API::DirectSound8) { s_instance=this; }
ysAndroidAudioSystem::~ysAndroidAudioSystem() { s_instance=nullptr; }
ysError ysAndroidAudioSystem::CreateBuffer(const ysAudioParameters *p, SampleOffset s, ysAudioBuffer **b) { *b=new ysAndroidAudioBuffer(); (*b)->Initialize(s,*p); return YDS_ERROR_RETURN(ysError::None); }
ysError ysAndroidAudioSystem::ConnectDevice(ysWindow *, ysAudioDevice **d) { *d=new ysAndroidAudioDevice(); return YDS_ERROR_RETURN(ysError::None); }

ysAndroidAudioDevice::ysAndroidAudioDevice() : ysAudioDevice(API::DirectSound8) {}
ysAndroidAudioDevice::~ysAndroidAudioDevice() {}
ysError ysAndroidAudioDevice::CreateSource(const ysAudioParameters *, SampleOffset, ysAudioSource **s) { *s=nullptr; return YDS_ERROR_RETURN(ysError::None); }
ysError ysAndroidAudioDevice::CreateSource(ysAudioBuffer *buf, ysAudioSource **s) { *s=new ysAndroidAudioSource(static_cast<ysAndroidAudioBuffer*>(buf)); return YDS_ERROR_RETURN(ysError::None); }

ysAndroidAudioBuffer::ysAndroidAudioBuffer() : ysAudioBuffer(ysAudioSystemObject::API::DirectSound8) {}
ysAndroidAudioBuffer::~ysAndroidAudioBuffer() {}
int ysAndroidAudioBuffer::SubmitToBackend(const int16_t *s, int c) { int w=0; esdroid::AndroidBackend::instance().writeAudioSamples(s,c,&w); return w; }

ysAndroidAudioSource::ysAndroidAudioSource(ysAndroidAudioBuffer *buf) : ysAudioSource(API::DirectSound8) {
    m_androidBuffer=buf; m_dataBuffer=static_cast<ysAudioBuffer*>(buf); m_bufferSize=buf->GetSampleCount();
    const ysAudioParameters *p=buf->GetAudioParameters(); if(p) m_audioParameters=*p;
}
ysAndroidAudioSource::~ysAndroidAudioSource() {}
ysError ysAndroidAudioSource::LockBufferSegment(SampleOffset, SampleOffset sc, void **s1, SampleOffset *sz1, void **s2, SampleOffset *sz2) {
    int16_t *scratch=new int16_t[sc]; if(s1)*s1=scratch; if(sz1)*sz1=sc; if(s2)*s2=nullptr; if(sz2)*sz2=0;
    return YDS_ERROR_RETURN(ysError::None);
}
ysError ysAndroidAudioSource::UnlockBufferSegments(void *s1, SampleOffset sz1, void *s2, SampleOffset sz2) {
    if(s1&&sz1>0){m_androidBuffer->SubmitToBackend((const int16_t*)s1,(int)sz1);delete[](int16_t*)s1;}
    if(s2&&sz2>0){m_androidBuffer->SubmitToBackend((const int16_t*)s2,(int)sz2);delete[](int16_t*)s2;}
    return YDS_ERROR_RETURN(ysError::None);
}
SampleOffset ysAndroidAudioSource::GetWP() const {
    // the read position of the ring how much audio opensl es consumed
    return (SampleOffset)esdroid::AndroidBackend::instance().getAudioReadPos();
}
ysError ysAndroidAudioSource::SetMode(Mode m) { return ysAudioSource::SetMode(m); }

class ysAndroidInputDevice : public ysInputDevice {
public:
    ysAndroidKeyboard *m_androidKb=nullptr;
    ysMouse *m_androidMouse=nullptr;
    ysAndroidInputDevice(Platform p, InputDeviceType t) : ysInputDevice(p,t) {
        if(t==InputDeviceType::KEYBOARD) {
            m_androidKb=new ysAndroidKeyboard();
            m_keyboard = m_androidKb;
        }
        else if(t==InputDeviceType::MOUSE) {
            m_androidMouse=new ysAndroidMouse();
            m_mouse = m_androidMouse;
        }
        SetConnected(true); SetGeneric(false);
    }
    virtual ~ysAndroidInputDevice() {
        // null the derived pointers the base destructor deletes them
        m_androidKb = nullptr;
        m_androidMouse = nullptr;
    }
};

ysAndroidInputSystem::ysAndroidInputSystem() : ysInputSystem(Platform::Android) {}
ysAndroidInputSystem::~ysAndroidInputSystem() {}
ysError ysAndroidInputSystem::CreateDevices() {
    ysInputDevice *kb=CreateDevice(ysInputDevice::InputDeviceType::KEYBOARD,0); RegisterDevice(kb); m_osKeyboard=kb;
    ysInputDevice *ms=CreateDevice(ysInputDevice::InputDeviceType::MOUSE,0); RegisterDevice(ms); m_osMouse=ms;
    return YDS_ERROR_RETURN(ysError::None);
}
ysInputDevice *ysAndroidInputSystem::CreateDevice(ysInputDevice::InputDeviceType t, int) { return new ysAndroidInputDevice(Platform::Android,t); }
ysInputDevice *ysAndroidInputSystem::CreateVirtualDevice(ysInputDevice::InputDeviceType t) { return new ysAndroidInputDevice(Platform::Android,t); }

ysAndroidKeyboard::ysAndroidKeyboard() : ysKeyboard() {}
ysAndroidKeyboard::~ysAndroidKeyboard() {}
// key state is fixed at press time so flipping fn mid hold cannot
// reassign a held button
bool ysAndroidKeyboard::IsKeyDown(ysKey::Code key) {
    auto& backend = esdroid::AndroidBackend::instance();
    for(int vk=1;vk<(int)esdroid::VirtualKey::Count;++vk) {
        int mapped=vkToYsKey((esdroid::VirtualKey)vk);
        if(mapped!=0&&mapped==(int)key
            &&backend.isKeyDown((esdroid::VirtualKey)vk)) return true;
    }
    return false;
}
bool ysAndroidKeyboard::ProcessKeyTransition(ysKey::Code key, ysKey::State state) {
    if(state!=ysKey::State::DownTransition) return false;
    auto& backend = esdroid::AndroidBackend::instance();
    for(int vk=1;vk<(int)esdroid::VirtualKey::Count;++vk) {
        int mapped=vkToYsKey((esdroid::VirtualKey)vk);
        if(mapped!=0&&mapped==(int)key
            &&backend.processKeyDown((esdroid::VirtualKey)vk)) return true;
    }
    return false;
}

ysAndroidMouse::ysAndroidMouse() : ysMouse() {}
ysAndroidMouse::~ysAndroidMouse() {}
int ysAndroidMouse::GetOsPositionX() const {
    return (int)esdroid::AndroidBackend::instance().engineTouchX();
}
int ysAndroidMouse::GetOsPositionY() const {
    return (int)esdroid::AndroidBackend::instance().engineTouchY();
}
int ysAndroidMouse::GetX() const { return GetOsPositionX(); }
int ysAndroidMouse::GetY() const { return GetOsPositionY(); }
// pull the backend edges into the state machine so each fires once
bool ysAndroidMouse::ProcessMouseButton(Button button, ButtonState state) {
    if(button==Button::Left) {
        auto& backend = esdroid::AndroidBackend::instance();
        if(backend.consumeEngineTouchDown()) UpdateButton(Button::Left,ButtonState::DownTransition);
        else if(backend.consumeEngineTouchUp()) UpdateButton(Button::Left,ButtonState::UpTransition);
    }
    return ysMouse::ProcessMouseButton(button,state);
}

// yswindowsaudiowavefile - portable wav reader
#pragma pack(push,1)
struct WaveHeader { char riff[4]; uint32_t riffSize; char wave[4]; char fmtId[4]; uint32_t fmtSize;
    uint16_t audioFormat,numChannels; uint32_t sampleRate,byteRate; uint16_t blockAlign,bitsPerSample;
    char dataId[4]; uint32_t dataSize; };
#pragma pack(pop)

ysWindowsAudioWaveFile::ysWindowsAudioWaveFile() : ysAudioFile(Platform::Android, AudioFormat::Wave) {}
ysWindowsAudioWaveFile::~ysWindowsAudioWaveFile() { DestroyInternalBuffer(); }

ysAudioFile::Error ysWindowsAudioWaveFile::OpenFile(const wchar_t *fname) {
    char path[512]; size_t i=0;
    for(;i<sizeof(path)-1&&fname[i];++i) path[i]=(char)(fname[i]&0xFF);
    path[i]=0;
    void* buf=nullptr; long sz=0;
    // try multiple path prefixes the impulse response filenames are
    // relative
    const char* prefixes[] = {
        "",  // as-is full path or already correct
        "sound-library/",  // assets/sound-library/
        "es/sound-library/",  // assets/es/sound-library/
    };
    bool ok=false;
    for (int p = 0; p < 3 && !ok; p++) {
        char fullPath[600];
        snprintf(fullPath, sizeof(fullPath), "%s%s", prefixes[p], path);
        ok = esdroid::AndroidBackend::instance().readFile(fullPath, &buf, &sz);
        if (!ok) ok = esdroid::AndroidBackend::instance().readAsset(fullPath, &buf, &sz);
    }
    if(sz<(long)sizeof(WaveHeader)){free(buf);return Error::InvalidFileFormat;}
    WaveHeader *hdr=(WaveHeader*)buf;
    if(memcmp(hdr->riff,"RIFF",4)!=0||memcmp(hdr->wave,"WAVE",4)!=0||memcmp(hdr->fmtId,"fmt ",4)!=0){free(buf);return Error::InvalidFileFormat;}
    ysAudioParameters params; params.m_sampleRate=hdr->sampleRate; params.m_bitsPerSample=hdr->bitsPerSample; params.m_channelCount=hdr->numChannels;
    m_audioParameters=params;
    long he=12+8+hdr->fmtSize; if(hdr->fmtSize&1) he+=1;
    char *p=(char*)buf+he, *end=(char*)buf+sz;
    void *dp=nullptr; long ds=0;
    while(p+8<=end) {
        if(memcmp(p,"data",4)==0){ds=*(uint32_t*)(p+4);dp=p+8;break;}
        uint32_t cs=*(uint32_t*)(p+4); p+=8+cs+(cs&1);
    }
    if(!dp){free(buf);return Error::InvalidFileFormat;}
    m_sampleCount=ds/(hdr->bitsPerSample/8*hdr->numChannels);
    m_fileOpen=true; m_externalBuffer=nullptr; m_maxBufferSamples=m_sampleCount; m_bufferDataSamples=m_sampleCount;
    m_buffer=(char*)dp; m_bufferOwner=buf; m_currentReadingOffset=0;
    return Error::None;
}
ysAudioFile::Error ysWindowsAudioWaveFile::CloseFile() { if(!m_fileOpen) return Error::NoFileOpen; m_fileOpen=false; return Error::None; }
ysAudioFile::Error ysWindowsAudioWaveFile::FillBuffer(SampleOffset) { if(!m_fileOpen) return Error::NoFileOpen; m_bufferDataSamples=m_sampleCount; return Error::None; }
void ysWindowsAudioWaveFile::InitializeInternalBuffer(SampleOffset s) { if(!m_buffer){m_buffer=new char[s*sizeof(int16_t)];m_maxBufferSamples=s;m_bufferDataSamples=0;m_externalBuffer=nullptr;} }
void ysWindowsAudioWaveFile::DestroyInternalBuffer() {
    if(m_bufferOwner){free(m_bufferOwner);m_bufferOwner=nullptr;}
    else if(m_buffer&&m_bufferOwner==nullptr) delete[] m_buffer;
    m_buffer=nullptr; m_bufferDataSamples=0; m_maxBufferSamples=0;
}

extern "C" ysWindowSystem *esdroid_create_window_system() { return new ysAndroidWindowSystem(); }
extern "C" ysAudioSystem *esdroid_create_audio_system() { return new ysAndroidAudioSystem(); }
extern "C" ysInputSystem *esdroid_create_input_system() { return new ysAndroidInputSystem(); }
