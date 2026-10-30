#include "PlayerControls.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QSlider>
#include <QStyle>
#include <QTime>
#include <QToolButton>
#include <QVBoxLayout>

TimelineSlider::TimelineSlider(QWidget *parent) : QSlider(Qt::Horizontal, parent)
{
    setRange(0, 1000);
    setMinimumHeight(22);
    setFocusPolicy(Qt::NoFocus);
}

void TimelineSlider::setMarkers(double a, double b, double duration)
{
    m_a = a; m_b = b; m_duration = duration; update();
}

void TimelineSlider::paintEvent(QPaintEvent *event)
{
    QSlider::paintEvent(event);
    if (m_duration <= 0.0) return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const auto marker = [&](double seconds, const QColor &color) {
        if (seconds < 0.0) return;
        const qreal x = 8.0 + qBound(0.0, seconds / m_duration, 1.0) * qMax(0, width() - 16);
        painter.setPen(Qt::NoPen); painter.setBrush(color);
        painter.drawRoundedRect(QRectF(x - 1.5, 1.5, 3, 8), 1.5, 1.5);
    };
    marker(m_a, QColor("#31c48d"));
    marker(m_b, QColor("#f0a35a"));
}

PlayerControls::PlayerControls(QWidget *parent) : QWidget(parent)
{
    setObjectName("playerPanel");
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 8, 18, 10);
    root->setSpacing(5);

    m_timeline = new TimelineSlider(this);
    root->addWidget(m_timeline);
    auto *row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(5);
    root->addLayout(row);

    const auto iconButton = [this, row](QStyle::StandardPixmap icon, const QString &tooltip,
                                        const QString &objectName, QSize size) {
        auto *button = new QToolButton(this);
        button->setObjectName(objectName);
        button->setIcon(style()->standardIcon(icon));
        button->setIconSize(QSize(17, 17));
        button->setToolTip(tooltip);
        button->setAutoRaise(true);
        button->setFixedSize(size);
        row->addWidget(button);
        return button;
    };

    m_previous = iconButton(QStyle::SP_MediaSkipBackward, tr("Previous file"), "iconButton", QSize(34, 34));
    m_play = iconButton(QStyle::SP_MediaPlay, tr("Play"), "playButton", QSize(38, 38));
    m_stop = iconButton(QStyle::SP_MediaStop, tr("Stop"), "iconButton", QSize(34, 34));
    m_next = iconButton(QStyle::SP_MediaSkipForward, tr("Next file"), "iconButton", QSize(34, 34));

    m_time = new QLabel("00:00 / 00:00", this);
    m_time->setObjectName("timeLabel");
    m_time->setAlignment(Qt::AlignCenter);
    m_time->setMinimumWidth(102);
    row->addWidget(m_time);
    row->addStretch(1);

    m_loopGroup = new QWidget(this);
    auto *loopRow = new QHBoxLayout(m_loopGroup);
    loopRow->setContentsMargins(0, 0, 0, 0);
    loopRow->setSpacing(2);
    const auto loopButton = [this, loopRow](const QString &label, const QString &tip) {
        auto *button = new QToolButton(m_loopGroup);
        button->setObjectName("loopButton");
        button->setText(label);
        button->setToolTip(tip);
        button->setAutoRaise(true);
        button->setMinimumSize(30, 32);
        loopRow->addWidget(button);
        return button;
    };
    auto *setA = loopButton(tr("A"), tr("Set loop start at the current position"));
    auto *setB = loopButton(tr("B"), tr("Set loop end at the current position"));
    auto *clearAb = loopButton(tr("Clear"), tr("Clear the A–B loop"));
    row->addWidget(m_loopGroup);

    m_repeat = iconButton(QStyle::SP_BrowserReload, tr("Repeat current file"), "iconButton", QSize(34, 34));
    m_repeat->setCheckable(true);
    m_mute = iconButton(QStyle::SP_MediaVolume, tr("Mute"), "iconButton", QSize(34, 34));
    m_mute->setCheckable(true);
    m_volume = new QSlider(Qt::Horizontal, this);
    m_volume->setRange(0, 100);
    m_volume->setValue(80);
    m_volume->setFocusPolicy(Qt::NoFocus);
    m_volume->setMinimumWidth(64);
    m_volume->setMaximumWidth(140);
    row->addWidget(m_volume);
    m_fullScreen = iconButton(QStyle::SP_TitleBarMaxButton, tr("Full screen"), "iconButton", QSize(34, 34));

    connect(m_previous, &QToolButton::clicked, this, &PlayerControls::previousRequested);
    connect(m_play, &QToolButton::clicked, this, &PlayerControls::playPauseRequested);
    connect(m_stop, &QToolButton::clicked, this, &PlayerControls::stopRequested);
    connect(m_next, &QToolButton::clicked, this, &PlayerControls::nextRequested);
    connect(setA, &QToolButton::clicked, this, &PlayerControls::setARequested);
    connect(setB, &QToolButton::clicked, this, &PlayerControls::setBRequested);
    connect(clearAb, &QToolButton::clicked, this, &PlayerControls::clearAbRequested);
    connect(m_repeat, &QToolButton::clicked, this, [this] { emit repeatRequested(m_repeat->isChecked()); });
    connect(m_mute, &QToolButton::toggled, this, [this](bool muted) { if (!m_updating) emit muteRequested(muted); });
    connect(m_volume, &QSlider::valueChanged, this, [this](int volume) { if (!m_updating) emit volumeRequested(volume); });
    connect(m_fullScreen, &QToolButton::clicked, this, &PlayerControls::fullScreenRequested);
    connect(m_timeline, &QSlider::valueChanged, this, [this](int value) {
        if (!m_updating && m_duration > 0.0) emit seekRequested(m_duration * value / 1000.0);
    });
}

QString PlayerControls::formatTime(double seconds) const
{
    const int value = qMax(0, qRound(seconds));
    if (value >= 3600)
        return QStringLiteral("%1:%2:%3").arg(value / 3600, 2, 10, QLatin1Char('0'))
            .arg((value / 60) % 60, 2, 10, QLatin1Char('0')).arg(value % 60, 2, 10, QLatin1Char('0'));
    return QStringLiteral("%1:%2").arg(value / 60, 2, 10, QLatin1Char('0'))
        .arg(value % 60, 2, 10, QLatin1Char('0'));
}

void PlayerControls::setPlaybackState(bool paused)
{
    m_play->setIcon(style()->standardIcon(paused ? QStyle::SP_MediaPlay : QStyle::SP_MediaPause));
    m_play->setToolTip(paused ? tr("Play") : tr("Pause"));
}

void PlayerControls::setPosition(double position, double duration)
{
    m_duration = duration;
    m_timeline->setMarkers(m_timeline->property("a").toDouble(), m_timeline->property("b").toDouble(), duration);
    m_time->setText(QStringLiteral("%1  /  %2").arg(formatTime(position), formatTime(duration)));
    if (!m_timeline->isSliderDown()) {
        m_updating = true;
        m_timeline->setValue(duration > 0.0 ? qRound(position / duration * 1000.0) : 0);
        m_updating = false;
    }
}

void PlayerControls::setVolume(int volume, bool muted)
{
    m_updating = true;
    m_volume->setValue(volume);
    m_mute->setChecked(muted);
    m_mute->setIcon(style()->standardIcon(muted ? QStyle::SP_MediaVolumeMuted : QStyle::SP_MediaVolume));
    m_mute->setToolTip(muted ? tr("Unmute") : tr("Mute"));
    m_updating = false;
}

void PlayerControls::setLoopState(bool repeat, double a, double b)
{
    m_repeat->setChecked(repeat);
    m_timeline->setProperty("a", a);
    m_timeline->setProperty("b", b);
    m_timeline->setMarkers(a, b, m_duration);
}

void PlayerControls::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    const int width = event->size().width();
    m_loopGroup->setVisible(width >= 690);
    m_stop->setVisible(width >= 480);
    m_time->setVisible(width >= 510);
    m_volume->setVisible(width >= 620);
    m_previous->setVisible(width >= 360);
    m_next->setVisible(width >= 360);
    m_volume->setMaximumWidth(width < 820 ? 82 : 140);
}
