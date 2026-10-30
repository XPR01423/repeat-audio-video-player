#pragma once

#include <QMainWindow>
#include "../settings/SettingsManager.h"

class MpvEngine;
class LoopController;
class PlaylistModel;
class ThemeManager;
class PlayerControls;
class VideoWidget;
class AudioArtworkWidget;
class QLabel;
class QListView;
class QDockWidget;
class QStackedWidget;
class QActionGroup;
class QFrame;
class QToolButton;

class MainWindow final : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void openFiles(const QStringList &files);
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
private:
    void buildUi();
    void buildMenus();
    void playRow(int row);
    void playNext();
    void playPrevious();
    void updateLoopUi();
    void applyTheme(SettingsManager::Theme theme);
    void openDialog(bool multiple);
    void showAbout();
    void setFullScreenMode(bool enabled);
    void updatePlaylistSummary();

    SettingsManager m_settings;
    MpvEngine *m_engine;
    LoopController *m_loop;
    PlaylistModel *m_playlist;
    ThemeManager *m_theme;
    PlayerControls *m_controls;
    VideoWidget *m_video;
    AudioArtworkWidget *m_audioView;
    QListView *m_listView;
    QStackedWidget *m_playlistStack;
    QLabel *m_playlistCount;
    QLabel *m_nowPlayingLabel;
    QDockWidget *m_playlistDock;
    QStackedWidget *m_viewStack;
    QFrame *m_headerBar;
    QToolButton *m_playlistButton;
    QActionGroup *m_themeActions;
    QString m_currentPath;
    bool m_fullScreen = false;
    bool m_wasMaximized = false;
};
