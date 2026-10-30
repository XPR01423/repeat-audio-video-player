#include "SettingsManager.h"

SettingsManager::SettingsManager() = default;
SettingsManager::Theme SettingsManager::theme() const
{
    return static_cast<Theme>(m_settings.value("appearance/theme", int(Theme::System)).toInt());
}
void SettingsManager::setTheme(Theme theme) { m_settings.setValue("appearance/theme", int(theme)); }
int SettingsManager::volume() const { return m_settings.value("playback/volume", 80).toInt(); }
bool SettingsManager::muted() const { return m_settings.value("playback/muted", false).toBool(); }
QByteArray SettingsManager::geometry() const { return m_settings.value("window/geometry").toByteArray(); }
QByteArray SettingsManager::windowState() const { return m_settings.value("window/state").toByteArray(); }
QStringList SettingsManager::playlist() const { return m_settings.value("playlist/files").toStringList(); }
int SettingsManager::currentRow() const { return m_settings.value("playlist/current", -1).toInt(); }
void SettingsManager::saveVolume(int value) { m_settings.setValue("playback/volume", value); }
void SettingsManager::saveMuted(bool value) { m_settings.setValue("playback/muted", value); }
void SettingsManager::saveGeometry(const QByteArray &value) { m_settings.setValue("window/geometry", value); }
void SettingsManager::saveWindowState(const QByteArray &value) { m_settings.setValue("window/state", value); }
void SettingsManager::savePlaylist(const QStringList &files, int currentRow)
{
    m_settings.setValue("playlist/files", files);
    m_settings.setValue("playlist/current", currentRow);
}
