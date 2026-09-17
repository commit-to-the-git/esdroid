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

// sse_scalar_fallback.h - Scalar SSE intrinsics for non-x86 platforms.
#ifndef SSE_SCALAR_FALLBACK_H
#define SSE_SCALAR_FALLBACK_H
#if defined(__x86_64__)||defined(_M_X64)||defined(__i386__)||defined(_M_IX86)
#include <xmmintrin.h>
#include <emmintrin.h>
#else
#include <cmath>
typedef struct __m128 { float m128_f32[4]; } __m128;
#define _MM_SHUFFLE(z,y,x,w) (((z)<<6)|((y)<<4)|((x)<<2)|(w))
static inline __m128 _mm_set_ps(float w,float z,float y,float x){__m128 r;r.m128_f32[0]=x;r.m128_f32[1]=y;r.m128_f32[2]=z;r.m128_f32[3]=w;return r;}
static inline __m128 _mm_set1_ps(float a){__m128 r;r.m128_f32[0]=r.m128_f32[1]=r.m128_f32[2]=r.m128_f32[3]=a;return r;}
static inline __m128 _mm_setzero_ps(){return _mm_set1_ps(0.0f);}
static inline __m128 _mm_add_ps(__m128 a,__m128 b){__m128 r;for(int i=0;i<4;++i)r.m128_f32[i]=a.m128_f32[i]+b.m128_f32[i];return r;}
static inline __m128 _mm_sub_ps(__m128 a,__m128 b){__m128 r;for(int i=0;i<4;++i)r.m128_f32[i]=a.m128_f32[i]-b.m128_f32[i];return r;}
static inline __m128 _mm_mul_ps(__m128 a,__m128 b){__m128 r;for(int i=0;i<4;++i)r.m128_f32[i]=a.m128_f32[i]*b.m128_f32[i];return r;}
static inline __m128 _mm_div_ps(__m128 a,__m128 b){__m128 r;for(int i=0;i<4;++i)r.m128_f32[i]=a.m128_f32[i]/b.m128_f32[i];return r;}
static inline __m128 _mm_min_ps(__m128 a,__m128 b){__m128 r;for(int i=0;i<4;++i)r.m128_f32[i]=(a.m128_f32[i]<b.m128_f32[i])?a.m128_f32[i]:b.m128_f32[i];return r;}
static inline __m128 _mm_max_ps(__m128 a,__m128 b){__m128 r;for(int i=0;i<4;++i)r.m128_f32[i]=(a.m128_f32[i]>b.m128_f32[i])?a.m128_f32[i]:b.m128_f32[i];return r;}
static inline __m128 _mm_sqrt_ps(__m128 a){__m128 r;for(int i=0;i<4;++i)r.m128_f32[i]=sqrtf(a.m128_f32[i]);return r;}
static inline __m128 _mm_and_ps(__m128 a,__m128 b){__m128 r;int*ia=(int*)a.m128_f32,*ib=(int*)b.m128_f32,*ir=(int*)r.m128_f32;for(int i=0;i<4;++i)ir[i]=ia[i]&ib[i];return r;}
static inline __m128 _mm_or_ps(__m128 a,__m128 b){__m128 r;int*ia=(int*)a.m128_f32,*ib=(int*)b.m128_f32,*ir=(int*)r.m128_f32;for(int i=0;i<4;++i)ir[i]=ia[i]|ib[i];return r;}
static inline __m128 _mm_andnot_ps(__m128 a,__m128 b){__m128 r;int*ia=(int*)a.m128_f32,*ib=(int*)b.m128_f32,*ir=(int*)r.m128_f32;for(int i=0;i<4;++i)ir[i]=(~ia[i])&ib[i];return r;}
static inline __m128 _mm_cmpeq_ps(__m128 a,__m128 b){__m128 r;for(int i=0;i<4;++i){int m=(a.m128_f32[i]==b.m128_f32[i])?-1:0;r.m128_f32[i]=*(float*)&m;}return r;}
static inline __m128 _mm_cmplt_ps(__m128 a,__m128 b){__m128 r;for(int i=0;i<4;++i){int m=(a.m128_f32[i]<b.m128_f32[i])?-1:0;r.m128_f32[i]=*(float*)&m;}return r;}
static inline __m128 _mm_cmple_ps(__m128 a,__m128 b){__m128 r;for(int i=0;i<4;++i){int m=(a.m128_f32[i]<=b.m128_f32[i])?-1:0;r.m128_f32[i]=*(float*)&m;}return r;}
static inline __m128 _mm_cmpgt_ps(__m128 a,__m128 b){__m128 r;for(int i=0;i<4;++i){int m=(a.m128_f32[i]>b.m128_f32[i])?-1:0;r.m128_f32[i]=*(float*)&m;}return r;}
static inline __m128 _mm_cmpge_ps(__m128 a,__m128 b){__m128 r;for(int i=0;i<4;++i){int m=(a.m128_f32[i]>=b.m128_f32[i])?-1:0;r.m128_f32[i]=*(float*)&m;}return r;}
static inline __m128 _mm_shuffle_ps(__m128 a,__m128 b,int imm){__m128 r;r.m128_f32[0]=a.m128_f32[(imm>>0)&3];r.m128_f32[1]=a.m128_f32[(imm>>2)&3];r.m128_f32[2]=b.m128_f32[(imm>>4)&3];r.m128_f32[3]=b.m128_f32[(imm>>6)&3];return r;}
#ifndef _mm_replicate_x_ps
#define _mm_replicate_x_ps(v) _mm_shuffle_ps((v),(v),_MM_SHUFFLE(0,0,0,0))
#endif
#ifndef _mm_replicate_y_ps
#define _mm_replicate_y_ps(v) _mm_shuffle_ps((v),(v),_MM_SHUFFLE(1,1,1,1))
#endif
#ifndef _mm_replicate_z_ps
#define _mm_replicate_z_ps(v) _mm_shuffle_ps((v),(v),_MM_SHUFFLE(2,2,2,2))
#endif
#ifndef _mm_replicate_w_ps
#define _mm_replicate_w_ps(v) _mm_shuffle_ps((v),(v),_MM_SHUFFLE(3,3,3,3))
#endif
#ifndef _mm_madd_ps
#define _mm_madd_ps(a,b,c) _mm_add_ps(_mm_mul_ps((a),(b)),(c))
#endif
#ifndef _MM_TRANSPOSE4_PS
#define _MM_TRANSPOSE4_PS(r0,r1,r2,r3) do{ \
    __m128 _t3=_mm_shuffle_ps((r0),(r1),_MM_SHUFFLE(1,0,1,0)); \
    __m128 _t2=_mm_shuffle_ps((r0),(r1),_MM_SHUFFLE(3,2,3,2)); \
    __m128 _t7=_mm_shuffle_ps((r2),(r3),_MM_SHUFFLE(1,0,1,0)); \
    __m128 _t6=_mm_shuffle_ps((r2),(r3),_MM_SHUFFLE(3,2,3,2)); \
    (r0)=_mm_shuffle_ps(_t3,_t7,_MM_SHUFFLE(2,0,2,0)); \
    (r1)=_mm_shuffle_ps(_t3,_t7,_MM_SHUFFLE(3,1,3,1)); \
    (r2)=_mm_shuffle_ps(_t2,_t6,_MM_SHUFFLE(2,0,2,0)); \
    (r3)=_mm_shuffle_ps(_t2,_t6,_MM_SHUFFLE(3,1,3,1)); \
}while(0)
#endif
static inline __m128 _mm_load_ps(const float*p){__m128 r;r.m128_f32[0]=p[0];r.m128_f32[1]=p[1];r.m128_f32[2]=p[2];r.m128_f32[3]=p[3];return r;}
static inline __m128 _mm_loadu_ps(const float*p){return _mm_load_ps(p);}
static inline void _mm_store_ps(float*p,__m128 a){p[0]=a.m128_f32[0];p[1]=a.m128_f32[1];p[2]=a.m128_f32[2];p[3]=a.m128_f32[3];}
static inline void _mm_storeu_ps(float*p,__m128 a){_mm_store_ps(p,a);}
static inline float _mm_cvtss_f32(__m128 a){return a.m128_f32[0];}
static inline __m128 _mm_rcp_ps(__m128 a){__m128 r;for(int i=0;i<4;++i)r.m128_f32[i]=1.0f/a.m128_f32[i];return r;}
static inline __m128 _mm_rsqrt_ps(__m128 a){__m128 r;for(int i=0;i<4;++i)r.m128_f32[i]=1.0f/sqrtf(a.m128_f32[i]);return r;}
#endif // __x86_64__
#endif // SSE_SCALAR_FALLBACK_H
