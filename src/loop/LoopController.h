#pragma once

#include <QObject>

class MpvEngine;
class LoopController final : public QObject
{
    Q_OBJECT
public:
    explicit LoopController(MpvEngine *engine, QObject *parent = nullptr);
    bool repeatEnabled() const { return m_repeat; }
    bool hasA() const { return m_a >= 0.0; }
    bool hasB() const { return m_b >= 0.0; }
    double pointA() const { return m_a; }
    double pointB() const { return m_b; }
    void setRepeatEnabled(bool enabled);
    void setPointA(double position);
    bool setPointB(double position);
    void clearAbLoop();
signals:
    void changed();
    void errorOccurred(const QString &message);
private:
    MpvEngine *m_engine;
    bool m_repeat = false;
    double m_a = -1.0;
    double m_b = -1.0;
};
