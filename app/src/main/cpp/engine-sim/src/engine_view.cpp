#include "../include/engine_view.h"

#include "../include/engine_sim_application.h"
#include "esdroid_render_log.h"

EngineView::EngineView() {
    m_pan = { 0, units::distance(-6, units::inch) };
    m_checkMouse = true;
    m_lastScroll = 0;
    m_zoom = 1.0f;
    m_drawFrame = true;
}

EngineView::~EngineView() {
    /* void */
}

void EngineView::update(float dt) {
    m_mouseBounds = m_bounds;
}

void EngineView::render() {
    ESLOG_ENTER();
    ESLOG("  [R] m_drawFrame=%d", m_drawFrame ? 1 : 0);
    if (m_drawFrame) {
        ESLOG_STEP(1);
        drawFrame(m_bounds, 1.0f, m_app->getForegroundColor(), m_app->getBackgroundColor(), false);
        ESLOG_STEP(2);
    }
    ESLOG_EXIT();
}

void EngineView::onMouseDown(const Point &mouseLocal) {
    UiElement::onMouseDown(mouseLocal);
    m_dragStart = m_pan;
}

void EngineView::onDrag(const Point &p0, const Point &mouse0, const Point &mouse) {
    const Point delta = mouse - mouse0;
    const Point deltaUnits = {
        m_app->pixelsToUnits(delta.x),
        m_app->pixelsToUnits(delta.y)
    };

    m_pan = m_dragStart + deltaUnits;
}

void EngineView::onMouseScroll(int scroll) {
    const float f = std::powf(2.0, (float)scroll / 500.0f);

    const Point prevCenter = getCenter();

    m_zoom *= f;
    const Point newCenter = getCenter();

    Point diff = newCenter - prevCenter;
    m_pan += diff * m_zoom;
    m_dragStart += diff * m_zoom;
}

void EngineView::onPinchZoom(int scroll, const Point &mouseLocal) {
    onMouseScroll(scroll);
    // onmousescroll keeps the screen center fixed shift the pan so the
    // point under the fingers stays under the fingers instead
    const float f = std::powf(2.0, (float)scroll / 500.0f);
    const Point anchor = {
        mouseLocal.x - (m_bounds.m0.x + m_bounds.m1.x) * 0.5f,
        mouseLocal.y - (m_bounds.m0.y + m_bounds.m1.y) * 0.5f
    };
    const Point shift = {
        -(f - 1.0f) * m_app->pixelsToUnits(anchor.x),
        -(f - 1.0f) * m_app->pixelsToUnits(anchor.y)
    };
    m_pan += shift;
    m_dragStart += shift;
}

void EngineView::setBounds(const Bounds &bounds) {
    m_bounds = bounds;
}

Point EngineView::getCenter() const {
    return getCameraPosition();
}

Point EngineView::getCameraPosition() const {
    return Point(-m_pan / m_zoom);
}
