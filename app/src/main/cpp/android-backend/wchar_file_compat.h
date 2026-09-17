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

#ifndef ESDROID_WCHAR_FILE_COMPAT_H
#define ESDROID_WCHAR_FILE_COMPAT_H
#if defined(__cplusplus)
#if defined(__ANDROID__) || defined(__linux__) || defined(__APPLE__)
#include <string>
#include <cwchar>
inline std::string esdroid_wchar_to_narrow(const wchar_t *wpath) {
    if (!wpath) return std::string();
    std::string r; size_t len = wcslen(wpath); r.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        wchar_t c = wpath[i];
        if (c < 0x80) r.push_back((char)c);
        else if (c < 0x800) { r.push_back((char)(0xC0|(c>>6))); r.push_back((char)(0x80|(c&0x3F))); }
        else { r.push_back((char)(0xE0|(c>>12))); r.push_back((char)(0x80|((c>>6)&0x3F))); r.push_back((char)(0x80|(c&0x3F))); }
    }
    return r;
}
inline std::string esdroid_wchar_to_narrow(const std::wstring &w) { return esdroid_wchar_to_narrow(w.c_str()); }
#define ESDROID_NARROW(wpath) esdroid_wchar_to_narrow(wpath).c_str()
#else
#define ESDROID_NARROW(wpath) wpath
#endif
#endif
#endif
