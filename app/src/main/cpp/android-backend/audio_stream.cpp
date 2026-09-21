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

#include "audio_stream.h"

#include <oboe/Oboe.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

namespace esdroid {

// frames handled per pass so the scratch buffer stays a fixed size
static constexpr int kSlabFrames = 1024;
// content samples the scratch can hold worst case is a 8khz device
static constexpr int kScratchSamples = 8192;
// gain ramps so dry edges fade instead of clicking
// the drop is fast so a short gap lands near silence before data returns
static constexpr float kFadeUp = 1.0f / 128.0f;
static constexpr float kFadeDown = 1.0f / 64.0f;
// ticks without a single callback before the stream is treated as dead
static constexpr int kStallTicks = 90;

// the fifo fill is smoothed then compared against watermarks built from
// a 120ms reference the same floor the backend regulator uses
static constexpr double kFillReferenceSeconds = 0.12;
static constexpr float kFillLowMark = 0.35f;
static constexpr float kFillHighMark = 0.75f;
// how fast the smoothed fill chases the real fill per callback
static constexpr float kFillTrack = 1.0f / 16.0f;
// playback rate bounds and step sizes per callback
// when the producer runs short the consumer eases slower so the fifo
// coasts instead of running dry and clicking
static constexpr float kRateMin = 0.85f;
static constexpr float kRateDrop = 0.004f;
static constexpr float kRateRise = 0.0015f;

struct AudioStream::Impl final : public oboe::AudioStreamCallback {
    AudioStreamHost host;
    int contentRate = 44100;
    int channels = 1;

    std::atomic<bool> live{false};
    std::atomic<bool> restartWanted{false};
    std::atomic<bool> flushWanted{false};
    std::atomic<unsigned> underruns{0};
    // bumped by every callback so tick can tell a dead stream
    std::atomic<unsigned> cbCount{0};

    std::shared_ptr<oboe::AudioStream> stream;
    int deviceRate = 44100;
    // frames between reopen attempts after a failed restart
    int retryTicks = 0;
    // callback progress as seen by the last tick
    unsigned lastCbCount = 0;
    int stallTicks = 0;

    // resampler position in content samples
    // the integer part is consumed from the fifo the fraction carries over
    double carry = 0.0;
    double step = 1.0;
    // base ratio before the rate control touches it
    double baseStep = 1.0;
    // playback rate control so a starving producer stretches the audio
    // instead of clicking dry
    float rateScale = 1.0f;
    float fillAvg = 0.0f;
    int fillLowMark = 1;
    int fillHighMark = 1;
    int16_t heldSample = 0;
    float fade = 0.0f;
    // true once the fifo has delivered samples
    // dry runs before that are pre roll not underruns
    bool primed = false;
    std::vector<int16_t> scratch;

    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream* s, void* audioData, int32_t numFrames) override;
    void onErrorAfterClose(oboe::AudioStream* s, oboe::Result error) override;

    bool openStream(bool exclusive);
    void closeStream();
};

bool AudioStream::Impl::openStream(bool exclusive) {
    oboe::AudioStreamBuilder builder;
    builder.setCallback(this)
        ->setDirection(oboe::Direction::Output)
        ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
        ->setUsage(oboe::Usage::Game)
        ->setContentType(oboe::ContentType::Music)
        ->setFormat(oboe::AudioFormat::I16)
        ->setChannelCount(channels)
        ->setSampleRate(contentRate)
        ->setSharingMode(
            exclusive ? oboe::SharingMode::Exclusive : oboe::SharingMode::Shared);
    // the stream is asked for the content rate so the framework resamples
    // on routes that run at another rate with a real filter
    // a self made linear resample of bright engine audio down to a low
    // rate route aliases badly so that job goes back to the framework

    oboe::Result r = builder.openStream(stream);
    if (r != oboe::Result::OK) {
        stream.reset();
        return false;
    }

    // three bursts of device slack so a late callback cannot glitch
    // the device clamps the ask when the capacity is smaller
    stream->setBufferSizeInFrames(3 * stream->getFramesPerBurst());

    r = stream->requestStart();
    if (r != oboe::Result::OK) {
        closeStream();
        return false;
    }

    deviceRate = stream->getSampleRate();
    if (deviceRate <= 0) deviceRate = contentRate;
    baseStep = (double)contentRate / (double)deviceRate;
    step = baseStep * (double)rateScale;
    return true;
}

void AudioStream::Impl::closeStream() {
    if (stream) {
        stream->requestStop();
        stream->close();
        stream.reset();
    }
}

void AudioStream::Impl::onErrorAfterClose(oboe::AudioStream* s, oboe::Result error) {
    (void)s;
    (void)error;
    // the main thread rebuilds it from tick
    restartWanted.store(true, std::memory_order_relaxed);
}

oboe::DataCallbackResult AudioStream::Impl::onAudioReady(
        oboe::AudioStream* s, void* audioData, int32_t numFrames) {
    (void)s;
    // count the callback so tick can see the stream is alive
    cbCount.fetch_add(1, std::memory_order_relaxed);

    if (!live.load(std::memory_order_relaxed)) {
        return oboe::DataCallbackResult::Stop;
    }

    if (flushWanted.exchange(false, std::memory_order_relaxed)) {
        // start over from the fifo head and fade in from silence
        carry = 0.0;
        heldSample = 0;
        fade = 0.0f;
        primed = false;
        rateScale = 1.0f;
        fillAvg = 0.0f;
    }

    // ride the playback rate against the fifo fill
    // a starving producer stretches the audio a little instead of
    // running the fifo dry and clicking
    if (primed && host.fill) {
        const float avail = (float)host.fill(host.ctx);
        fillAvg += (avail - fillAvg) * kFillTrack;
        if (fillAvg < (float)fillLowMark) {
            rateScale -= kRateDrop;
            if (rateScale < kRateMin) rateScale = kRateMin;
        }
        else if (fillAvg > (float)fillHighMark) {
            rateScale += kRateRise;
            if (rateScale > 1.0f) rateScale = 1.0f;
        }
        step = baseStep * (double)rateScale;
    }

    int16_t* out = static_cast<int16_t*>(audioData);
    int framesLeft = numFrames;

    while (framesLeft > 0) {
        int n = framesLeft < kSlabFrames ? framesLeft : kSlabFrames;
        // shrink the pass when the scratch cannot hold the peek window
        while (n > 1
                && (int)(carry + (double)(n - 1) * step) + 2 > kScratchSamples) {
            n /= 2;
        }

        const double carry0 = carry;
        const int lastIdx = (int)(carry0 + (double)(n - 1) * step);
        const int need = lastIdx + 2;
        const int got = host.peek
            ? host.peek(host.ctx, scratch.data(), need)
            : 0;

        for (int i = 0; i < n; ++i) {
            const double pos = carry0 + (double)i * step;
            const int idx = (int)pos;
            const float frac = (float)(pos - (double)idx);

            const int16_t a = idx < got ? scratch[idx] : heldSample;
            const int16_t b = (idx + 1) < got ? scratch[idx + 1] : a;

            const float v = (float)a + ((float)b - (float)a) * frac;

            // ramp the gain toward silence while dry and back up when wet
            const bool dry = (idx + 1) >= got;
            fade += dry ? -kFadeDown : kFadeUp;
            if (fade < 0.0f) fade = 0.0f;
            else if (fade > 1.0f) fade = 1.0f;

            *out++ = (int16_t)std::lrintf(v * fade);
        }

        const double end = carry0 + (double)n * step;
        const int advance = (int)end;
        if (got >= advance && got > 0) {
            // the fifo held the base samples so the position stays continuous
            if (advance > 0 && host.consume) host.consume(host.ctx, advance);
            carry = end - (double)advance;
            if (got > 0) heldSample = scratch[got - 1];
        }
        else {
            // hard underrun drop what is left and restart from the new head
            // the fade ramp keeps running so the gap edge stays quiet
            if (got > 0 && host.consume) host.consume(host.ctx, got);
            carry = 0.0;
        }

        // only count dry runs once audio has actually flowed
        if (primed && got < need) {
            underruns.fetch_add(1, std::memory_order_relaxed);
        }
        if (got > 0) primed = true;

        framesLeft -= n;
    }

    return oboe::DataCallbackResult::Continue;
}

AudioStream::AudioStream() : m_impl(new Impl) {}

AudioStream::~AudioStream() {
    stop();
    delete m_impl;
}

bool AudioStream::start(
        const AudioStreamHost& host, int contentRate, int channels) {
    if (m_impl->live.load(std::memory_order_relaxed)) return true;
    if (!host.peek || !host.consume || contentRate <= 0) return false;
    // the resampler only handles mono which is all the synth outputs
    if (channels != 1) {
        return false;
    }

    m_impl->host = host;
    m_impl->contentRate = contentRate;
    m_impl->channels = channels;
    m_impl->carry = 0.0;
    m_impl->step = 1.0;
    m_impl->baseStep = 1.0;
    m_impl->rateScale = 1.0f;
    m_impl->fillAvg = 0.0f;
    m_impl->fillLowMark = (int)(kFillReferenceSeconds * contentRate * kFillLowMark);
    m_impl->fillHighMark = (int)(kFillReferenceSeconds * contentRate * kFillHighMark);
    if (m_impl->fillLowMark < 1) m_impl->fillLowMark = 1;
    if (m_impl->fillHighMark < m_impl->fillLowMark + 1)
        m_impl->fillHighMark = m_impl->fillLowMark + 1;
    m_impl->heldSample = 0;
    m_impl->fade = 0.0f;
    m_impl->primed = false;
    m_impl->scratch.assign(kScratchSamples, 0);
    m_impl->underruns.store(0, std::memory_order_relaxed);
    m_impl->restartWanted.store(false, std::memory_order_relaxed);
    m_impl->flushWanted.store(false, std::memory_order_relaxed);
    m_impl->cbCount.store(0, std::memory_order_relaxed);
    m_impl->lastCbCount = 0;
    m_impl->stallTicks = 0;
    m_impl->retryTicks = 0;

    // live goes up before the stream starts
    // a callback can fire the moment the stream starts and an early
    // stop return would kill the stream for good with no error callback
    m_impl->live.store(true, std::memory_order_relaxed);

    // exclusive gives the shortest path fall back to shared when taken
    if (!m_impl->openStream(true)) {
        if (!m_impl->openStream(false)) {
            m_impl->live.store(false, std::memory_order_relaxed);
            return false;
        }
    }
    return true;
}

void AudioStream::stop() {
    m_impl->live.store(false, std::memory_order_relaxed);
    m_impl->closeStream();
}

void AudioStream::tick() {
    if (!m_impl->live.load(std::memory_order_relaxed)) {
        // a restart failed so retry every few seconds
        if (++m_impl->retryTicks < 240) return;
        m_impl->retryTicks = 0;
        if (m_impl->openStream(true) || m_impl->openStream(false)) {
            m_impl->live.store(true, std::memory_order_relaxed);
            m_impl->lastCbCount =
                m_impl->cbCount.load(std::memory_order_relaxed);
            m_impl->stallTicks = 0;
        }
        return;
    }
    m_impl->retryTicks = 0;

    // a stream can stop without an error callback so watch for silence
    const unsigned cb = m_impl->cbCount.load(std::memory_order_relaxed);
    bool dead = false;
    if (cb != m_impl->lastCbCount) {
        m_impl->lastCbCount = cb;
        m_impl->stallTicks = 0;
    }
    else if (m_impl->stream && ++m_impl->stallTicks >= kStallTicks) {
        m_impl->stallTicks = 0;
        dead = true;
    }

    if (!dead
            && !m_impl->restartWanted.exchange(false, std::memory_order_relaxed)) {
        return;
    }

    // rebuild the stream after a stall or a disconnect
    m_impl->closeStream();
    if (m_impl->openStream(true) || m_impl->openStream(false)) {
        // drop a restart flag raised by the old stream while it closed
        m_impl->restartWanted.store(false, std::memory_order_relaxed);
        m_impl->lastCbCount =
            m_impl->cbCount.load(std::memory_order_relaxed);
        m_impl->stallTicks = 0;
    }
    else {
        m_impl->live.store(false, std::memory_order_relaxed);
    }
}

void AudioStream::flush() {
    m_impl->flushWanted.store(true, std::memory_order_relaxed);
}

bool AudioStream::active() const {
    return m_impl->live.load(std::memory_order_relaxed);
}

int AudioStream::deviceRate() const {
    return m_impl->deviceRate;
}

unsigned AudioStream::takeUnderrunCount() {
    return m_impl->underruns.exchange(0, std::memory_order_relaxed);
}

} // namespace esdroid
