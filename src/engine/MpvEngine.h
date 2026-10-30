#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include <mpv/client.h>
#include <mpv/render.h>

class MpvEngine final : public QObject
{
    Q_OBJECT
public:
    explicit MpvEngine(QObject *parent = nullptr);
    ~MpvEngine() override;

    bool isReady() const { return m_handle != nullptr; }
    QString initializationError() const { return m_initializationError; }
    QStringList recentLogs() const { return m_recentLogs; }
    bool createRenderContext();
    void destroyRenderContext();
    void render(quintptr framebuffer, int width, int height);
    void loadFile(const QString &path);
    void togglePause();
    void setPaused(bool paused);
    void stop();
    void seek(double seconds);
    void seekRelative(double seconds);
    void setVolume(int volume);
    void setMuted(bool muted);
    void setFileLoop(bool enabled);
    void setAbLoop(double a, double b);
    void clearAbLoop();

    double position() const { return m_position; }
    double duration() const { return m_duration; }
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    bool paused() const { return m_paused; }
    bool hasVideo() const { return m_hasVideo; }
    QString mediaTitle() const { return m_title; }

signals:
    void frameRequested();
    void positionChanged(double position, double duration);
    void playbackStateChanged(bool paused);
    void volumeChanged(int volume, bool muted);
    void mediaChanged(const QString &title, bool hasVideo);
    void mediaDetailsChanged(const QString &title, const QString &artist, const QString &album);
    void mediaLoaded();
    void endOfFile();
    void errorOccurred(const QString &message);

private:
    static void updateCallback(void *ctx);
    void pollEvents();
    void refreshProperties();
    void setNumericProperty(const char *name, double value);
    void handleLogMessage(const mpv_event_log_message *message);
    void drainStartupEvents();
    void failInitialization(const QString &message);

    mpv_handle *m_handle = nullptr;
    mpv_render_context *m_renderContext = nullptr;
    double m_position = 0.0;
    double m_duration = 0.0;
    int m_volume = 100;
    bool m_muted = false;
    bool m_paused = true;
    bool m_hasVideo = false;
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_initializationError;
    QStringList m_recentLogs;
    bool m_initialized = false;
};
