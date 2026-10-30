#include "AudioArtworkWidget.h"

#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>

AudioArtworkWidget::AudioArtworkWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("audioArtwork");
    setMinimumSize(260, 200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void AudioArtworkWidget::setMedia(const QString &path, const QString &title, const QString &artist,
                                  const QString &album, bool paused)
{
    const bool pathChanged = m_path != path;
    m_path = path;
    m_title = title.isEmpty() ? QFileInfo(path).completeBaseName() : title;
    m_artist = artist;
    m_album = album;
    m_paused = paused;
    if (pathChanged) m_artwork = findArtwork(path);
    update();
}

QImage AudioArtworkWidget::findArtwork(const QString &mediaPath) const
{
    if (mediaPath.isEmpty()) return {};
    const QDir directory = QFileInfo(mediaPath).absoluteDir();
    const QStringList preferredNames = {
        "cover", "front", "folder", "albumart", "album", "thumb", ".folder"
    };
    const QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.webp", "*.bmp"};
    const QFileInfoList candidates = directory.entryInfoList(filters, QDir::Files, QDir::Name);
    for (const QString &preferred : preferredNames) {
        for (const QFileInfo &candidate : candidates) {
            if (candidate.completeBaseName().compare(preferred, Qt::CaseInsensitive) != 0) continue;
            QImageReader reader(candidate.absoluteFilePath());
            reader.setAutoTransform(true);
            reader.setScaledSize(QSize(720, 720));
            const QImage image = reader.read();
            if (!image.isNull()) return image;
        }
    }
    return {};
}

void AudioArtworkWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    const QRectF canvas = rect();
    const QColor base = palette().color(QPalette::Window);
    QLinearGradient background(canvas.topLeft(), canvas.bottomRight());
    background.setColorAt(0.0, base.lighter(112));
    background.setColorAt(1.0, base.darker(108));
    painter.fillRect(canvas, background);

    const QColor accent = palette().color(QPalette::Highlight);
    QRadialGradient glow(QPointF(width() * 0.32, height() * 0.42), qMax(width(), height()) * 0.62);
    QColor glowColor = accent; glowColor.setAlpha(34);
    glow.setColorAt(0.0, glowColor); glow.setColorAt(1.0, Qt::transparent);
    painter.fillRect(canvas, glow);

    const bool sideBySide = width() >= 650;
    const qreal imageLimit = sideBySide ? qMin(height() * 0.64, width() * 0.34)
                                        : qMin(height() * 0.39, width() * 0.58);
    const qreal artSize = qBound<qreal>(112, imageLimit, 286);
    const QRectF artRect = sideBySide
        ? QRectF(qMax<qreal>(28, width() * 0.08), (height() - artSize) / 2.0 - 15, artSize, artSize)
        : QRectF((width() - artSize) / 2.0, qMax<qreal>(22, height() * 0.08), artSize, artSize);

    painter.save();
    QPainterPath artClip; artClip.addRoundedRect(artRect, 18, 18); painter.setClipPath(artClip);
    if (!m_artwork.isNull()) {
        painter.drawImage(artRect, m_artwork);
    } else {
        QLinearGradient tile(artRect.topLeft(), artRect.bottomRight());
        tile.setColorAt(0.0, accent.lighter(135)); tile.setColorAt(1.0, accent.darker(145));
        painter.fillRect(artRect, tile);
        painter.setPen(QPen(QColor(255, 255, 255, 36), 1));
        painter.drawEllipse(artRect.adjusted(artSize * 0.12, artSize * 0.12, -artSize * 0.12, -artSize * 0.12));
        painter.drawEllipse(artRect.adjusted(artSize * 0.22, artSize * 0.22, -artSize * 0.22, -artSize * 0.22));
        const QImage logo(QStringLiteral(":/icons/icon.png"));
        const QSize logoSize = QSize(artSize * 0.48, artSize * 0.5);
        const QRectF logoRect(QPointF(artRect.center().x() - logoSize.width() / 2.0,
                                      artRect.center().y() - logoSize.height() / 2.0), logoSize);
        painter.drawImage(logoRect, logo);
    }
    painter.restore();
    painter.setPen(QPen(QColor(255, 255, 255, 38), 1));
    painter.drawRoundedRect(artRect, 18, 18);

    const qreal textX = sideBySide ? artRect.right() + 42 : 28;
    const qreal textWidth = sideBySide ? qMax<qreal>(180, width() - textX - 32) : width() - 56;
    const qreal blockHeight = 156;
    const qreal textY = sideBySide ? (height() - blockHeight) / 2.0 + 8 : artRect.bottom() + 34;
    const QRectF textRect(textX, textY, textWidth, qMax<qreal>(72, height() - textY - 30));

    QColor muted = palette().color(QPalette::WindowText); muted.setAlpha(175);
    painter.setPen(accent);
    QFont eyebrow = font(); eyebrow.setPointSizeF(qMax(8.0, font().pointSizeF() - 1)); eyebrow.setWeight(QFont::DemiBold);
    painter.setFont(eyebrow);
    const QString status = m_path.isEmpty() ? tr("READY WHEN YOU ARE") : (m_paused ? tr("PAUSED") : tr("NOW PLAYING"));
    painter.drawText(QRectF(textRect.left(), textRect.top(), textRect.width(), 22), Qt::AlignLeft | Qt::AlignVCenter, status);

    painter.setPen(palette().color(QPalette::WindowText));
    QFont titleFont = font(); titleFont.setPointSizeF(qBound(17.0, font().pointSizeF() * 1.65, 29.0)); titleFont.setWeight(QFont::DemiBold);
    painter.setFont(titleFont);
    const QString title = m_title.isEmpty() ? tr("Your next favorite track") : m_title;
    painter.drawText(QRectF(textRect.left(), textRect.top() + 31, textRect.width(), 45),
                     Qt::AlignLeft | Qt::AlignVCenter, QFontMetrics(titleFont).elidedText(title, Qt::ElideRight, qRound(textRect.width())));

    painter.setPen(muted);
    QFont detailFont = font(); detailFont.setPointSizeF(qMax(10.0, font().pointSizeF()));
    painter.setFont(detailFont);
    QString details = m_artist;
    if (!m_album.isEmpty()) details += (details.isEmpty() ? QString() : QStringLiteral("  ·  ")) + m_album;
    if (details.isEmpty() && !m_path.isEmpty()) details = QFileInfo(m_path).fileName();
    painter.drawText(QRectF(textRect.left(), textRect.top() + 80, textRect.width(), 27),
                     Qt::AlignLeft | Qt::AlignVCenter, QFontMetrics(detailFont).elidedText(details, Qt::ElideRight, qRound(textRect.width())));

    painter.setPen(QPen(muted, 1));
    const qreal lineY = textRect.top() + 121;
    painter.drawLine(QPointF(textRect.left(), lineY), QPointF(textRect.left() + qMin<qreal>(textRect.width(), 240), lineY));
    painter.setPen(muted);
    QFont hintFont = font(); hintFont.setPointSizeF(qMax(8.0, font().pointSizeF() - 1));
    painter.setFont(hintFont);
    painter.drawText(QRectF(textRect.left(), lineY + 10, textRect.width(), 24), Qt::AlignLeft | Qt::AlignVCenter,
                     m_path.isEmpty() ? tr("Open a file or drop media here") : tr("Audio playback by libmpv"));
}
