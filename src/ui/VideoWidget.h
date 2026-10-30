#pragma once

#include <QOpenGLWidget>

class MpvEngine;
class VideoWidget final : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit VideoWidget(MpvEngine *engine, QWidget *parent = nullptr);
    ~VideoWidget() override;
protected:
    void initializeGL() override;
    void paintGL() override;
private:
    MpvEngine *m_engine;
};
