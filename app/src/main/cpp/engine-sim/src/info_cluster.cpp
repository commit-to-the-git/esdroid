#include "../include/info_cluster.h"

#include "../include/engine_sim_application.h"
#include "esdroid_render_log.h"

#include <sstream>
#include <iomanip>

InfoCluster::InfoCluster() {
    m_engine = nullptr;
    m_logMessage = "Started";
}

InfoCluster::~InfoCluster() {
    /* void */
}

void InfoCluster::initialize(EngineSimApplication *app) {
    UiElement::initialize(app);
}

void InfoCluster::destroy() {
    UiElement::destroy();
}

void InfoCluster::update(float dt) {
    UiElement::update(dt);
}

void InfoCluster::render() {
    ESLOG_ENTER();
    ESLOG_PTR("m_engine", m_engine);
    ESLOG_PTR("m_app", m_app);
    ESLOG_STEP(1);

    Grid grid;
    grid.h_cells = 6;
    grid.v_cells = 4;
    ESLOG_STEP(2);

    const Bounds logoBounds = grid.get(m_bounds, 0, 0, 1, 2);
    ESLOG_STEP(3);
    drawFrame(logoBounds, 1.0f, m_app->getForegroundColor(), m_app->getBackgroundColor());
    ESLOG_STEP(4);

    dbasic::ModelAsset *logoAsset = m_app->getAssetManager()->GetModelAsset("Logo");
    ESLOG_PTR("logoAsset", logoAsset);
    ESLOG_STEP(5);
    drawModel(
        logoAsset,
        m_app->getForegroundColor(),
        logoBounds.getPosition(Bounds::center),
        Point(logoBounds.height(), logoBounds.height()) * 0.75f);
    ESLOG_STEP(6);

    const Bounds titleBounds = grid.get(m_bounds, 1, 0, 5, 2);
    drawFrame(titleBounds, 1.0f, m_app->getForegroundColor(), m_app->getBackgroundColor());
    ESLOG_STEP(7);

    Grid titleSplit;
    titleSplit.h_cells = 1;
    titleSplit.v_cells = 3;
    ESLOG_STEP(8);
    drawAlignedText(
        "ENGINE SIMULATOR",
        titleSplit.get(titleBounds, 0, 0).inset(10.0f).move({ 0.0f, -21.0f }),
        42.0f,
        Bounds::bl,
        Bounds::bl);
    ESLOG_STEP(9);
    drawAlignedText(
        "YOUTUBE/ANGETHEGREAT",
        titleSplit.get(titleBounds, 0, 1).inset(10.0f).move({ 0.0f, 5.0f }),
        24.0f,
        Bounds::tl,
        Bounds::tl);
    ESLOG_STEP(10);
    drawAlignedText(
        "BUILD: v" + EngineSimApplication::getBuildVersion() + " // " __DATE__,
        titleSplit.get(titleBounds, 0, 2).inset(10.0f).move({ 0.0f, 10.0f }),
        16.0f,
        Bounds::tl,
        Bounds::tl);
    ESLOG_STEP(11);

    const Bounds engineInfoBounds = grid.get(m_bounds, 0, 2, 6, 1);
    drawFrame(engineInfoBounds, 1.0f, m_app->getForegroundColor(), m_app->getBackgroundColor());
    ESLOG_STEP(12);

    drawAlignedText(
        (m_engine != nullptr) ? m_engine->getName() : "<NO ENGINE>",
        engineInfoBounds.inset(10.0f),
        24.0f,
        Bounds::lm,
        Bounds::lm);
    ESLOG_STEP(13);

    std::stringstream ss;
    if (m_engine != nullptr) {
        ESLOG_STEP(14);
        ss << std::fixed;

        if (m_engine->getDisplacement() < units::volume(1.0, units::L)) {
            ESLOG_STEP(15);
            ss << std::setprecision(0) << units::convert(m_engine->getDisplacement(), units::cc) << " cc -- ";
        }
        else {
            ESLOG_STEP(16);
            ss << std::setprecision(1) << units::convert(m_engine->getDisplacement(), units::L) << " L -- ";
        }

        ss << std::setprecision(0) << units::convert(m_engine->getDisplacement(), units::cubic_inches) << " CI";
    }
    else {
        ESLOG_STEP(17);
        ss << "N/A";
    }
    ESLOG_STEP(18);

    drawAlignedText(
        ss.str(),
        engineInfoBounds.inset(10.0f),
        24.0f,
        Bounds::rm,
        Bounds::rm);
    ESLOG_STEP(19);

    const Bounds infoMessagesBounds = grid.get(m_bounds, 0, 3, 6, 1);
    drawFrame(infoMessagesBounds, 1.0f, m_app->getForegroundColor(), m_app->getBackgroundColor());
    ESLOG_STEP(20);

    drawAlignedText(
        m_logMessage,
        infoMessagesBounds.inset(10.0f),
        24.0f,
        Bounds::lm,
        Bounds::lm);
    ESLOG_STEP(21);
    ESLOG_EXIT();
}
