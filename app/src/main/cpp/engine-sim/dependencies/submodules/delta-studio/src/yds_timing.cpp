#include "../include/yds_timing.h"
#include "../include/yds_math.h"
#if defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__)
  #include <time.h>
  #include <sys/time.h>
  #include <unistd.h>
#else
  #define NOMINMAX
  #include <Windows.h>
  #include <mmsystem.h>
  #include <intrin.h>
#endif
static bool qpcFlag;
#if defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__)
static uint64_t qpcFrequency=1000000000ULL;
#else
static LARGE_INTEGER qpcFrequency;
#endif
ysTimingSystem *ysTimingSystem::g_instance = nullptr;
uint64_t SystemTime() {
#if defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__)
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts);
    return (uint64_t)ts.tv_sec*1000000000ULL+(uint64_t)ts.tv_nsec;
#else
    if (qpcFlag) { LARGE_INTEGER t; QueryPerformanceCounter(&t); return uint64_t(t.QuadPart); }
    else return uint64_t(timeGetTime()) * 1000;
#endif
}
ysTimingSystem::ysTimingSystem() { SetPrecisionMode(Precision::Microsecond); Initialize(); m_averageTimer=0.0; }
ysTimingSystem::~ysTimingSystem() {}
uint64_t ysTimingSystem::GetTime() { return SystemTime(); }
#if defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__)
static inline unsigned __int64 SystemClock() {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts);
    return (unsigned __int64)((uint64_t)ts.tv_sec*1000000000ULL+(uint64_t)ts.tv_nsec);
}
#else
inline unsigned __int64 SystemClock() { return __rdtsc(); }
#endif
unsigned __int64 ysTimingSystem::GetClock() { return (unsigned long long)SystemClock(); }
void ysTimingSystem::SetPrecisionMode(Precision mode) {
    m_precisionMode=mode;
    if(mode==Precision::Millisecond) m_div=1000.0;
    else if(mode==Precision::Microsecond) m_div=1000000.0;
}
double ysTimingSystem::ConvertToSeconds(uint64_t t_u) {
#if defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__)
    return (double)t_u/1e9;
#else
    return t_u/m_div;
#endif
}
void ysTimingSystem::Update() {
    if(!m_isPaused) m_frameNumber++;
    const uint64_t thisTime=GetTime();
#if defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__)
    m_lastFrameDuration=((thisTime-m_lastFrameTimestamp)/1000ULL);
#else
    m_lastFrameDuration=((thisTime-m_lastFrameTimestamp)*1000000)/qpcFrequency.QuadPart;
#endif
    m_lastFrameTimestamp=thisTime;
    const uint64_t thisClock=GetClock();
    m_lastFrameClockTicks=(thisClock-m_lastFrameClockstamp);
    m_lastFrameClockstamp=thisClock;
    const double frameDuration=GetFrameDuration();
    if(m_frameNumber>1) {
        constexpr double dt=1/10000.0;
        const double f_c=(m_frameNumber>1000)?0.1:100.0;
        const double RC=1.0/(ysMath::Constants::TWO_PI*f_c);
        const double alpha=dt/(RC+dt);
        m_averageTimer+=frameDuration;
        if(m_averageTimer>1.0) m_averageTimer=1.0;
        while(m_averageTimer>0) { m_averageTimer-=dt; m_averageFrameDuration=alpha*frameDuration+(1-alpha)*m_averageFrameDuration; }
    } else { m_averageFrameDuration=frameDuration; }
    m_fps=float(1/m_averageFrameDuration);
}
void ysTimingSystem::RestartFrame() { m_lastFrameTimestamp=GetTime(); }
void ysTimingSystem::Initialize() {
#if defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__)
    qpcFrequency=1000000000ULL; qpcFlag=true;
#else
    qpcFlag=(QueryPerformanceFrequency(&qpcFrequency)>0);
#endif
    m_frameNumber=0; m_lastFrameTimestamp=GetTime(); m_lastFrameDuration=0;
    m_lastFrameClockstamp=GetClock(); m_lastFrameClockTicks=0; m_isPaused=false; m_fps=1024.0;
}
double ysTimingSystem::GetFrameDuration() {
#if defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__)
    return (double)m_lastFrameDuration/1e6;
#else
    return m_lastFrameDuration/m_div;
#endif
}
uint64_t ysTimingSystem::GetFrameDuration_us() { return m_lastFrameDuration; }
