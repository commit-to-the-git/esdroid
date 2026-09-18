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

#ifndef ESDROID_TOUCH_UI_H
#define ESDROID_TOUCH_UI_H
#include <GLES3/gl3.h>
#include "android_backend.h"
namespace esdroid {
class TouchUI {
public:
    TouchUI();
    ~TouchUI();
    void initialize(int sw,int sh);
    void resize(int sw,int sh);
    void render();
private:
    void compileShaders();
    void generateQuadGeometry();
    void drawButton(const TouchButton& b, bool held);
    void drawRect(float x,float y,float w,float h,const float* color);
    void drawSettingsButton();
    void drawSettingsPanel();
    void drawBlurredBackdrop();
    GLuint m_program=0, m_vao=0, m_vbo=0;
    GLint m_loc_color=-1, m_loc_rect=-1, m_loc_screen=-1, m_loc_pos=-1;
    int m_screenW=0, m_screenH=0;
};
} // namespace esdroid
#endif
