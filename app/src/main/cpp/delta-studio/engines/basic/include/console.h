#ifndef DELTA_BASIC_CONSOLE_H
#define DELTA_BASIC_CONSOLE_H

#include "delta_core.h"

#include "font.h"
#include "shader_controls.h"
#include "ui_renderer.h"
#include "console_shaders.h"

#include <string>

namespace dbasic {

    // class name declarations
    class DeltaEngine;

    class Console : public ysObject {
    public:
        static const int BufferWidth = 175;
        static const int BufferHeight = 75;
        static const int BufferSize = BufferWidth * BufferHeight; // maximum of 4096 characters displayed at once

    public:
        Console();
        ~Console();

        ysError Initialize();
        ysError ResetScreenPosition();
        ysError Destroy();

        ysError UpdateGeometry();

        void SetDefaultFontDirectory(const std::wstring &s) { m_defaultFontDirectory = s; }
        void SetDefaultFontDirectory(const std::string &s) { m_defaultFontDirectory = std::wstring(s.begin(), s.end()); }

        void SetEngine(DeltaEngine *engine) { m_engine = engine; }
        DeltaEngine *GetEngine() const { return m_engine; }

        void SetRenderer(UiRenderer *renderer) { m_renderer = renderer; }
        UiRenderer *GetRenderer() const { return m_renderer; }

        Font *GetFont() const { return m_font; }

    protected:
        // settings
        std::wstring m_defaultFontDirectory;

    protected:
        // window metrics
        int m_bufferWidth;
        int m_bufferHeight;

    protected:
        //
        // gui drawing tools
        //
        // system
        //
        //

        void RealignLocation() { m_actualLocation = m_nominalLocation; }

        GuiPoint m_nominalLocation;
        GuiPoint m_actualLocation;

        DeltaEngine *m_engine;
        UiRenderer *m_renderer;

        Font *m_font;

        char *m_buffer;

    public:
        //
        // gui drawing tools
        //
        // interface
        //
        //

        // drawing text
        ysError SetCharacter(char character);

        void Clear();

        void OutputChar(unsigned char c, int n = 1);
        void DrawGeneralText(const char *text, int maxLength = -1);
        void DrawBoundText(const char *text, int width, int height, int xOffset, int yOffset);
        void DrawWrappedText(const char *text, int width);

        int GetTotalNotWhitespace() const;

        // drawing shapes
        void DrawLineRectangle(int width, int height);
        void DrawHorizontalLine(int length);
        void DrawVerticalLine(int length);

        // utilities
        static int FindEndOfNextWord(const char *text, int location);
        static inline bool IsWhiteSpace(char c) {
            if (c == '\t' || c == '\n' || c == ' ') { return true; }
            return false;
        }

        // navigation
        void MoveDownLine(int n = 1);
        void MoveToLocation(const GuiPoint &location);
        void MoveToOrigin();
    };

} /* namespace dbasic */

#endif /* DELTA_BASIC_CONSOLE_H  */
