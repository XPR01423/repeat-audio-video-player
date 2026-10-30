#include "ThemeManager.h"

#include <QApplication>
#include <QEvent>
#include <QPalette>
#include <QStyle>

ThemeManager::ThemeManager(QObject *parent) : QObject(parent)
{
    qApp->installEventFilter(this);
}

void ThemeManager::setTheme(SettingsManager::Theme theme)
{
    if (m_theme == theme) return;
    m_theme = theme;
    apply();
    emit themeChanged(theme);
}

void ThemeManager::apply()
{
    QPalette palette;
    if (m_theme == SettingsManager::Theme::System) {
        palette = QApplication::style()->standardPalette();
    } else {
        const bool dark = m_theme == SettingsManager::Theme::Dark;
        palette.setColor(QPalette::Window, dark ? QColor("#11151b") : QColor("#f4f6f8"));
        palette.setColor(QPalette::WindowText, dark ? QColor("#e9eef4") : QColor("#202b38"));
        palette.setColor(QPalette::Base, dark ? QColor("#171c23") : QColor("#ffffff"));
        palette.setColor(QPalette::AlternateBase, dark ? QColor("#1d242d") : QColor("#f0f3f6"));
        palette.setColor(QPalette::Text, dark ? QColor("#e9eef4") : QColor("#202b38"));
        palette.setColor(QPalette::Button, dark ? QColor("#202731") : QColor("#edf1f4"));
        palette.setColor(QPalette::ButtonText, dark ? QColor("#e9eef4") : QColor("#202b38"));
        palette.setColor(QPalette::Highlight, dark ? QColor("#38c49c") : QColor("#178a6c"));
        palette.setColor(QPalette::HighlightedText, dark ? QColor("#071711") : Qt::white);
        palette.setColor(QPalette::PlaceholderText, dark ? QColor("#8995a3") : QColor("#798692"));
        palette.setColor(QPalette::Mid, dark ? QColor("#303944") : QColor("#dce2e8"));
    }
    if (qApp->palette() != palette) qApp->setPalette(palette);

    const bool dark = palette.color(QPalette::Window).lightness() < 128;
    const QString bg = palette.color(QPalette::Window).name();
    const QString raised = palette.color(QPalette::Button).name();
    const QString text = palette.color(QPalette::WindowText).name();
    const QString muted = dark ? QStringLiteral("#98a4b1") : QStringLiteral("#6d7a88");
    const QString border = dark ? QStringLiteral("#29323d") : QStringLiteral("#e1e6eb");
    const QString hover = dark ? QStringLiteral("#29323d") : QStringLiteral("#e7edf1");
    const QString accent = palette.color(QPalette::Highlight).name();
    const QString accentText = palette.color(QPalette::HighlightedText).name();

    qApp->setStyleSheet(QStringLiteral(R"QSS(
        QMainWindow, QDialog { background: %1; color: %3; }
        QWidget { color: %3; }
        QMenuBar { background: %1; color: %3; border: none; border-bottom: 1px solid %5; padding: 3px 10px; }
        QMenuBar::item { padding: 6px 10px; border-radius: 6px; background: transparent; }
        QMenuBar::item:selected, QMenuBar::item:pressed { background: %6; }
        QMenu { background: %2; color: %3; border: 1px solid %5; padding: 6px; border-radius: 9px; }
        QMenu::item { padding: 7px 28px 7px 12px; border-radius: 5px; }
        QMenu::item:selected { background: %7; color: %8; }
        QMenu::separator { height: 1px; background: %5; margin: 5px 8px; }
        QToolBar { background: %1; border: none; spacing: 8px; padding: 8px 14px; }
        QToolButton, QPushButton { background: %2; color: %3; border: 1px solid %5; border-radius: 9px; padding: 7px 12px; }
        QToolButton:hover, QPushButton:hover { background: %6; border-color: %7; }
        QToolButton:pressed, QPushButton:pressed { background: %7; }
        QToolButton:checked, QPushButton:checked { background: %7; color: %8; border-color: %7; }
        QToolButton#iconButton { background: transparent; border: 1px solid transparent; border-radius: 9px; padding: 6px; }
        QToolButton#iconButton:hover { background: %6; border-color: %5; }
        QToolButton#primaryButton { background: %7; color: %8; border: 1px solid %7; font-weight: 600; padding: 8px 15px; }
        QToolButton#primaryButton:hover { background: %7; border-color: %7; }
        QToolButton#playButton { background: %7; color: %8; border: none; border-radius: 17px; padding: 4px; }
        QToolButton#playButton:hover { background: %7; }
        QToolButton#loopButton { background: transparent; border: 1px solid transparent; padding: 5px 8px; }
        QToolButton#loopButton:checked { background: %7; color: %8; }
        QFrame#appHeader { background: %1; border: none; border-bottom: 1px solid %5; }
        QFrame#playlistTitleBar { background: %1; border: none; border-bottom: 1px solid %5; }
        QWidget#playerPanel { background: %2; border: none; border-top: 1px solid %5; }
        QFrame#playlistPanel { background: %1; border: none; }
        QToolButton#playlistToggle { background: transparent; border: 1px solid transparent; padding: 7px 9px; }
        QToolButton#playlistToggle:checked { background: %6; border-color: %5; }
        QDockWidget { color: %4; border: none; }
        QListView { background: %1; border: none; outline: 0; padding: 8px; }
        QListView::item { min-height: 40px; padding: 5px 10px; border-radius: 8px; border: 1px solid transparent; }
        QListView::item:hover { background: %6; }
        QListView::item:selected { background: %7; color: %8; }
        QSlider::groove:horizontal { height: 4px; background: %5; border-radius: 2px; }
        QSlider::sub-page:horizontal { background: %7; border-radius: 2px; }
        QSlider::handle:horizontal { width: 12px; height: 12px; margin: -5px 0; background: %7; border: 2px solid %2; border-radius: 7px; }
        QSlider::handle:horizontal:hover { width: 14px; height: 14px; margin: -6px -1px; }
        QLabel#eyebrow { color: %4; font-size: 9pt; font-weight: 700; letter-spacing: 1px; }
        QLabel#secondaryText, QLabel#timeLabel { color: %4; }
        QLabel#timeLabel { font-size: 9pt; font-weight: 600; }
        QStatusBar { background: %1; color: %4; border-top: 1px solid %5; }
        QToolTip { background: %2; color: %3; border: 1px solid %5; padding: 5px 8px; border-radius: 5px; }
        QScrollBar:vertical { background: transparent; width: 10px; margin: 4px 2px; }
        QScrollBar::handle:vertical { background: %5; min-height: 24px; border-radius: 5px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )QSS").arg(bg, raised, text, muted, border, hover, accent, accentText));
}

bool ThemeManager::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == qApp && event->type() == QEvent::ApplicationPaletteChange
        && m_theme == SettingsManager::Theme::System) {
        apply();
    }
    return QObject::eventFilter(watched, event);
}
