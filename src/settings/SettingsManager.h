#pragma once

#include <QSettings>

class SettingsManager final
{
public:
    enum class Theme { System, Light, Dark };
    SettingsManager();
    Theme theme() const;
    void setTheme(Theme theme);
    int volume() const;
    bool muted() const;
    QByteArray geometry() const;
    QByteArray windowState() const;
    QStringList playlist() const;
    int currentRow() const;
    void saveVolume(int value);
    void saveMuted(bool value);
    void saveGeometry(const QByteArray &value);
    void saveWindowState(const QByteArray &value);
    void savePlaylist(const QStringList &files, int currentRow);
private:
    QSettings m_settings;
};
