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
#include <android/log.h>
#include <stb/stb_truetype.h>
#include <stb/stb_image.h>

static unsigned char* s_fontBitmap = nullptr;
static stbtt_bakedchar s_fontChars[96];
static GLuint s_fontTexture = 0;
static int s_fontLoaded = 0;
static const int FONT_TEX_SIZE = 512;
static const float FONT_SIZE = 20.0f;

// Text rendering for button labels, using the engine's Silkscreen font.
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
    int result = stbtt_BakeFontBitmap((unsigned char*)ttfData, 0, FONT_SIZE, s_fontBitmap, FONT_TEX_SIZE, FONT_TEX_SIZE,
        32, 96, s_fontChars);
    free(ttfData);
    if (result <= 0) {
        __android_log_print(ANDROID_LOG_ERROR, "ESDroid", "TouchUI: bake failed");
        delete[] s_fontBitmap; s_fontBitmap = nullptr;
        return;
    }

    glGenTextures(1, &s_fontTexture);
    glBindTexture(GL_TEXTURE_2D, s_fontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, FONT_TEX_SIZE, FONT_TEX_SIZE, 0,
                 GL_RED, GL_UNSIGNED_BYTE, s_fontBitmap);
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

// The app icon is drawn over the info cluster's logo box; the engine
// publishes that rect every frame via esdroid_set_logo_rect().
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

static void drawTouchText(const char* text, float x, float y, float screenW, float screenH) {
    if (!s_fontBitmap || s_fontTexture == 0 || s_textProgram == 0) return;

    float verts[4 * 6 * 64]; // pos.xy, uv.xy per vertex, 6 verts per quad, max 64 chars
    int nVerts = 0;
    float penX = x, penY = y + FONT_SIZE; // stbtt pen y is the baseline
    int len = strlen(text);

    for (int i = 0; i < len && i < 64; i++) {
        if (text[i] < 32 || text[i] >= 128) continue;
        stbtt_aligned_quad q;
        float qx = penX, qy = penY;
        stbtt_GetBakedQuad(s_fontChars, FONT_TEX_SIZE, FONT_TEX_SIZE,
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
    glUniform4f(s_textLocColor, 1.0f, 1.0f, 1.0f, 0.95f);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_fontTexture);
    glDrawArrays(GL_TRIANGLES, 0, nVerts / 4);
    glBindVertexArray(0);
    glUseProgram(0);
}

namespace esdroid {
TouchUI::TouchUI() {}
TouchUI::~TouchUI() {}
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
        float tw = strlen(label) * FONT_SIZE * 0.6f;
        float tx = b.x + (b.w - tw) / 2.0f;
        float ty = b.y + (b.h - FONT_SIZE) / 2.0f - 2.0f;
        drawTouchText(label, tx, ty, (float)m_screenW, (float)m_screenH);
    }
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

    const bool fn=AndroidBackend::instance().fnActive();
    for (const auto& b : AndroidBackend::instance().buttons())
        drawButton(b, b.key==VirtualKey::Fn ? fn : b.held);

    glDisable(GL_BLEND);
    if (prevCull) glEnable(GL_CULL_FACE);
    if (prevDepth) glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevFbo);
    glUseProgram(prevProgram);
}
} // namespace esdroid
