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

#ifndef ESDROID_AUDIO_STREAM_H
#define ESDROID_AUDIO_STREAM_H

#include <cstdint>

namespace esdroid {

// the app side of the audio fifo the stream pulls from
struct AudioStreamHost {
    void* ctx = nullptr;
    // copy up to count samples into dst without consuming them
    // returns how many samples actually exist
    int (*peek)(void* ctx, int16_t* dst, int count) = nullptr;
    // drop count samples from the front of the fifo
    void (*consume)(void* ctx, int count) = nullptr;
    // samples sitting in the fifo right now or negative when unknown
    int (*fill)(void* ctx) = nullptr;
};

// owns the oboe output stream and feeds it from the app fifo
// aaudio is used on android 8 and up with opensl es below that
class AudioStream {
public:
    AudioStream();
    ~AudioStream();
    AudioStream(const AudioStream&) = delete;
    AudioStream& operator=(const AudioStream&) = delete;

    // opens and starts the stream at the content sample rate
    // the framework resamples when the route runs at another rate
    bool start(const AudioStreamHost& host, int contentRate, int channels);
    void stop();
    // call every frame on the main thread
    // rebuilds the stream after a disconnect or route change
    void tick();
    // drop the resampler state so playback fades in from silence
    void flush();
    bool active() const;
    int deviceRate() const;
    // dry callbacks since the last call
    unsigned takeUnderrunCount();

private:
    struct Impl;
    Impl* m_impl;
};

} // namespace esdroid

#endif // ESDROID_AUDIO_STREAM_H
