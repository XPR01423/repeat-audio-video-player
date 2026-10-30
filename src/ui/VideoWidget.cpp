#include "VideoWidget.h"
#include "../engine/MpvEngine.h"
#include <QOpenGLFunctions>

VideoWidget::VideoWidget(MpvEngine *engine, QWidget *parent) : QOpenGLWidget(parent), m_engine(engine)
{
    setMinimumSize(320, 180);
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
    setAutoFillBackground(false);
    connect(engine, &MpvEngine::frameRequested, this, QOverload<>::of(&VideoWidget::update));
}
VideoWidget::~VideoWidget()
{
    if (m_engine) {
        if (context() && context()->isValid()) { makeCurrent(); m_engine->destroyRenderContext(); doneCurrent(); }
        else m_engine->destroyRenderContext();
    }
}
void VideoWidget::initializeGL()
{
    if (!m_engine->createRenderContext()) return;
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, [this] {
        makeCurrent();
        m_engine->destroyRenderContext();
        doneCurrent();
    });
}
void VideoWidget::paintGL()
{
    QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
    gl->glClearColor(0.025f, 0.03f, 0.04f, 1.0f);
    gl->glClear(GL_COLOR_BUFFER_BIT);
    m_engine->render(defaultFramebufferObject(), width() * devicePixelRatioF(), height() * devicePixelRatioF());
}
