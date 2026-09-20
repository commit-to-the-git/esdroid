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

#ifndef ESDROID_MSVC_COMPAT_H
#define ESDROID_MSVC_COMPAT_H
#if defined(__cplusplus)
#if !defined(_MSC_VER)
#if defined(__cplusplus)
#include <cstring>
#include <cwchar>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sys/stat.h>
#endif //  __cplusplus

#ifndef __stdcall
#define __stdcall
#endif
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __fastcall
#define __fastcall
#endif
#ifndef WINAPI
#define WINAPI
#endif
#ifndef CALLBACK
#define CALLBACK
#endif
#ifndef __forceinline
#define __forceinline inline __attribute__((always_inline))
#endif
#ifndef __int8
#define __int8 signed char
#endif
#ifndef __int16
#define __int16 short
#endif
#ifndef __int32
#define __int32 int
#endif
#ifndef __int64
#define __int64 long long
#endif
#ifndef __uint8
#define __uint8 unsigned char
#endif
#ifndef __uint16
#define __uint16 unsigned short
#endif
#ifndef __uint32
#define __uint32 unsigned int
#endif
#ifndef __uint64
#define __uint64 unsigned long long
#endif

static inline int strcpy_s(char *d, size_t n, const char *s) {
    if(!d||!s||!n) return 22; size_t l=strlen(s); if(l>=n) return 34;
    memcpy(d,s,l+1); return 0;
}
static inline int wcscpy_s(wchar_t *d, size_t n, const wchar_t *s) {
    if(!d||!s||!n) return 22; size_t l=wcslen(s); if(l>=n) return 34;
    wmemcpy(d,s,l+1); return 0;
}
static inline int wcscpy_s(wchar_t *d, size_t n, const wchar_t *s, size_t c) {
    if(!d||!s||!n) return 22; if(c>=n) return 34;
    wcsncpy(d,s,c); d[c]=0; return 0;
}
static inline int wcscat_s(wchar_t *d, size_t n, const wchar_t *s) {
    if(!d||!s||!n) return 22; size_t dl=wcslen(d),sl=wcslen(s);
    if(dl+sl>=n) return 34; wmemcpy(d+dl,s,sl+1); return 0;
}
static inline int strcat_s(char *d, size_t n, const char *s) {
    if(!d||!s||!n) return 22; size_t dl=strlen(d),sl=strlen(s);
    if(dl+sl>=n) return 34; memcpy(d+dl,s,sl+1); return 0;
}
static inline int swprintf_s(wchar_t *buf, size_t n, const wchar_t *fmt, ...) {
    va_list a; va_start(a,fmt); int r=vswprintf(buf,n,fmt,a); va_end(a); return r;
}
static inline int sprintf_s(char *buf, size_t n, const char *fmt, ...) {
    va_list a; va_start(a,fmt); int r=vsnprintf(buf,n,fmt,a); va_end(a); return r;
}
static inline int _snprintf_s(char *b, size_t n, size_t, const char *fmt, ...) {
    va_list a; va_start(a,fmt); int r=vsnprintf(b,n,fmt,a); va_end(a); return r;
}
static inline int _swprintf_s(wchar_t *b, size_t n, const wchar_t *fmt, ...) {
    va_list a; va_start(a,fmt); int r=vswprintf(b,n,fmt,a); va_end(a); return r;
}
static inline int _wtoi(const wchar_t *s) { return (int)wcstol(s,nullptr,10); }
static inline double _wtof(const wchar_t *s) { return wcstod(s,nullptr); }
static inline FILE *_wfopen(const wchar_t *p, const wchar_t *m) {
    char np[512],nm[16]; size_t i;
    for(i=0;i<sizeof(np)-1&&p[i];++i) np[i]=(char)p[i]; np[i]=0;
    for(i=0;i<sizeof(nm)-1&&m[i];++i) nm[i]=(char)m[i]; nm[i]=0;
    return fopen(np,nm);
}
static inline int _wfopen_s(FILE **fp, const wchar_t *p, const wchar_t *m) {
    if(!fp) return 22; char np[512],nm[16]; size_t i;
    for(i=0;i<sizeof(np)-1&&p[i];++i) np[i]=(char)p[i]; np[i]=0;
    for(i=0;i<sizeof(nm)-1&&m[i];++i) nm[i]=(char)m[i]; nm[i]=0;
    *fp=fopen(np,nm); return (*fp==nullptr)?2:0;
}
static inline int _wcsicmp(const wchar_t *a, const wchar_t *b) {
    while(*a&&*b) {
        int da=(*a>=L'A'&&*a<=L'Z')?*a+32:*a;
        int db=(*b>=L'A'&&*b<=L'Z')?*b+32:*b;
        if(da!=db) return da-db; ++a; ++b;
    }
    return (int)*a-(int)*b;
}
static inline struct tm *localtime_s(struct tm *r, const time_t *t) {
    return localtime_r(t,r)?r:nullptr;
}
#define _stat stat
static inline int _wstat(const wchar_t *p, struct stat *b) {
    char np[512]; size_t i;
    for(i=0;i<sizeof(np)-1&&p[i];++i) np[i]=(char)p[i]; np[i]=0;
    return stat(np,b);
}
#endif //
#endif //  __cplusplus
#endif //  ESDROID_MSVC_COMPAT_H
