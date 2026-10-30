#include "LoopController.h"
#include "../engine/MpvEngine.h"

LoopController::LoopController(MpvEngine *engine, QObject *parent) : QObject(parent), m_engine(engine) {}
void LoopController::setRepeatEnabled(bool enabled)
{
    m_repeat = enabled;
    if (enabled) clearAbLoop();
    m_engine->setFileLoop(enabled);
    emit changed();
}
void LoopController::setPointA(double position)
{
    m_repeat = false;
    m_engine->setFileLoop(false);
    m_a = qMax(0.0, position);
    m_b = -1.0;
    m_engine->clearAbLoop();
    emit changed();
}
bool LoopController::setPointB(double position)
{
    if (m_a < 0.0) {
        emit errorOccurred(tr("Set point A before setting point B."));
        return false;
    }
    if (position <= m_a) {
        emit errorOccurred(tr("Point B must be later than point A."));
        return false;
    }
    m_b = position;
    m_engine->setAbLoop(m_a, m_b);
    emit changed();
    return true;
}
void LoopController::clearAbLoop()
{
    m_a = -1.0;
    m_b = -1.0;
    m_engine->clearAbLoop();
    emit changed();
}
