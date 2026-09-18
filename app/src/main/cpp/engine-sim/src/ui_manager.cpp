#include "../include/ui_manager.h"

#include "../include/engine_sim_application.h"
#include "esdroid_render_log.h"

UiManager::UiManager() {
    m_app = nullptr;
    m_dragStart = nullptr;
    m_hover = nullptr;
    m_lastMouseScroll = 0;
}

UiManager::~UiManager() {
    /* void */
}

void UiManager::initialize(EngineSimApplication *app) {
    m_app = app;
    m_root.initialize(app);
}

void UiManager::destroy() {
    m_root.destroy();
    m_hover = nullptr;
    m_dragStart = nullptr;
    m_app = nullptr;
}

void UiManager::update(float dt) {
    ESLOG_ENTER();
    ESLOG("  [U]   root children=%zu", m_root.getChildCount());
    ESLOG("  [U]   calling m_root.update(dt)...");
    m_root.update(dt);
    ESLOG("  [U]   m_root.update OK");

    int mouse_x = 0, mouse_y = 0;
    ESLOG("  [U]   GetOsMousePos...");
    m_app->getEngine()->GetOsMousePos(&mouse_x, &mouse_y);
    ESLOG("  [U]   mouse=%d,%d", mouse_x, mouse_y);

    Point mousePos = { (float)mouse_x, (float)mouse_y };
    ESLOG("  [U]   mouseOver...");
    UiElement *newHover = m_root.mouseOver(mousePos);
    ESLOG("  [U]   newHover=%p oldHover=%p", (void*)newHover, (void*)m_hover);
    if (newHover != m_hover) {
        if (m_hover != nullptr) m_hover->onMouseLeave();
        if (newHover != nullptr) newHover->onMouseOver(mousePos);
        m_hover = newHover;
    }
    ESLOG("  [U]   mouse button check...");

    if (m_app->getEngine()->ProcessMouseButtonDown(ysMouse::Button::Left)) {
        m_dragStart = m_hover;
        m_mouse_p0 = mousePos;
        if (m_dragStart != nullptr) {
            m_drag_p0 = m_dragStart->getLocalPosition();
            m_dragStart->onMouseDown(m_dragStart->worldToLocal(mousePos));
        }
    }
    else if (m_app->getEngine()->ProcessMouseButtonUp(ysMouse::Button::Left)) {
        UiElement *dragRelease = m_hover;

        if (m_dragStart != nullptr) m_dragStart->onMouseUp(mousePos);

        if (dragRelease != nullptr && m_dragStart == dragRelease) {
            m_dragStart->onMouseClick(m_dragStart->worldToLocal(mousePos));
        }

        m_dragStart = nullptr;
    }
    ESLOG("  [U]   mouse scroll check...");

    const int newMouseScroll = m_app->getEngine()->GetMouseWheel();
    if (m_lastMouseScroll != newMouseScroll) {
        if (m_hover != nullptr) {
            m_hover->onMouseScroll(newMouseScroll - m_lastMouseScroll);
        }

        m_lastMouseScroll = newMouseScroll;
    }

    if (m_dragStart != nullptr) {
        m_dragStart->onDrag(m_drag_p0, m_mouse_p0, mousePos);
    }
    ESLOG_EXIT();
}

void UiManager::render() {
    ESLOG_ENTER();
    ESLOG("  [R]   root children=%zu", m_root.getChildCount());
    m_root.render();
    ESLOG_EXIT();
}
