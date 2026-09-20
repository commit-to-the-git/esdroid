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

#include "touch_ui.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <android/log.h>
#include <stb/stb_truetype.h>
#include <stb/stb_image.h>

static unsigned char* s_fontBitmap = nullptr;
static unsigned char* s_fontBitmapBig = nullptr;
static stbtt_bakedchar s_fontChars[96];
static stbtt_bakedchar s_fontCharsBig[96];
static GLuint s_fontTexture = 0;
static GLuint s_fontTextureBig = 0;
static int s_fontLoaded = 0;
static const int FONT_TEX_SIZE = 512;
static const float FONT_SIZE = 20.0f;
static const float FONT_SIZE_BIG = 28.0f;
static float s_settingsLabelW = 0.0f; // real settings advance set at font load

// text rendering for button labels using the engines silkscreen font
static GLuint s_textProgram = 0;
static GLuint s_textVao = 0, s_textVbo = 0;
static GLint s_textLocScreen = -1, s_textLocColor = -1, s_textLocPos = -1, s_textLocUV = -1;

static void loadTouchFont() {
    if (s_fontLoaded) return;
    s_fontLoaded = 1;

    void* ttfData = nullptr;
    long ttfSize = 0;
    esdroid::AndroidBackend::instance().readAsset(
        "delta-engine-assets/fonts/Silkscreen/slkscr.ttf", &ttfData, &ttfSize);
    if (!ttfData || ttfSize <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, "ESDroid", "TouchUI: font load failed");
        return;
    }

    s_fontBitmap = new unsigned char[FONT_TEX_SIZE * FONT_TEX_SIZE];
    s_fontBitmapBig = new unsigned char[FONT_TEX_SIZE * FONT_TEX_SIZE];
    int result = stbtt_BakeFontBitmap((unsigned char*)ttfData, 0, FONT_SIZE, s_fontBitmap,
        FONT_TEX_SIZE, FONT_TEX_SIZE, 32, 96, s_fontChars);
    int resultBig = stbtt_BakeFontBitmap((unsigned char*)ttfData, 0, FONT_SIZE_BIG,
        s_fontBitmapBig, FONT_TEX_SIZE, FONT_TEX_SIZE, 32, 96, s_fontCharsBig);
    free(ttfData);
    if (result <= 0 || resultBig <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, "ESDroid", "TouchUI: bake failed");
        delete[] s_fontBitmap; s_fontBitmap = nullptr;
        delete[] s_fontBitmapBig; s_fontBitmapBig = nullptr;
        return;
    }

    // measure the real advance so the settings button is sized to its text
    for (const char* p = "SETTINGS"; *p; ++p) {
        if (*p < 32 || *p >= 128) continue;
        s_settingsLabelW += s_fontChars[*p - 32].xadvance;
    }

    glGenTextures(1, &s_fontTexture);
    glBindTexture(GL_TEXTURE_2D, s_fontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, FONT_TEX_SIZE, FONT_TEX_SIZE, 0,
                 GL_RED, GL_UNSIGNED_BYTE, s_fontBitmap);
    glGenTextures(1, &s_fontTextureBig);
    glBindTexture(GL_TEXTURE_2D, s_fontTextureBig);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, FONT_TEX_SIZE, FONT_TEX_SIZE, 0,
                 GL_RED, GL_UNSIGNED_BYTE, s_fontBitmapBig);
    glBindTexture(GL_TEXTURE_2D, 0);

    static const char* tvs = R"ES3(#version 300 es
        uniform vec2 uScreen;
        in vec2 aPos; in vec2 aUV; out vec2 vUV;
        void main(){
            gl_Position=vec4((aPos.x/uScreen.x)*2.0-1.0, 1.0-(aPos.y/uScreen.y)*2.0, 0.0, 1.0);
            vUV=aUV;
        })ES3";
    static const char* tfs = R"ES3(#version 300 es
        precision mediump float;
        uniform sampler2D uFontTex; uniform vec4 uColor;
        in vec2 vUV; out vec4 fragColor;
        void main(){
            float a=texture(uFontTex, vUV).r;
            fragColor=vec4(uColor.rgb, uColor.a*a);
        })ES3";

    auto compile = [](GLenum t, const char* src) -> GLuint {
        GLuint s = glCreateShader(t);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { glDeleteShader(s); return 0; }
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, tvs);
    GLuint fs = compile(GL_FRAGMENT_SHADER, tfs);
    s_textProgram = glCreateProgram();
    glAttachShader(s_textProgram, vs);
    glAttachShader(s_textProgram, fs);
    glLinkProgram(s_textProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
    s_textLocScreen = glGetUniformLocation(s_textProgram, "uScreen");
    s_textLocColor = glGetUniformLocation(s_textProgram, "uColor");
    s_textLocPos = glGetAttribLocation(s_textProgram, "aPos");
    s_textLocUV = glGetAttribLocation(s_textProgram, "aUV");

    glGenVertexArrays(1, &s_textVao);
    glGenBuffers(1, &s_textVbo);
    glBindVertexArray(s_textVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_textVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 6 * 64, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(s_textLocPos);
    glVertexAttribPointer(s_textLocPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(s_textLocUV);
    glVertexAttribPointer(s_textLocUV, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    __android_log_print(ANDROID_LOG_INFO, "ESDroid", "TouchUI: font+shader OK tex=%u prog=%u",
                        s_fontTexture, s_textProgram);
}

// the app icon is drawn over the info clusters logo box
static GLuint s_iconTexture = 0;
static GLuint s_iconProgram = 0;
static GLuint s_iconVao = 0, s_iconVbo = 0;
static GLint s_iconLocScreen = -1;
static float s_logoRect[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
static bool s_logoRectValid = false;

extern "C" void esdroid_set_logo_rect(float x, float y, float w, float h) {
    s_logoRect[0] = x; s_logoRect[1] = y; s_logoRect[2] = w; s_logoRect[3] = h;
    s_logoRectValid = true;
}

// the settings button derives from the published title box
extern "C" void esdroid_set_title_rect(float x, float y, float w, float h) {
    const float margin = 8.0f;
    float bh = h * 0.24f;
    if (bh > 28.0f) bh = 28.0f;
    if (bh < 16.0f) bh = 16.0f;
    const float labelW = (s_settingsLabelW > 0.0f) ? s_settingsLabelW
        : 8.0f * FONT_SIZE * 0.6f;
    const float bw = labelW + 26.0f; // settings + padding
    if (w - 2.0f * margin < bw || h - 2.0f * margin < bh) {
        esdroid::AndroidBackend::instance().setSettingsButtonRect(0, 0, 0, 0);
        return;
    }
    esdroid::AndroidBackend::instance().setSettingsButtonRect(
        x + w - margin - bw, y + h - margin - bh, bw, bh);
}

// called at the top of every renderscene an unpublished rect stops
// being tappable
extern "C" void esdroid_invalidate_ui_rects() {
    esdroid::AndroidBackend::instance().invalidateUiRects();
}

//
// frosted backdrop downsample the frame gaussian blur it stretch
// it back over the screen

static GLuint s_blurTexA = 0, s_blurTexB = 0;
static GLuint s_blurFboA = 0, s_blurFboB = 0;
static GLuint s_blurProgram = 0;
static GLuint s_blurVao = 0, s_blurVbo = 0;
static GLint s_blurLocScreen = -1, s_blurLocStep = -1;
static int s_blurW = 0, s_blurH = 0;

static void ensureBlurResources(int sw, int sh) {
    const int bw = sw > 6 ? sw / 6 : 1;
    const int bh = sh > 6 ? sh / 6 : 1;
    if (s_blurTexA != 0 && s_blurW == bw && s_blurH == bh) return;
    s_blurW = bw; s_blurH = bh;

    if (s_blurTexA == 0) {
        glGenTextures(1, &s_blurTexA);
        glGenTextures(1, &s_blurTexB);
        glGenFramebuffers(1, &s_blurFboA);
        glGenFramebuffers(1, &s_blurFboB);
        glGenVertexArrays(1, &s_blurVao);
        glGenBuffers(1, &s_blurVbo);
    }

    for (int t = 0; t < 2; ++t) {
        glBindTexture(GL_TEXTURE_2D, t == 0 ? s_blurTexA : s_blurTexB);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bw, bh, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, s_blurFboA);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_blurTexA, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, s_blurFboB);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_blurTexB, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (s_blurProgram == 0) {
        static const char* bvs = R"ES3(#version 300 es
            uniform vec2 uScreen;
            in vec2 aPos; in vec2 aUV; out vec2 vUV;
            void main(){
                gl_Position=vec4((aPos.x/uScreen.x)*2.0-1.0, 1.0-(aPos.y/uScreen.y)*2.0, 0.0, 1.0);
                vUV=aUV;
            })ES3";
        static const char* bfs = R"ES3(#version 300 es
            precision mediump float;
            uniform sampler2D uTex; uniform vec2 uStep;
            in vec2 vUV; out vec4 fragColor;
            void main(){
                vec4 c=vec4(0.0);
                c+=texture(uTex, vUV+uStep*-4.0)*0.01621622;
                c+=texture(uTex, vUV+uStep*-3.0)*0.05405405;
                c+=texture(uTex, vUV+uStep*-2.0)*0.12162162;
                c+=texture(uTex, vUV+uStep*-1.0)*0.19459459;
                c+=texture(uTex, vUV)*0.22702703;
                c+=texture(uTex, vUV+uStep*1.0)*0.19459459;
                c+=texture(uTex, vUV+uStep*2.0)*0.12162162;
                c+=texture(uTex, vUV+uStep*3.0)*0.05405405;
                c+=texture(uTex, vUV+uStep*4.0)*0.01621622;
                fragColor=c;
            })ES3";
        auto compile = [](GLenum t, const char* src) -> GLuint {
            GLuint s = glCreateShader(t);
            glShaderSource(s, 1, &src, nullptr);
            glCompileShader(s);
            GLint ok = 0;
            glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
            if (!ok) { glDeleteShader(s); return 0; }
            return s;
        };
        GLuint vs = compile(GL_VERTEX_SHADER, bvs);
        GLuint fs = compile(GL_FRAGMENT_SHADER, bfs);
        s_blurProgram = glCreateProgram();
        glAttachShader(s_blurProgram, vs);
        glAttachShader(s_blurProgram, fs);
        glLinkProgram(s_blurProgram);
        glDeleteShader(vs);
        glDeleteShader(fs);
        s_blurLocScreen = glGetUniformLocation(s_blurProgram, "uScreen");
        s_blurLocStep = glGetUniformLocation(s_blurProgram, "uStep");
        const GLint locPos = glGetAttribLocation(s_blurProgram, "aPos");
        const GLint locUV = glGetAttribLocation(s_blurProgram, "aUV");
        glBindVertexArray(s_blurVao);
        glBindBuffer(GL_ARRAY_BUFFER, s_blurVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 6, nullptr, GL_DYNAMIC_DRAW);
        if (locPos >= 0) {
            glEnableVertexAttribArray(locPos);
            glVertexAttribPointer(locPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        }
        if (locUV >= 0) {
            glEnableVertexAttribArray(locUV);
            glVertexAttribPointer(locUV, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        }
        glBindVertexArray(0);
    }
}

// draws a fullscreen rect of the bound framebuffer zero step is a
// plain copy
static void drawBlurQuad(float w, float h, GLuint tex, float stepX, float stepY) {
    const float verts[4 * 6] = {
        0, 0, 0, 0,
        w, 0, 1, 0,
        0, h, 0, 1,
        0, h, 0, 1,
        w, 0, 1, 0,
        w, h, 1, 1,
    };
    glUseProgram(s_blurProgram);
    glBindVertexArray(s_blurVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_blurVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glUniform2f(s_blurLocScreen, w, h);
    glUniform2f(s_blurLocStep, stepX, stepY);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

static void loadTouchIcon() {
    if (s_iconTexture != 0) return;

    void* iconData = nullptr;
    long iconSize = 0;
    esdroid::AndroidBackend::instance().readAsset("esdroid-icon.png", &iconData, &iconSize);
    if (!iconData || iconSize <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, "ESDroid", "TouchUI: icon load failed");
        return;
    }

    int w = 0, h = 0, n = 0;
    unsigned char* pixels = stbi_load_from_memory(
        (const stbi_uc*)iconData, (int)iconSize, &w, &h, &n, 4);
    free(iconData);
    if (!pixels || w <= 0 || h <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, "ESDroid", "TouchUI: icon decode failed");
        if (pixels) stbi_image_free(pixels);
        return;
    }

    glGenTextures(1, &s_iconTexture);
    glBindTexture(GL_TEXTURE_2D, s_iconTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);

    static const char* ivs = R"ES3(#version 300 es
        uniform vec2 uScreen;
        in vec2 aPos; in vec2 aUV; out vec2 vUV;
        void main(){
            gl_Position=vec4((aPos.x/uScreen.x)*2.0-1.0, 1.0-(aPos.y/uScreen.y)*2.0, 0.0, 1.0);
            vUV=aUV;
        })ES3";
    static const char* ifs = R"ES3(#version 300 es
        precision mediump float;
        uniform sampler2D uTex;
        in vec2 vUV; out vec4 fragColor;
        void main(){ fragColor=texture(uTex, vUV); })ES3";

    auto compile = [](GLenum t, const char* src) -> GLuint {
        GLuint s = glCreateShader(t);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { glDeleteShader(s); return 0; }
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, ivs);
    GLuint fs = compile(GL_FRAGMENT_SHADER, ifs);
    if (vs == 0 || fs == 0) {
        __android_log_print(ANDROID_LOG_ERROR, "ESDroid", "TouchUI: icon shader compile FAILED");
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return;
    }
    s_iconProgram = glCreateProgram();
    glAttachShader(s_iconProgram, vs);
    glAttachShader(s_iconProgram, fs);
    glLinkProgram(s_iconProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
    s_iconLocScreen = glGetUniformLocation(s_iconProgram, "uScreen");

    GLint locPos = glGetAttribLocation(s_iconProgram, "aPos");
    GLint locUV = glGetAttribLocation(s_iconProgram, "aUV");
    if (locPos < 0 || locUV < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "ESDroid", "TouchUI: icon attribs missing");
        return;
    }
    glGenVertexArrays(1, &s_iconVao);
    glGenBuffers(1, &s_iconVbo);
    glBindVertexArray(s_iconVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_iconVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 6, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(locPos);
    glVertexAttribPointer(locPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(locUV);
    glVertexAttribPointer(locUV, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    __android_log_print(ANDROID_LOG_INFO, "ESDroid", "TouchUI: icon OK %dx%d tex=%u prog=%u",
                        w, h, s_iconTexture, s_iconProgram);
}

static void drawTouchIcon(float screenW, float screenH) {
    const bool valid = s_logoRectValid;
    s_logoRectValid = false;
    if (!valid || s_iconTexture == 0 || s_iconProgram == 0 || s_iconVao == 0) return;

    const float x = s_logoRect[0], y = s_logoRect[1], w = s_logoRect[2], h = s_logoRect[3];
    float verts[4 * 6] = {
        x,     y,     0.0f, 0.0f,
        x + w, y,     1.0f, 0.0f,
        x,     y + h, 0.0f, 1.0f,
        x,     y + h, 0.0f, 1.0f,
        x + w, y,     1.0f, 0.0f,
        x + w, y + h, 1.0f, 1.0f,
    };

    glUseProgram(s_iconProgram);
    glBindVertexArray(s_iconVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_iconVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glUniform2f(s_iconLocScreen, screenW, screenH);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_iconTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

static void drawTouchText(const char* text, float x, float y, float screenW, float screenH,
                           const float* color, bool big) {
    const stbtt_bakedchar* chars = big ? s_fontCharsBig : s_fontChars;
    const GLuint tex = big ? s_fontTextureBig : s_fontTexture;
    const float fontSize = big ? FONT_SIZE_BIG : FONT_SIZE;
    if (!s_fontBitmap || tex == 0 || s_textProgram == 0) return;

    float verts[4 * 6 * 64]; // pos.xy uv.xy per vertex 6 verts per quad max 64 chars
    int nVerts = 0;
    float penX = x, penY = y + fontSize; // stbtt pen y is the baseline
    int len = strlen(text);

    for (int i = 0; i < len && i < 64; i++) {
        if (text[i] < 32 || text[i] >= 128) continue;
        stbtt_aligned_quad q;
        float qx = penX, qy = penY;
        stbtt_GetBakedQuad(chars, FONT_TEX_SIZE, FONT_TEX_SIZE,
                           text[i] - 32, &qx, &qy, &q, 1);
        float* v = &verts[nVerts];
        v[0]=q.x0; v[1]=q.y0; v[2]=q.s0; v[3]=q.t0;
        v[4]=q.x1; v[5]=q.y0; v[6]=q.s1; v[7]=q.t0;
        v[8]=q.x0; v[9]=q.y1; v[10]=q.s0; v[11]=q.t1;
        v[12]=q.x0; v[13]=q.y1; v[14]=q.s0; v[15]=q.t1;
        v[16]=q.x1; v[17]=q.y0; v[18]=q.s1; v[19]=q.t0;
        v[20]=q.x1; v[21]=q.y1; v[22]=q.s1; v[23]=q.t1;
        nVerts += 24;
        penX = qx;
    }

    if (nVerts == 0) return;

    glUseProgram(s_textProgram);
    glBindVertexArray(s_textVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_textVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, nVerts * sizeof(float), verts);
    glUniform2f(s_textLocScreen, screenW, screenH);
    glUniform4fv(s_textLocColor, 1, color);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glDrawArrays(GL_TRIANGLES, 0, nVerts / 4);
    glBindVertexArray(0);
    glUseProgram(0);
}

// loading screen drawn straight into the gl surface so it shows even
// while the engine compiles
static GLuint s_loadProgram = 0;
static GLuint s_loadVao = 0, s_loadVbo = 0;
static GLint s_loadLocScreen = -1, s_loadLocColor = -1;
static GLuint s_loadWhiteTex = 0;
static double s_loadLastFrame = -1.0;

static double loadClockSeconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static void ensureLoadingResources() {
    if (s_loadProgram != 0) return;

    static const char* lvs = R"ES3(#version 300 es
        uniform vec2 uScreen;
        in vec2 aPos; in vec2 aUV; out vec2 vUV;
        void main(){
            gl_Position=vec4((aPos.x/uScreen.x)*2.0-1.0, 1.0-(aPos.y/uScreen.y)*2.0, 0.0, 1.0);
            vUV=aUV;
        })ES3";
    static const char* lfs = R"ES3(#version 300 es
        precision mediump float;
        uniform sampler2D uTex; uniform vec4 uColor;
        in vec2 vUV; out vec4 fragColor;
        void main(){ fragColor=texture(uTex, vUV)*uColor; })ES3";

    auto compile = [](GLenum t, const char* src) -> GLuint {
        GLuint s = glCreateShader(t);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { glDeleteShader(s); return 0; }
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, lvs);
    GLuint fs = compile(GL_FRAGMENT_SHADER, lfs);
    if (vs == 0 || fs == 0) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return;
    }
    s_loadProgram = glCreateProgram();
    glAttachShader(s_loadProgram, vs);
    glAttachShader(s_loadProgram, fs);
    glLinkProgram(s_loadProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);
    s_loadLocScreen = glGetUniformLocation(s_loadProgram, "uScreen");
    s_loadLocColor = glGetUniformLocation(s_loadProgram, "uColor");
    GLint locPos = glGetAttribLocation(s_loadProgram, "aPos");
    GLint locUV = glGetAttribLocation(s_loadProgram, "aUV");

    glGenVertexArrays(1, &s_loadVao);
    glGenBuffers(1, &s_loadVbo);
    glBindVertexArray(s_loadVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_loadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 6, nullptr, GL_DYNAMIC_DRAW);
    if (locPos >= 0) {
        glEnableVertexAttribArray(locPos);
        glVertexAttribPointer(locPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    }
    if (locUV >= 0) {
        glEnableVertexAttribArray(locUV);
        glVertexAttribPointer(locUV, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }
    glBindVertexArray(0);

    // one white pixel so the same shader draws the black backdrop
    unsigned char white[4] = { 255, 255, 255, 255 };
    glGenTextures(1, &s_loadWhiteTex);
    glBindTexture(GL_TEXTURE_2D, s_loadWhiteTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void drawLoadingQuadVerts(const float* pos12, GLuint tex,
        const float* color, float screenW, float screenH) {
    const float uv[12] = {
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    };
    float verts[4 * 6];
    for (int i = 0; i < 6; ++i) {
        verts[i * 4 + 0] = pos12[i * 2 + 0];
        verts[i * 4 + 1] = pos12[i * 2 + 1];
        verts[i * 4 + 2] = uv[i * 2 + 0];
        verts[i * 4 + 3] = uv[i * 2 + 1];
    }

    glUseProgram(s_loadProgram);
    glBindVertexArray(s_loadVao);
    glBindBuffer(GL_ARRAY_BUFFER, s_loadVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glUniform2f(s_loadLocScreen, screenW, screenH);
    glUniform4fv(s_loadLocColor, 1, color);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

static void drawLoadingQuadRect(float x0, float y0, float x1, float y1,
        GLuint tex, const float* color, float screenW, float screenH) {
    const float pos[12] = {
        x0, y0, x1, y0, x0, y1,
        x0, y1, x1, y0, x1, y1,
    };
    drawLoadingQuadVerts(pos, tex, color, screenW, screenH);
}

// draws the spinning icon and the loading text
// alpha under one blends it over the engine frame
static void loadingDrawContents(float alpha, bool clearScreen) {
    if (!esdroid::AndroidBackend::instance().isWindowReady()) return;

    loadTouchFont();
    loadTouchIcon();
    ensureLoadingResources();

    const int sw = esdroid::AndroidBackend::instance().screenWidth();
    const int sh = esdroid::AndroidBackend::instance().screenHeight();
    if (sw <= 0 || sh <= 0) return;

    GLint prevProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    GLint prevFbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevFbo);
    const GLboolean prevDepth = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean prevBlend = glIsEnabled(GL_BLEND);
    const GLboolean prevCull = glIsEnabled(GL_CULL_FACE);
    const GLboolean prevScissor = glIsEnabled(GL_SCISSOR_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, sw, sh);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (clearScreen) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    else if (s_loadProgram != 0) {
        // fading over the engine frame so the fade needs a black sheet
        const float black[4] = { 0.0f, 0.0f, 0.0f, alpha };
        drawLoadingQuadRect(0.0f, 0.0f, (float)sw, (float)sh,
            s_loadWhiteTex, black, (float)sw, (float)sh);
    }

    const float fw = (float)sw, fh = (float)sh;
    const float iconSize = (fw < fh ? fw : fh) * 0.30f;
    const float gap = (fw < fh ? fw : fh) * 0.035f;
    const float textH = FONT_SIZE_BIG * 1.25f;

    float textW = 0.0f;
    for (const char* p = "LOADING..."; *p; ++p) {
        if (*p < 32 || *p >= 128) continue;
        textW += s_fontCharsBig[*p - 32].xadvance;
    }

    const float blockH = iconSize + gap + textH;
    const float iconCy = fh * 0.5f - blockH * 0.5f + iconSize * 0.5f;
    const float cx = fw * 0.5f;

    if (s_iconTexture != 0 && s_loadProgram != 0) {
        // one turn every 900 ms
        const double now = loadClockSeconds();
        const float angle = (float)fmod(now / 0.9, 1.0) * 6.2831853f;
        const float cs = cosf(angle), sn = sinf(angle);
        const float r = iconSize * 0.5f;
        // rotate the corner offsets around the icon center
        const float dx[4] = { -r, r, -r, r };
        const float dy[4] = { -r, -r, r, r };
        float corner[4][2];
        for (int i = 0; i < 4; ++i) {
            corner[i][0] = cx + dx[i] * cs - dy[i] * sn;
            corner[i][1] = iconCy + dx[i] * sn + dy[i] * cs;
        }
        const float pos[12] = {
            corner[0][0], corner[0][1],
            corner[1][0], corner[1][1],
            corner[2][0], corner[2][1],
            corner[2][0], corner[2][1],
            corner[1][0], corner[1][1],
            corner[3][0], corner[3][1],
        };
        const float tint[4] = { 1.0f, 1.0f, 1.0f, alpha };
        drawLoadingQuadVerts(pos, s_iconTexture, tint, fw, fh);
    }

    if (s_fontBitmapBig != nullptr) {
        const float white[4] = { 1.0f, 1.0f, 1.0f, alpha };
        drawTouchText("LOADING...", cx - textW * 0.5f,
            fh * 0.5f - blockH * 0.5f + iconSize + gap, fw, fh, white, true);
    }

    glDisable(GL_BLEND);
    if (prevScissor) glEnable(GL_SCISSOR_TEST);
    if (prevCull) glEnable(GL_CULL_FACE);
    if (prevDepth) glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevFbo);
    glUseProgram(prevProgram);
}

// one full loading frame with its own buffer swap
extern "C" void esdroid_render_loading_frame() {
    if (!esdroid::AndroidBackend::instance().isWindowReady()) return;

    // cap at 60 fps the compile poll loop spins much faster
    const double now = loadClockSeconds();
    if (s_loadLastFrame >= 0.0 && now - s_loadLastFrame < 0.016) return;
    s_loadLastFrame = now;

    loadingDrawContents(1.0f, true);
    esdroid::AndroidBackend::instance().swapBuffers();
}

// transparent pass over the engine frame for the fade out
extern "C" void esdroid_draw_loading_overlay(float alpha) {
    if (alpha <= 0.0f) return;
    loadingDrawContents(alpha, false);
}

namespace esdroid {
TouchUI::TouchUI() {}
TouchUI::~TouchUI() {}

// ink box of a baked string silkscreen is not monospace so this is
// a guess
static void bakedTextInk(const char* text, const stbtt_bakedchar* chars,
                          float* advance, float* top, float* bottom) {
    float adv = 0.0f, t = 0.0f, b = 0.0f;
    for (const char* p = text; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c < 32 || c >= 128) continue;
        const stbtt_bakedchar& ch = chars[c - 32];
        adv += ch.xadvance;
        if (ch.yoff < t) t = ch.yoff;
        const float bot = ch.yoff + (float)(ch.y1 - ch.y0);
        if (bot > b) b = bot;
    }
    *advance = adv; *top = t; *bottom = b;
}
void TouchUI::initialize(int sw,int sh) { m_screenW=sw; m_screenH=sh; compileShaders(); generateQuadGeometry(); loadTouchFont(); loadTouchIcon(); }
void TouchUI::resize(int sw,int sh) { m_screenW=sw; m_screenH=sh; }
void TouchUI::compileShaders() {
    static const char* kVS=R"ES3(#version 300 es
        uniform vec4 uColor; uniform vec4 uRect; uniform vec2 uScreen;
        in vec2 aPos; out vec4 vColor;
        void main(){float px=uRect.x+aPos.x*uRect.z; float py=uRect.y+aPos.y*uRect.w;
        gl_Position=vec4((px/uScreen.x)*2.0-1.0,1.0-(py/uScreen.y)*2.0,0.0,1.0); vColor=uColor;})ES3";
    static const char* kFS=R"ES3(#version 300 es
        precision mediump float; in vec4 vColor; out vec4 fragColor;
        void main(){fragColor=vColor;})ES3";
    auto compile=[](GLenum t,const char* src)->GLuint{
        GLuint s=glCreateShader(t); glShaderSource(s,1,&src,nullptr); glCompileShader(s);
        GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
        if(!ok){char log[1024]; glGetShaderInfoLog(s,1024,nullptr,log); glDeleteShader(s); return 0;}
        return s;
    };
    GLuint vs=compile(GL_VERTEX_SHADER,kVS), fs=compile(GL_FRAGMENT_SHADER,kFS);
    if(vs==0||fs==0){__android_log_print(ANDROID_LOG_ERROR,"ESDroid","TouchUI shader compile FAILED");return;}
    m_program=glCreateProgram(); glAttachShader(m_program,vs); glAttachShader(m_program,fs); glLinkProgram(m_program);
    glDeleteShader(vs); glDeleteShader(fs);
    m_loc_color=glGetUniformLocation(m_program,"uColor");
    m_loc_rect=glGetUniformLocation(m_program,"uRect");
    m_loc_screen=glGetUniformLocation(m_program,"uScreen");
    m_loc_pos=glGetAttribLocation(m_program,"aPos");
}
void TouchUI::generateQuadGeometry() {
    float q[]={0,0, 0,1, 1,0, 0,1, 1,1, 1,0};
    glGenVertexArrays(1,&m_vao); glGenBuffers(1,&m_vbo);
    glBindVertexArray(m_vao); glBindBuffer(GL_ARRAY_BUFFER,m_vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(q),q,GL_STATIC_DRAW);
    glEnableVertexAttribArray(m_loc_pos);
    glVertexAttribPointer(m_loc_pos,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
    glBindVertexArray(0);
}
void TouchUI::drawButton(const TouchButton& b, bool held) {
    float bg[4]={0.055f,0.063f,0.071f,0.85f};
    float border[4]={1,1,1,0.9f};
    if(held){bg[0]=0.937f;bg[1]=0.271f;bg[2]=0.271f;bg[3]=0.85f;}
    glUseProgram(m_program); glBindVertexArray(m_vao);
    glUniform2f(m_loc_screen,(float)m_screenW,(float)m_screenH);
    glUniform4f(m_loc_rect,b.x+1,b.y+1,b.w-2,b.h-2); glUniform4fv(m_loc_color,1,bg);
    glDrawArrays(GL_TRIANGLES,0,6);
    float bt=2.0f; glUniform4fv(m_loc_color,1,border);
    glUniform4f(m_loc_rect,b.x,b.y,b.w,bt); glDrawArrays(GL_TRIANGLES,0,6);
    glUniform4f(m_loc_rect,b.x,b.y+b.h-bt,b.w,bt); glDrawArrays(GL_TRIANGLES,0,6);
    glUniform4f(m_loc_rect,b.x,b.y,bt,b.h); glDrawArrays(GL_TRIANGLES,0,6);
    glUniform4f(m_loc_rect,b.x+b.w-bt,b.y,bt,b.h); glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0); glUseProgram(0);

    const bool fn=AndroidBackend::instance().fnActive();
    const char* label=(fn&&b.altLabel)?b.altLabel:b.label;
    if (label) {
        const float labelColor[4]={1.0f,1.0f,1.0f,0.95f};
        float tw = strlen(label) * FONT_SIZE * 0.6f;
        float tx = b.x + (b.w - tw) / 2.0f;
        float ty = b.y + (b.h - FONT_SIZE) / 2.0f - 2.0f;
        drawTouchText(label, tx, ty, (float)m_screenW, (float)m_screenH, labelColor, false);
    }
}
void TouchUI::drawRect(float x, float y, float w, float h, const float* color) {
    if (w <= 0 || h <= 0) return;
    glUseProgram(m_program); glBindVertexArray(m_vao);
    glUniform2f(m_loc_screen,(float)m_screenW,(float)m_screenH);
    glUniform4f(m_loc_rect,x,y,w,h); glUniform4fv(m_loc_color,1,color);
    glDrawArrays(GL_TRIANGLES,0,6);
    glBindVertexArray(0); glUseProgram(0);
}

// the settings button on the info clusters title box white on black
// the two colors the engine ui itself uses
void TouchUI::drawSettingsButton() {
    auto& backend=AndroidBackend::instance();
    if(!backend.settingsButtonValid()) return;
    const float* r=backend.settingsButtonRect();
    const float white[4]={1.0f,1.0f,1.0f,1.0f};
    const float black[4]={0.0f,0.0f,0.0f,1.0f};
    drawRect(r[0],r[1],r[2],r[3],white);
    const char* label="SETTINGS";
    float adv,inkTop,inkBot;
    bakedTextInk(label,s_fontChars,&adv,&inkTop,&inkBot);
    // center the actual ink box not a guessed em box
    const float baseline=r[1]+(r[3]-(inkBot-inkTop))*0.5f-inkTop;
    drawTouchText(label,r[0]+(r[2]-adv)*0.5f,baseline-FONT_SIZE,
        (float)m_screenW,(float)m_screenH,black,false);
}

void TouchUI::drawBlurredBackdrop() {
    const float sw=(float)m_screenW, sh=(float)m_screenH;
    ensureBlurResources(m_screenW,m_screenH);
    const float scrim[4]={0.0f,0.0f,0.0f,0.45f};
    if(s_blurProgram==0||s_blurFboA==0) {
        drawRect(0,0,sw,sh,scrim);
        return;
    }
    glDisable(GL_BLEND);
    // capture the frame the reversed rect flips v so v=0 is the top
    glBindFramebuffer(GL_READ_FRAMEBUFFER,0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,s_blurFboA);
    glBlitFramebuffer(0,0,m_screenW,m_screenH, 0,s_blurH,s_blurW,0,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,s_blurFboB);
    glViewport(0,0,s_blurW,s_blurH);
    drawBlurQuad((float)s_blurW,(float)s_blurH,s_blurTexA,1.0f/(float)s_blurW,0.0f);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,s_blurFboA);
    glViewport(0,0,s_blurW,s_blurH);
    drawBlurQuad((float)s_blurW,(float)s_blurH,s_blurTexB,0.0f,1.0f/(float)s_blurH);
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    glViewport(0,0,m_screenW,m_screenH);
    drawBlurQuad(sw,sh,s_blurTexA,0.0f,0.0f);
    glEnable(GL_BLEND);
    drawRect(0,0,sw,sh,scrim);
}

void TouchUI::drawSettingsPanel() {
    auto& backend=AndroidBackend::instance();
    const SettingsLayout& L=backend.settingsLayout();
    const float sw=(float)m_screenW, sh=(float)m_screenH;
    const float white[4]={1.0f,1.0f,1.0f,1.0f};
    const float black[4]={0.0f,0.0f,0.0f,1.0f};

    drawBlurredBackdrop();

    drawRect(L.panel[0],L.panel[1],L.panel[2],L.panel[3],white);
    auto border=[&](const float* r,float t,const float* c){
        drawRect(r[0],r[1],r[2],t,c);
        drawRect(r[0],r[1]+r[3]-t,r[2],t,c);
        drawRect(r[0],r[1],t,r[3],c);
        drawRect(r[0]+r[2]-t,r[1],t,r[3],c);
    };
    border(L.panel,3.0f,black);

    // header big title + close button in the panels own font colors
    drawTouchText("SETTINGS",L.panel[0]+22.0f,L.close[1]+(L.close[3]-FONT_SIZE_BIG)*0.5f-2.0f,
        sw,sh,black,true);
    drawRect(L.close[0],L.close[1],L.close[2],L.close[3],white);
    border(L.close,2.0f,black);
    {
        const char* x="CLOSE";
        const float tw=(float)strlen(x)*FONT_SIZE*0.6f;
        drawTouchText(x,L.close[0]+(L.close[2]-tw)*0.5f,
            L.close[1]+(L.close[3]-FONT_SIZE)*0.5f-2.0f,sw,sh,black,false);
    }

    for(int i=0;i<esdroid::kSettingCount;++i) {
        const float* t=L.track[i];
        const float* v=L.value[i];
        const float cy=t[1]+t[3]*0.5f;

        drawTouchText(backend.settingLabel(i),L.panel[0]+18.0f,cy-FONT_SIZE*0.5f-1.0f,
            sw,sh,black,false);

        // slider black outline black fill bar black knob
        border(t,2.0f,black);
        const float tt=backend.settingSliderT(i);
        drawRect(t[0]+2.0f,t[1]+2.0f,(t[2]-4.0f)*tt,t[3]-4.0f,black);
        const float kw=12.0f, kh=t[3]+14.0f;
        float kx=t[0]+t[2]*tt-kw*0.5f;
        if(kx<t[0]) kx=t[0];
        if(kx>t[0]+t[2]-kw) kx=t[0]+t[2]-kw;
        drawRect(kx,cy-kh*0.5f,kw,kh,black);

        // value box tappable opens the system keyboard for typing
        drawRect(v[0],v[1],v[2],v[3],white);
        border(v,2.0f,black);
        char buf[32];
        backend.formatSettingValue(buf,sizeof(buf),i);
        const float tw=(float)strlen(buf)*FONT_SIZE*0.6f;
        drawTouchText(buf,v[0]+(v[2]-tw)*0.5f,v[1]+(v[3]-FONT_SIZE)*0.5f-2.0f,
            sw,sh,black,false);
    }
}

// one dropdown the selector box always the scrollable entry list below it
// when open entries clip to the list rect through the scissor test and the
// selected entry renders inverted
void TouchUI::drawDropdown(const float* box,const MrAsset* entries,int count,
        int sel,bool open,float scroll,float listTop,float listH,float rowH) {
    const float sw=(float)m_screenW, sh=(float)m_screenH;
    const float white[4]={1.0f,1.0f,1.0f,1.0f};
    const float black[4]={0.0f,0.0f,0.0f,1.0f};
    auto border=[&](const float* r,float t,const float* c){
        drawRect(r[0],r[1],r[2],t,c);
        drawRect(r[0],r[1]+r[3]-t,r[2],t,c);
        drawRect(r[0],r[1],t,r[3],c);
        drawRect(r[0]+r[2]-t,r[1],t,r[3],c);
    };
    auto centerText=[&](const char* text,const float* r,const float* color){
        float adv,inkTop,inkBot;
        bakedTextInk(text,s_fontChars,&adv,&inkTop,&inkBot);
        const float baseline=r[1]+(r[3]-(inkBot-inkTop))*0.5f-inkTop;
        drawTouchText(text,r[0]+(r[2]-adv)*0.5f,baseline-FONT_SIZE,
            sw,sh,color,false);
    };

    const char* label=(sel>=0&&sel<count)?entries[sel].name:"";
    drawRect(box[0],box[1],box[2],box[3],white);
    border(box,2.0f,black);
    // the label clips to the box so long engine names cannot bleed out
    const float arrowW=26.0f;
    {
        const float textRect[4]={box[0]+4.0f,box[1],box[2]-arrowW-8.0f,box[3]};
        GLint sx=(GLint)textRect[0], sy=(GLint)(sh-(textRect[1]+textRect[3]));
        GLint scw=(GLint)textRect[2], sch=(GLint)textRect[3];
        glEnable(GL_SCISSOR_TEST);
        glScissor(sx,sy,scw>0?scw:0,sch>0?sch:0);
        float adv,inkTop,inkBot;
        bakedTextInk(label,s_fontChars,&adv,&inkTop,&inkBot);
        const float baseline=box[1]+(box[3]-(inkBot-inkTop))*0.5f-inkTop;
        // left aligned unlike the buttons the widest names would not fit
        drawTouchText(label,box[0]+8.0f,baseline-FONT_SIZE,sw,sh,black,false);
        glDisable(GL_SCISSOR_TEST);
    }
    {
        const float arrowRect[4]={box[0]+box[2]-arrowW,box[1],arrowW,box[3]};
        centerText(open?"^":"v",arrowRect,black);
    }

    if(!open) return;

    const float list[4]={box[0],listTop,box[2],listH};
    drawRect(list[0],list[1],list[2],list[3],white);
    border(list,2.0f,black);
    const float full=(float)count*rowH;
    if(full>listH) {
        const float thumbH=listH*listH/full;
        const float trackH=listH-thumbH;
        const float thumbY=listTop+(scroll/(full-listH))*trackH;
        drawRect(list[0]+list[2]-6.0f,listTop,4.0f,listH,black);
        drawRect(list[0]+list[2]-7.0f,thumbY,6.0f,thumbH,white);
    }
    GLint sx=(GLint)list[0], sy=(GLint)(sh-(list[1]+list[3]));
    GLint scw=(GLint)list[2], sch=(GLint)list[3];
    glEnable(GL_SCISSOR_TEST);
    glScissor(sx,sy,scw>0?scw:0,sch>0?sch:0);
    const int first=(int)(scroll/rowH);
    for(int i=first;i<=first+(int)(listH/rowH)+1&&i<count;++i) {
        const float row[4]={list[0],listTop+(float)i*rowH-scroll,list[2],rowH};
        if(row[1]+row[3]<=list[1]) continue;
        if(row[1]>=list[1]+list[3]) break;
        const bool selected=(i==sel);
        if(selected) drawRect(row[0],row[1],row[2],row[3],black);
        const float* textColor=selected?white:black;
        const char* text=entries[i].name;
        float adv,inkTop,inkBot;
        bakedTextInk(text,s_fontChars,&adv,&inkTop,&inkBot);
        const float baseline=row[1]+(row[3]-(inkBot-inkTop))*0.5f-inkTop;
        drawTouchText(text,row[0]+8.0f,baseline-FONT_SIZE,sw,sh,textColor,false);
    }
    glDisable(GL_SCISSOR_TEST);
}

// the import overlay themes on the left half engines on the right each
// with a dropdown a load button and a custom file import button
void TouchUI::drawImportPanel() {
    auto& backend=AndroidBackend::instance();
    const ImportLayout& L=backend.importLayout();
    const ImportMenuState& menu=backend.importMenu();
    const float sw=(float)m_screenW, sh=(float)m_screenH;
    const float white[4]={1.0f,1.0f,1.0f,1.0f};
    const float black[4]={0.0f,0.0f,0.0f,1.0f};

    drawBlurredBackdrop();

    drawRect(L.panel[0],L.panel[1],L.panel[2],L.panel[3],white);
    auto border=[&](const float* r,float t,const float* c){
        drawRect(r[0],r[1],r[2],t,c);
        drawRect(r[0],r[1]+r[3]-t,r[2],t,c);
        drawRect(r[0],r[1],t,r[3],c);
        drawRect(r[0]+r[2]-t,r[1],t,r[3],c);
    };
    border(L.panel,3.0f,black);

    drawTouchText("IMPORT",L.panel[0]+22.0f,
        L.close[1]+(L.close[3]-FONT_SIZE_BIG)*0.5f-2.0f,sw,sh,black,true);
    drawRect(L.close[0],L.close[1],L.close[2],L.close[3],white);
    border(L.close,2.0f,black);
    {
        const char* x="CLOSE";
        const float tw=(float)strlen(x)*FONT_SIZE*0.6f;
        drawTouchText(x,L.close[0]+(L.close[2]-tw)*0.5f,
            L.close[1]+(L.close[3]-FONT_SIZE)*0.5f-2.0f,sw,sh,black,false);
    }

    int themeCount=0, engineCount=0;
    const MrAsset* themes=backend.importThemes(&themeCount);
    const MrAsset* engines=backend.importEngines(&engineCount);

    // column titles sit above the selector boxes
    drawTouchText("THEME",L.themeBox[0],L.themeBox[1]-30.0f,sw,sh,black,false);
    drawTouchText("ENGINE",L.engineBox[0],L.engineBox[1]-30.0f,sw,sh,black,false);

    // selector boxes first the buttons then an open list back on top so
    // it covers the buttons it overlaps
    drawDropdown(L.themeBox,themes,themeCount,menu.themeSel,
        false,menu.themeScroll,L.listTop,L.listH,L.rowH);
    drawDropdown(L.engineBox,engines,engineCount,menu.engineSel,
        false,menu.engineScroll,L.listTop,L.listH,L.rowH);

    auto button=[&](const float* r,const char* text){
        drawRect(r[0],r[1],r[2],r[3],white);
        border(r,2.0f,black);
        const float tw=(float)strlen(text)*FONT_SIZE*0.6f;
        drawTouchText(text,r[0]+(r[2]-tw)*0.5f,
            r[1]+(r[3]-FONT_SIZE)*0.5f-2.0f,sw,sh,black,false);
    };
    button(L.themeLoad,"LOAD THEME");
    button(L.themeImport,"IMPORT CUSTOM THEME");
    button(L.engineLoad,"LOAD ENGINE");
    button(L.engineImport,"IMPORT CUSTOM ENGINE");

    // an open list draws again on top of the buttons it overlaps
    if(menu.themeListOpen)
        drawDropdown(L.themeBox,themes,themeCount,menu.themeSel,
            true,menu.themeScroll,L.listTop,L.listH,L.rowH);
    if(menu.engineListOpen)
        drawDropdown(L.engineBox,engines,engineCount,menu.engineSel,
            true,menu.engineScroll,L.listTop,L.listH,L.rowH);
}

void TouchUI::render() {
    if (m_program == 0) return;
    GLint prevProgram; glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    GLint prevFbo; glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevFbo);
    GLboolean prevDepth = glIsEnabled(GL_DEPTH_TEST);
    GLboolean prevBlend = glIsEnabled(GL_BLEND);
    GLboolean prevCull = glIsEnabled(GL_CULL_FACE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_screenW, m_screenH);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawTouchIcon((float)m_screenW, (float)m_screenH);

    if (AndroidBackend::instance().settingsOpen()) {
        // buttons vanish behind the frosted panel the engine keeps
        // simulating under the blur
        drawSettingsPanel();
    }
    else if (AndroidBackend::instance().importMenuOpen()) {
        drawImportPanel();
    }
    else {
        drawSettingsButton();

        const bool fn=AndroidBackend::instance().fnActive();
        for (const auto& b : AndroidBackend::instance().buttons())
            drawButton(b, b.key==VirtualKey::Fn ? fn : b.held);
    }

    glDisable(GL_BLEND);
    if (prevCull) glEnable(GL_CULL_FACE);
    if (prevDepth) glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevFbo);
    glUseProgram(prevProgram);
}
} // namespace esdroid
