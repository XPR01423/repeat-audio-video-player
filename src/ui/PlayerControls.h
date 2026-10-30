#pragma once

#include <QWidget>
#include <QSlider>

class QLabel;
class QToolButton;
class QResizeEvent;
class TimelineSlider final : public QSlider
{
public:
    explicit TimelineSlider(QWidget *parent = nullptr);
    void setMarkers(double a, double b, double duration);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    double m_a = -1, m_b = -1, m_duration = 0;
};

class PlayerControls final : public QWidget
{
    Q_OBJECT
public:
    explicit PlayerControls(QWidget *parent = nullptr);
    void setPlaybackState(bool paused);
    void setPosition(double position, double duration);
    void setVolume(int volume, bool muted);
    void setLoopState(bool repeat, double a, double b);
protected:
    void resizeEvent(QResizeEvent *event) override;
signals:
    void playPauseRequested();
    void stopRequested();
    void previousRequested();
    void nextRequested();
    void seekRequested(double position);
    void volumeRequested(int volume);
    void muteRequested(bool muted);
    void repeatRequested(bool enabled);
    void setARequested();
    void setBRequested();
    void clearAbRequested();
    void fullScreenRequested();
private:
    QString formatTime(double seconds) const;
    QToolButton *m_play, *m_mute, *m_repeat;
    QToolButton *m_previous, *m_next, *m_stop, *m_fullScreen;
    QWidget *m_loopGroup;
    QSlider *m_volume;
    QLabel *m_time;
    TimelineSlider *m_timeline;
    double m_duration = 0;
    bool m_updating = false;
};
