#include "MpvEngine.h"

#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QOpenGLContext>
#include <QtMath>
#include <QTimer>

#include <clocale>
#include <mpv/render_gl.h>

namespace {
void *getGlProcAddress(void *, const char *name)
{
    auto *context = QOpenGLContext::currentContext();
    return context ? reinterpret_cast<void *>(context->getProcAddress(QByteArray(name))) : nullptr;
}
}

MpvEngine::MpvEngine(QObject *parent) : QObject(parent)
{
    if (!std::setlocale(LC_NUMERIC, "C")) {
        failInitialization(tr("Could not set LC_NUMERIC=C before MPV initialization."));
        return;
    }
    m_handle = mpv_create();
    if (!m_handle) {
        failInitialization(tr("mpv_create() returned a null handle. Check that libmpv is installed and loadable."));
        return;
    }

    const int logResult = mpv_request_log_messages(m_handle, "debug");
    if (logResult < 0) {
        failInitialization(tr("mpv_request_log_messages() failed: %1 (%2)")
                               .arg(QString::fromUtf8(mpv_error_string(logResult)))
                               .arg(logResult));
        mpv_terminate_destroy(m_handle);
        m_handle = nullptr;
        return;
    }

    const auto setOption = [this](const char *name, const char *value) {
        const int result = mpv_set_option_string(m_handle, name, value);
        if (result < 0) {
            failInitialization(tr("MPV option '%1=%2' was rejected: %3 (%4)")
                                   .arg(QString::fromUtf8(name), QString::fromUtf8(value),
                                        QString::fromUtf8(mpv_error_string(result)))
                                   .arg(result));
            drainStartupEvents();
            return false;
        }
        qInfo().noquote() << "[RAV Player] MPV option" << name << "=" << value;
        return true;
    };
    const QList<QPair<const char *, const char *>> options = {
        {"config", "no"}, {"terminal", "no"},
        {"input-default-bindings", "no"}, {"input-vo-keyboard", "no"},
        {"vo", "libmpv"}, {"keep-open", "yes"}, {"pause", "no"},
        {"video-timing-offset", "0"}, {"cover-art-auto", "no"}
    };
    for (const auto &[name, value] : options) {
        if (!setOption(name, value)) {
            mpv_terminate_destroy(m_handle);
            m_handle = nullptr;
            return;
        }
    }
    const QByteArray audioOutput = qEnvironmentVariable("RAVPLAYER_AUDIO_OUTPUT").trimmed().toUtf8();
    if (!audioOutput.isEmpty() && !setOption("ao", audioOutput.constData())) {
        mpv_terminate_destroy(m_handle);
        m_handle = nullptr;
        return;
    }

    const int initResult = mpv_initialize(m_handle);
    if (initResult < 0) {
        failInitialization(tr("mpv_initialize() failed: %1 (%2)")
                               .arg(QString::fromUtf8(mpv_error_string(initResult)))
                               .arg(initResult));
        drainStartupEvents();
        mpv_terminate_destroy(m_handle);
        m_handle = nullptr;
        return;
    }
    m_initialized = true;
    qInfo().noquote() << "[RAV Player] libmpv initialized successfully; client API"
                      << QStringLiteral("%1.%2").arg(mpv_client_api_version() >> 16)
                             .arg(mpv_client_api_version() & 0xffff);

    mpv_observe_property(m_handle, 1, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_handle, 2, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_handle, 3, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_handle, 4, "volume", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_handle, 5, "mute", MPV_FORMAT_FLAG);
    mpv_observe_property(m_handle, 6, "media-title", MPV_FORMAT_STRING);
    mpv_observe_property(m_handle, 7, "track-list", MPV_FORMAT_NODE);
    mpv_observe_property(m_handle, 8, "metadata", MPV_FORMAT_NODE);

    auto *timer = new QTimer(this);
    timer->setInterval(25);
    connect(timer, &QTimer::timeout, this, &MpvEngine::pollEvents);
    timer->start();
}

void MpvEngine::failInitialization(const QString &message)
{
    m_initializationError = message;
    qCritical().noquote() << "[RAV Player] libmpv initialization error:" << message;
}

void MpvEngine::handleLogMessage(const mpv_event_log_message *message)
{
    if (!message) return;
    const QString level = QString::fromUtf8(message->level ? message->level : "?");
    const QString prefix = QString::fromUtf8(message->prefix ? message->prefix : "mpv");
    const QString text = QString::fromUtf8(message->text ? message->text : "").trimmed();
    const QString line = QStringLiteral("[%1] %2: %3").arg(level, prefix, text);
    m_recentLogs.append(line);
    if (m_recentLogs.size() > 160) m_recentLogs.removeFirst();
    if (level == "fatal" || level == "error") qCritical().noquote() << line;
    else if (level == "warn") qWarning().noquote() << line;
    else qInfo().noquote() << line;
}

void MpvEngine::drainStartupEvents()
{
    if (!m_handle) return;
    while (mpv_event *event = mpv_wait_event(m_handle, 0)) {
        if (event->event_id == MPV_EVENT_NONE) break;
        if (event->event_id == MPV_EVENT_LOG_MESSAGE)
            handleLogMessage(static_cast<const mpv_event_log_message *>(event->data));
    }
    if (!m_recentLogs.isEmpty()) {
        m_initializationError += tr("\n\nMPV diagnostic log:\n%1")
                                     .arg(m_recentLogs.mid(qMax(0, m_recentLogs.size() - 12))
                                              .join(QLatin1Char('\n')));
    }
}

MpvEngine::~MpvEngine()
{
    destroyRenderContext();
    if (m_handle) {
        mpv_terminate_destroy(m_handle);
        m_handle = nullptr;
    }
}

void MpvEngine::destroyRenderContext()
{
    if (!m_renderContext) return;
    mpv_render_context_set_update_callback(m_renderContext, nullptr, nullptr);
    mpv_render_context_free(m_renderContext);
    m_renderContext = nullptr;
}

bool MpvEngine::createRenderContext()
{
    if (!m_handle || m_renderContext)
        return m_renderContext != nullptr;
    const char *apiType = MPV_RENDER_API_TYPE_OPENGL;
    mpv_opengl_init_params glInit{getGlProcAddress, nullptr};
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(apiType)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    const int result = mpv_render_context_create(&m_renderContext, m_handle, params);
    if (result < 0) {
        emit errorOccurred(tr("Video output could not be initialized: %1")
                               .arg(QString::fromUtf8(mpv_error_string(result))));
        return false;
    }
    mpv_render_context_set_update_callback(m_renderContext, &MpvEngine::updateCallback, this);
    return true;
}

void MpvEngine::updateCallback(void *ctx)
{
    auto *self = static_cast<MpvEngine *>(ctx);
    QMetaObject::invokeMethod(self, &MpvEngine::frameRequested, Qt::QueuedConnection);
}

void MpvEngine::render(quintptr framebuffer, int width, int height)
{
    if (!m_renderContext || width <= 0 || height <= 0)
        return;
    mpv_opengl_fbo fbo{static_cast<int>(framebuffer), width, height, 0};
    int flipY = 1;
    int blockForTargetTime = 0;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flipY},
        {MPV_RENDER_PARAM_BLOCK_FOR_TARGET_TIME, &blockForTargetTime},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };
    mpv_render_context_render(m_renderContext, params);
}

void MpvEngine::loadFile(const QString &path)
{
    if (!m_handle) return;
    const QByteArray encoded = QFile::encodeName(path);
    const char *args[] = {"loadfile", encoded.constData(), "replace", nullptr};
    const int result = mpv_command_async(m_handle, 0, args);
    if (result < 0)
        emit errorOccurred(tr("Could not open file: %1").arg(QString::fromUtf8(mpv_error_string(result))));
}

void MpvEngine::togglePause() { setPaused(!m_paused); }
void MpvEngine::setPaused(bool paused)
{
    if (m_handle) {
        int value = paused ? 1 : 0;
        mpv_set_property(m_handle, "pause", MPV_FORMAT_FLAG, &value);
    }
}
void MpvEngine::stop()
{
    if (!m_handle) return;
    const char *args[] = {"stop", nullptr};
    mpv_command_async(m_handle, 0, args);
}
void MpvEngine::seek(double seconds) { setNumericProperty("time-pos", qMax(0.0, seconds)); }
void MpvEngine::seekRelative(double seconds)
{
    if (!m_handle) return;
    const QByteArray value = QByteArray::number(seconds, 'f', 2);
    const char *args[] = {"seek", value.constData(), "relative", nullptr};
    mpv_command_async(m_handle, 0, args);
}
void MpvEngine::setVolume(int volume)
{
    if (m_handle) { double value = qBound(0, volume, 100); mpv_set_property(m_handle, "volume", MPV_FORMAT_DOUBLE, &value); }
}
void MpvEngine::setMuted(bool muted)
{
    if (m_handle) {
        int value = muted ? 1 : 0;
        mpv_set_property(m_handle, "mute", MPV_FORMAT_FLAG, &value);
    }
}
void MpvEngine::setFileLoop(bool enabled)
{
    if (m_handle) mpv_set_property_string(m_handle, "loop-file", enabled ? "inf" : "no");
}
void MpvEngine::setAbLoop(double a, double b)
{
    if (!m_handle) return;
    setNumericProperty("ab-loop-a", a);
    setNumericProperty("ab-loop-b", b);
}
void MpvEngine::clearAbLoop()
{
    if (!m_handle) return;
    mpv_set_property_string(m_handle, "ab-loop-a", "no");
    mpv_set_property_string(m_handle, "ab-loop-b", "no");
}
void MpvEngine::setNumericProperty(const char *name, double value)
{
    if (m_handle) mpv_set_property(m_handle, name, MPV_FORMAT_DOUBLE, &value);
}

void MpvEngine::pollEvents()
{
    if (!m_handle) return;
    while (true) {
        mpv_event *event = mpv_wait_event(m_handle, 0);
        if (!event || event->event_id == MPV_EVENT_NONE) break;
        if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            auto *property = static_cast<mpv_event_property *>(event->data);
            if (!property) continue;
            if (property->name && qstrcmp(property->name, "time-pos") == 0 && property->data)
                m_position = *static_cast<double *>(property->data);
            else if (property->name && qstrcmp(property->name, "duration") == 0)
                m_duration = property->data ? *static_cast<double *>(property->data) : 0.0;
            else if (property->name && qstrcmp(property->name, "pause") == 0 && property->data)
                m_paused = *static_cast<int *>(property->data) != 0;
            else if (property->name && qstrcmp(property->name, "volume") == 0 && property->data)
                m_volume = qBound(0, qRound(*static_cast<double *>(property->data)), 100);
            else if (property->name && qstrcmp(property->name, "mute") == 0 && property->data)
                m_muted = *static_cast<int *>(property->data) != 0;
            else if (property->name && qstrcmp(property->name, "media-title") == 0) {
                const char *title = property->data ? *static_cast<char **>(property->data) : nullptr;
                m_title = title ? QString::fromUtf8(title) : QString();
            } else if (property->name && qstrcmp(property->name, "metadata") == 0) {
                m_artist.clear();
                m_album.clear();
                const auto *metadata = static_cast<const mpv_node *>(property->data);
                if (metadata && metadata->format == MPV_FORMAT_NODE_MAP && metadata->u.list) {
                    const mpv_node_list *fields = metadata->u.list;
                    for (int i = 0; i < fields->num; ++i) {
                        if (!fields->keys[i] || fields->values[i].format != MPV_FORMAT_STRING
                            || !fields->values[i].u.string) continue;
                        const QString key = QString::fromUtf8(fields->keys[i]).toLower();
                        const QString value = QString::fromUtf8(fields->values[i].u.string);
                        if (key == "artist" || key == "album_artist") m_artist = value;
                        else if (key == "album") m_album = value;
                    }
                }
            } else if (property->name && qstrcmp(property->name, "track-list") == 0 && property->data) {
                const auto *tracks = static_cast<const mpv_node *>(property->data);
                bool hasVideo = false;
                if (tracks->format == MPV_FORMAT_NODE_ARRAY && tracks->u.list) {
                    const mpv_node_list *items = tracks->u.list;
                    for (int i = 0; i < items->num && !hasVideo; ++i) {
                        const mpv_node &track = items->values[i];
                        if (track.format != MPV_FORMAT_NODE_MAP || !track.u.list) continue;
                        const mpv_node_list *fields = track.u.list;
                        bool videoType = false, selected = false;
                        for (int j = 0; j < fields->num; ++j) {
                            if (qstrcmp(fields->keys[j], "type") == 0
                                && fields->values[j].format == MPV_FORMAT_STRING)
                                videoType = qstrcmp(fields->values[j].u.string, "video") == 0;
                            else if (qstrcmp(fields->keys[j], "selected") == 0
                                     && fields->values[j].format == MPV_FORMAT_FLAG)
                                selected = fields->values[j].u.flag != 0;
                        }
                        hasVideo = videoType && selected;
                    }
                }
                m_hasVideo = hasVideo;
            }
            emit positionChanged(m_position, m_duration);
            emit playbackStateChanged(m_paused);
            emit volumeChanged(m_volume, m_muted);
            emit mediaChanged(m_title, m_hasVideo);
            emit mediaDetailsChanged(m_title, m_artist, m_album);
        } else if (event->event_id == MPV_EVENT_START_FILE) {
            m_position = 0.0;
            m_duration = 0.0;
            m_title.clear();
            m_artist.clear();
            m_album.clear();
            m_hasVideo = false;
        } else if (event->event_id == MPV_EVENT_FILE_LOADED) {
            double loadedDuration = 0.0;
            if (mpv_get_property(m_handle, "duration", MPV_FORMAT_DOUBLE, &loadedDuration) >= 0)
                m_duration = loadedDuration;
            qInfo().noquote() << "[RAV Player] MPV opened media:" << m_title
                              << "; duration=" << m_duration << "seconds; video=" << m_hasVideo;
            emit mediaLoaded();
        } else if (event->event_id == MPV_EVENT_END_FILE) {
            auto *end = static_cast<mpv_event_end_file *>(event->data);
            if (end && end->reason == MPV_END_FILE_REASON_EOF) emit endOfFile();
            else if (end && end->reason == MPV_END_FILE_REASON_ERROR)
                emit errorOccurred(tr("Playback failed: %1 (%2)")
                                       .arg(QString::fromUtf8(mpv_error_string(end->error)))
                                       .arg(end->error));
        } else if (event->event_id == MPV_EVENT_LOG_MESSAGE) {
            handleLogMessage(static_cast<const mpv_event_log_message *>(event->data));
        }
    }
}
