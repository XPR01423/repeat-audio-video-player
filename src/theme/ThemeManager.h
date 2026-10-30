#pragma once

#include "../settings/SettingsManager.h"
#include <QObject>

class ThemeManager final : public QObject
{
    Q_OBJECT
public:
    explicit ThemeManager(QObject *parent = nullptr);
    SettingsManager::Theme theme() const { return m_theme; }
    void setTheme(SettingsManager::Theme theme);
    void apply();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
signals:
    void themeChanged(SettingsManager::Theme theme);
private:
    SettingsManager::Theme m_theme = SettingsManager::Theme::System;
};
