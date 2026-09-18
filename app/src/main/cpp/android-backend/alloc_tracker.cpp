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

// Allocation tracking is disabled. Overriding operator new without matching
// delete overrides corrupted the heap on Android arm64, so the STL handles
// all allocation with its own matched new/delete pair.
#if defined(__ANDROID__)
#include <android/log.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "ESDroid", __VA_ARGS__))

extern "C" void esdroid_alloc_log(const char *msg) {
    LOGI("%s", msg);
}
#endif
