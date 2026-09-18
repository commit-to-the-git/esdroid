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

#ifndef ESDROID_ANDROID_SHIM_H
#define ESDROID_ANDROID_SHIM_H
#include "yds_window_system.h"
#include "yds_window.h"
#include "yds_audio_system.h"
#include "yds_audio_device.h"
#include "yds_audio_buffer.h"
#include "yds_audio_source.h"
#include "yds_input_system.h"
#include "yds_keyboard.h"
#include "yds_mouse.h"
#include "yds_audio_file.h"

class ysAndroidWindowSystem : public ysWindowSystem {
public:
    ysAndroidWindowSystem();
    virtual ~ysAndroidWindowSystem();
    virtual ysError NewWindow(ysWindow **window) override;
    virtual ysError SurveyMonitors() override { return YDS_ERROR_RETURN(ysError::None); }
    virtual ysMonitor *MonitorFromWindow(ysWindow *window) override;
    virtual ysMonitor *NewMonitor() override;
    virtual void ProcessMessages() override;
    virtual void SetCursor(Cursor cursor) override { (void)cursor; }
    virtual void ConnectInstance(void *g) override { (void)g; }
    static ysAndroidWindowSystem *s_instance;
};

class ysAndroidWindow : public ysWindow {
    friend class ysAndroidWindowSystem;
public:
    ysAndroidWindow();
    virtual ~ysAndroidWindow();
    ysWindowSystemObject::Platform GetPlatform() const { return ysWindowSystemObject::Platform::Android; }
    virtual bool SetWindowStyle(WindowStyle s) override { m_windowStyle=s; return true; }
    virtual void SetState(WindowState s=WindowState::Visible) override { m_windowState=s; }
    virtual WindowState GetState() const override { return m_windowState; }
    virtual void SetTitle(const wchar_t *) override {}
    virtual void SetScreenSize(int w,int h) override;
    virtual void SetWindowSize(int w,int h) override;
    virtual void SetLocation(int,int) override {}
    virtual void Close() override { m_windowState=WindowState::Closed; }
    virtual bool IsActive() override { return true; }
    virtual bool IsVisible() override { return true; }
    virtual int GetScreenWidth() const override;
    virtual int GetScreenHeight() const override;
    virtual void ScreenToLocal(int &x, int &y) const override;
    const int GetGameWidth() const;
    const int GetGameHeight() const;
    bool IsOpen() const;
private:
    bool m_open=true;
};

class ysAndroidAudioSystem : public ysAudioSystem {
public:
    ysAndroidAudioSystem();
    virtual ~ysAndroidAudioSystem();
    virtual ysError CreateBuffer(const ysAudioParameters *p, SampleOffset s, ysAudioBuffer **b) override;
    virtual ysError EnumerateDevices() override { return YDS_ERROR_RETURN(ysError::None); }
    virtual ysError ConnectDevice(ysAudioDevice *d, ysWindow *w) override { (void)d;(void)w; return YDS_ERROR_RETURN(ysError::None); }
    virtual ysError ConnectDevice(ysWindow *w, ysAudioDevice **d) override;
    virtual ysError ConnectDeviceConsole(ysAudioDevice *d) override { (void)d; return YDS_ERROR_RETURN(ysError::None); }
    virtual ysError DisconnectDevice(ysAudioDevice *d) override { (void)d; return YDS_ERROR_RETURN(ysError::None); }
    static ysAndroidAudioSystem *s_instance;
};

class ysAndroidAudioDevice : public ysAudioDevice {
public:
    ysAndroidAudioDevice();
    virtual ~ysAndroidAudioDevice();
    virtual ysError CreateSource(const ysAudioParameters *p, SampleOffset s, ysAudioSource **src) override;
    virtual ysError CreateSource(ysAudioBuffer *buf, ysAudioSource **src) override;
    virtual void UpdateAudioSources() override {}
};

class ysAndroidAudioBuffer : public ysAudioBuffer {
public:
    ysAndroidAudioBuffer();
    virtual ~ysAndroidAudioBuffer();
    int SubmitToBackend(const int16_t *samples, int count);
};

class ysAndroidAudioSource : public ysAudioSource {
public:
    ysAndroidAudioSource(ysAndroidAudioBuffer *buffer);
    virtual ~ysAndroidAudioSource();
    virtual ysError LockBufferSegment(SampleOffset off, SampleOffset s, void **s1, SampleOffset *sz1, void **s2, SampleOffset *sz2) override;
    virtual ysError UnlockBufferSegments(void *s1, SampleOffset sz1, void *s2, SampleOffset sz2) override;
    virtual bool GetCurrentWritePosition(SampleOffset *p) override { *p=GetWP(); return true; }
    virtual bool GetCurrentPosition(SampleOffset *p) override { *p=GetWP(); return true; }
    virtual ysError SetMode(Mode m) override;
    virtual ysError SetVolume(float v) override { m_volume=v; return YDS_ERROR_RETURN(ysError::None); }
    virtual ysError SetPan(float p) override { m_pan=p; return YDS_ERROR_RETURN(ysError::None); }
    SampleOffset GetWP() const;
    ysAndroidAudioBuffer *m_androidBuffer=nullptr;
};

class ysAndroidInputSystem : public ysInputSystem {
public:
    ysAndroidInputSystem();
    virtual ~ysAndroidInputSystem();
protected:
    virtual ysError CreateDevices() override;
    virtual ysInputDevice *CreateDevice(ysInputDevice::InputDeviceType type, int id) override;
    virtual ysInputDevice *CreateVirtualDevice(ysInputDevice::InputDeviceType type) override;
    virtual ysError CheckDeviceStatus(ysInputDevice *d) override { (void)d; return YDS_ERROR_RETURN(ysError::None); }
    virtual ysError CheckAllDevices() override { return YDS_ERROR_RETURN(ysError::None); }
};

class ysAndroidKeyboard : public ysKeyboard {
public:
    ysAndroidKeyboard();
    virtual ~ysAndroidKeyboard();
    virtual bool IsKeyDown(ysKey::Code key) override;
    virtual bool ProcessKeyTransition(ysKey::Code key, ysKey::State state=ysKey::State::DownTransition) override;
};

class ysAndroidMouse : public ysMouse {
public:
    ysAndroidMouse();
    virtual ~ysAndroidMouse();
    // Raw touch position, y-down, exactly what the window's ScreenToLocal
    // expects as input (same as the raw cursor pos the desktop stores).
    virtual int GetOsPositionX() const override;
    virtual int GetOsPositionY() const override;
    virtual int GetX() const override;
    virtual int GetY() const override;
    virtual bool ProcessMouseButton(Button button, ButtonState state) override;
};

class ysWindowsAudioWaveFile : public ysAudioFile {
public:
    ysWindowsAudioWaveFile();
    virtual ~ysWindowsAudioWaveFile();
    virtual ysAudioFile::Error OpenFile(const wchar_t *fname) override;
    virtual ysAudioFile::Error CloseFile() override;
    ysAudioFile::Error FillBuffer(SampleOffset offset);
    void InitializeInternalBuffer(SampleOffset samples);
    void FillBuffer(int) { FillBuffer((SampleOffset)0); }
    void DestroyInternalBuffer();
    SampleOffset GetSampleCount() const { return m_sampleCount; }
    void *GetBuffer() const { return (void*)m_buffer; }
private:
    void *m_bufferOwner=nullptr;
};

#endif // ESDROID_ANDROID_SHIM_H
