#pragma once

#include <QImage>
#include <QWidget>

class AudioArtworkWidget final : public QWidget
{
    Q_OBJECT
public:
    explicit AudioArtworkWidget(QWidget *parent = nullptr);
    void setMedia(const QString &path, const QString &title, const QString &artist,
                  const QString &album, bool paused);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    QImage findArtwork(const QString &mediaPath) const;

    QString m_path;
    QString m_title;
    QString m_artist;
    QString m_album;
    QImage m_artwork;
    bool m_paused = true;
};
