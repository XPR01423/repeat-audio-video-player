#include "MainWindow.h"
#include "PlayerControls.h"
#include "VideoWidget.h"
#include "AudioArtworkWidget.h"
#include "../engine/MpvEngine.h"
#include "../loop/LoopController.h"
#include "../playlist/PlaylistModel.h"
#include "../theme/ThemeManager.h"

#include <QAction>
#include <QActionGroup>
#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QListView>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QKeySequence>
#include <QLabel>
#include <QIcon>
#include <QPixmap>
#include <QStackedWidget>
#include <QStyle>
#include <QStatusBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_engine(new MpvEngine(this)), m_loop(new LoopController(m_engine, this)),
      m_playlist(new PlaylistModel(this)), m_theme(new ThemeManager(this))
{
    setWindowTitle(tr("RAV Player"));
    setWindowIcon(QIcon::fromTheme(QStringLiteral("com.triasoft.ravplayer"), QIcon(QStringLiteral(":/icons/icon.png"))));
    setAcceptDrops(true);
    resize(1120, 720);
    buildUi();
    buildMenus();
    if (!m_settings.geometry().isEmpty()) restoreGeometry(m_settings.geometry());
    if (!m_settings.windowState().isEmpty()) restoreState(m_settings.windowState());
    m_theme->setTheme(m_settings.theme());
    applyTheme(m_settings.theme());
    m_engine->setVolume(m_settings.volume());
    m_engine->setMuted(m_settings.muted());

    const QStringList saved = m_settings.playlist();
    m_playlist->addFiles(saved);
    const int row = m_settings.currentRow();
    if (row >= 0 && row < m_playlist->rowCount()) m_playlist->setCurrentRow(row);

    connect(m_engine, &MpvEngine::positionChanged, this, [this](double p, double d) { m_controls->setPosition(p, d); });
    connect(m_engine, &MpvEngine::playbackStateChanged, this, [this](bool paused) {
        m_controls->setPlaybackState(paused);
        m_audioView->setMedia(m_currentPath, m_engine->mediaTitle(), {}, {}, paused);
    });
    connect(m_engine, &MpvEngine::volumeChanged, this, [this](int v, bool mute) {
        m_controls->setVolume(v, mute); m_settings.saveVolume(v); m_settings.saveMuted(mute);
    });
    connect(m_engine, &MpvEngine::mediaChanged, this, [this](const QString &title, bool video) {
        m_audioView->setMedia(m_currentPath, title, {}, {}, m_engine->paused());
        m_viewStack->setCurrentWidget(video ? static_cast<QWidget *>(m_video) : static_cast<QWidget *>(m_audioView));
        const QString visibleTitle = title.isEmpty() ? QFileInfo(m_currentPath).completeBaseName() : title;
        m_nowPlayingLabel->setText(visibleTitle.isEmpty() ? tr("Repeat Audio Video Player") : visibleTitle);
        setWindowTitle(visibleTitle.isEmpty() ? tr("RAV Player") : QStringLiteral("%1 — RAV Player").arg(visibleTitle));
    });
    connect(m_engine, &MpvEngine::mediaDetailsChanged, this,
            [this](const QString &title, const QString &artist, const QString &album) {
        m_audioView->setMedia(m_currentPath, title, artist, album, m_engine->paused());
    });
    connect(m_engine, &MpvEngine::endOfFile, this, &MainWindow::playNext);
    connect(m_engine, &MpvEngine::errorOccurred, this, [this](const QString &message) {
        statusBar()->showMessage(message, 8000);
        QMessageBox::warning(this, tr("Playback error"), message);
    });
    connect(m_loop, &LoopController::changed, this, &MainWindow::updateLoopUi);
    connect(m_loop, &LoopController::errorOccurred, this, [this](const QString &message) {
        statusBar()->showMessage(message, 5000);
    });
    connect(m_controls, &PlayerControls::playPauseRequested, m_engine, &MpvEngine::togglePause);
    connect(m_controls, &PlayerControls::stopRequested, m_engine, &MpvEngine::stop);
    connect(m_controls, &PlayerControls::previousRequested, this, &MainWindow::playPrevious);
    connect(m_controls, &PlayerControls::nextRequested, this, &MainWindow::playNext);
    connect(m_controls, &PlayerControls::seekRequested, m_engine, &MpvEngine::seek);
    connect(m_controls, &PlayerControls::volumeRequested, m_engine, &MpvEngine::setVolume);
    connect(m_controls, &PlayerControls::muteRequested, m_engine, &MpvEngine::setMuted);
    connect(m_controls, &PlayerControls::repeatRequested, m_loop, &LoopController::setRepeatEnabled);
    connect(m_controls, &PlayerControls::setARequested, this, [this] { m_loop->setPointA(m_engine->position()); });
    connect(m_controls, &PlayerControls::setBRequested, this, [this] { m_loop->setPointB(m_engine->position()); });
    connect(m_controls, &PlayerControls::clearAbRequested, m_loop, &LoopController::clearAbLoop);
    connect(m_controls, &PlayerControls::fullScreenRequested, this, [this] { setFullScreenMode(!m_fullScreen); });
    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex &i) { playRow(i.row()); });
    connect(m_theme, &ThemeManager::themeChanged, this, &MainWindow::applyTheme);
    if (!m_engine->isReady())
        QMessageBox::critical(this, tr("Media engine initialization failed"), m_engine->initializationError());
    updateLoopUi();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_headerBar = new QFrame(container);
    m_headerBar->setObjectName("appHeader");
    auto *header = new QHBoxLayout(m_headerBar);
    header->setContentsMargins(20, 10, 18, 10);
    header->setSpacing(12);
    auto *logo = new QLabel(m_headerBar);
    logo->setPixmap(QIcon(QStringLiteral(":/icons/icon.png")).pixmap(36, 36));
    logo->setFixedSize(36, 36);
    header->addWidget(logo);
    auto *brandStack = new QVBoxLayout;
    brandStack->setContentsMargins(0, 0, 0, 0);
    brandStack->setSpacing(1);
    auto *brand = new QLabel(tr("RAV PLAYER"), m_headerBar);
    brand->setObjectName("eyebrow");
    brandStack->addWidget(brand);
    m_nowPlayingLabel = new QLabel(tr("Repeat Audio Video Player"), m_headerBar);
    m_nowPlayingLabel->setObjectName("secondaryText");
    brandStack->addWidget(m_nowPlayingLabel);
    header->addLayout(brandStack);
    header->addStretch(1);
    auto *open = new QToolButton(m_headerBar);
    open->setObjectName("primaryButton");
    open->setText(tr("Add media"));
    open->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    open->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    open->setToolTip(tr("Add one or more media files"));
    connect(open, &QToolButton::clicked, this, [this] { openDialog(true); });
    header->addWidget(open);
    m_playlistButton = new QToolButton(m_headerBar);
    m_playlistButton->setObjectName("playlistToggle");
    m_playlistButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogListView));
    m_playlistButton->setText(tr("Playlist"));
    m_playlistButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_playlistButton->setCheckable(true);
    m_playlistButton->setChecked(true);
    m_playlistButton->setToolTip(tr("Show or hide the playlist"));
    header->addWidget(m_playlistButton);
    layout->addWidget(m_headerBar);

    m_viewStack = new QStackedWidget(container);
    m_video = new VideoWidget(m_engine, m_viewStack);
    m_video->setObjectName("videoOutput");
    m_audioView = new AudioArtworkWidget(m_viewStack);
    m_viewStack->addWidget(m_video); m_viewStack->addWidget(m_audioView); m_viewStack->setCurrentWidget(m_audioView);
    layout->addWidget(m_viewStack, 1);
    m_controls = new PlayerControls(container); layout->addWidget(m_controls);
    setCentralWidget(container);

    m_playlistDock = new QDockWidget(tr("Playlist"), this);
    m_playlistDock->setObjectName("playlistDock");
    m_playlistDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
    auto *dockTitle = new QFrame(m_playlistDock);
    dockTitle->setObjectName("playlistTitleBar");
    auto *dockHeader = new QHBoxLayout(dockTitle);
    dockHeader->setContentsMargins(14, 7, 10, 7);
    auto *playlistTitle = new QLabel(tr("UP NEXT"), dockTitle);
    playlistTitle->setObjectName("eyebrow");
    dockHeader->addWidget(playlistTitle);
    dockHeader->addStretch(1);
    m_playlistCount = new QLabel("0", dockTitle);
    m_playlistCount->setObjectName("secondaryText");
    dockHeader->addWidget(m_playlistCount);
    auto *addToPlaylist = new QToolButton(dockTitle);
    addToPlaylist->setObjectName("iconButton");
    addToPlaylist->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    addToPlaylist->setToolTip(tr("Add files to the playlist"));
    addToPlaylist->setFixedSize(32, 32);
    dockHeader->addWidget(addToPlaylist);
    connect(addToPlaylist, &QToolButton::clicked, this, [this] { openDialog(true); });
    m_playlistDock->setTitleBarWidget(dockTitle);

    auto *playlistPanel = new QFrame(m_playlistDock);
    playlistPanel->setObjectName("playlistPanel");
    auto *playlistLayout = new QVBoxLayout(playlistPanel);
    playlistLayout->setContentsMargins(8, 8, 8, 10);
    playlistLayout->setSpacing(0);
    m_playlistStack = new QStackedWidget(playlistPanel);
    m_listView = new QListView(m_playlistStack);
    m_listView->setModel(m_playlist);
    m_listView->setAlternatingRowColors(false);
    m_listView->setSpacing(3);
    m_listView->setUniformItemSizes(true);
    m_listView->setEditTriggers(QListView::NoEditTriggers);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listView->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_playlistStack->addWidget(m_listView);
    auto *empty = new QLabel(tr("Your playlist is empty\n\nAdd audio or video files to get started."), m_playlistStack);
    empty->setObjectName("secondaryText");
    empty->setAlignment(Qt::AlignCenter);
    empty->setWordWrap(true);
    m_playlistStack->addWidget(empty);
    playlistLayout->addWidget(m_playlistStack);
    auto *remove = new QAction(tr("Remove selected"), m_listView);
    connect(remove, &QAction::triggered, this, [this] {
        const int row = m_listView->currentIndex().row();
        if (row < 0) return;
        const bool wasPlaying = row == m_playlist->currentRow();
        m_playlist->removeRowAt(row);
        if (wasPlaying) { m_engine->stop(); m_currentPath.clear(); m_audioView->setMedia({}, {}, {}, {}, true); m_viewStack->setCurrentWidget(m_audioView); }
    });
    m_listView->addAction(remove);
    auto *clear = new QAction(tr("Clear playlist"), m_listView);
    connect(clear, &QAction::triggered, this, [this] { m_engine->stop(); m_playlist->clear(); m_currentPath.clear(); m_audioView->setMedia({}, {}, {}, {}, true); m_viewStack->setCurrentWidget(m_audioView); }); m_listView->addAction(clear);
    m_playlistDock->setWidget(playlistPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_playlistDock);
    m_playlistDock->setMinimumWidth(225);
    connect(m_playlistButton, &QToolButton::toggled, m_playlistDock, &QDockWidget::setVisible);
    connect(m_playlistDock, &QDockWidget::visibilityChanged, m_playlistButton, &QToolButton::setChecked);
    connect(m_playlist, &QAbstractItemModel::rowsInserted, this, &MainWindow::updatePlaylistSummary);
    connect(m_playlist, &QAbstractItemModel::rowsRemoved, this, &MainWindow::updatePlaylistSummary);
    connect(m_playlist, &QAbstractItemModel::modelReset, this, &MainWindow::updatePlaylistSummary);
    updatePlaylistSummary();
}

void MainWindow::buildMenus()
{
    auto *file = menuBar()->addMenu(tr("&File"));
    auto *openOne = file->addAction(tr("Open file…")); openOne->setShortcut(QKeySequence("O")); connect(openOne, &QAction::triggered, this, [this] { openDialog(false); });
    auto *openMany = file->addAction(tr("Add files…")); openMany->setShortcut(QKeySequence("Ctrl+Shift+O")); connect(openMany, &QAction::triggered, this, [this] { openDialog(true); });
    file->addSeparator(); auto *quit = file->addAction(tr("Quit")); quit->setShortcut(QKeySequence::Quit); connect(quit, &QAction::triggered, this, &QWidget::close);

    auto *play = menuBar()->addMenu(tr("&Playback"));
    auto addShortcut = [play](const QString &text, const QKeySequence &key, auto callback) {
        auto *a = play->addAction(text); a->setShortcut(key); QObject::connect(a, &QAction::triggered, callback); return a;
    };
    addShortcut(tr("Play / pause"), QKeySequence(Qt::Key_Space), [this] { m_engine->togglePause(); });
    addShortcut(tr("Stop"), QKeySequence("S"), [this] { m_engine->stop(); });
    addShortcut(tr("Previous"), QKeySequence("Alt+Left"), [this] { playPrevious(); });
    addShortcut(tr("Next"), QKeySequence("Alt+Right"), [this] { playNext(); });
    addShortcut(tr("Seek back 5 seconds"), QKeySequence(Qt::Key_Left), [this] { m_engine->seekRelative(-5.0); });
    addShortcut(tr("Seek forward 5 seconds"), QKeySequence(Qt::Key_Right), [this] { m_engine->seekRelative(5.0); });
    addShortcut(tr("Volume up"), QKeySequence(Qt::Key_Up), [this] { m_engine->setVolume(qMin(100, m_engine->volume() + 5)); });
    addShortcut(tr("Volume down"), QKeySequence(Qt::Key_Down), [this] { m_engine->setVolume(qMax(0, m_engine->volume() - 5)); });
    addShortcut(tr("Mute / unmute"), QKeySequence("M"), [this] { m_engine->setMuted(!m_engine->muted()); });
    auto *repeat = play->addAction(tr("Repeat current file")); repeat->setCheckable(true); repeat->setShortcut(QKeySequence("R"));
    connect(repeat, &QAction::toggled, m_loop, &LoopController::setRepeatEnabled);
    connect(m_controls, &PlayerControls::repeatRequested, repeat, &QAction::setChecked);
    connect(repeat, &QAction::toggled, m_controls, [this](bool enabled) { m_controls->setLoopState(enabled, m_loop->pointA(), m_loop->pointB()); });
    auto *setA = play->addAction(tr("Set A point")); setA->setShortcut(QKeySequence("[")); connect(setA, &QAction::triggered, this, [this] { m_loop->setPointA(m_engine->position()); });
    auto *setB = play->addAction(tr("Set B point")); setB->setShortcut(QKeySequence("]")); connect(setB, &QAction::triggered, this, [this] { m_loop->setPointB(m_engine->position()); });
    auto *clearAb = play->addAction(tr("Clear A–B loop")); clearAb->setShortcut(QKeySequence("\\")); connect(clearAb, &QAction::triggered, m_loop, &LoopController::clearAbLoop);

    auto *view = menuBar()->addMenu(tr("&View"));
    m_themeActions = new QActionGroup(this); m_themeActions->setExclusive(true);
    const QList<QPair<QString, SettingsManager::Theme>> themes = {{tr("System theme"), SettingsManager::Theme::System}, {tr("Light theme"), SettingsManager::Theme::Light}, {tr("Dark theme"), SettingsManager::Theme::Dark}};
    for (const auto &item : themes) {
        auto *action = view->addAction(item.first); action->setCheckable(true); action->setData(int(item.second)); m_themeActions->addAction(action);
        connect(action, &QAction::triggered, this, [this, item] { m_settings.setTheme(item.second); m_theme->setTheme(item.second); });
    }
    auto *full = view->addAction(tr("Full screen")); full->setShortcut(QKeySequence("F")); connect(full, &QAction::triggered, this, [this] { setFullScreenMode(!m_fullScreen); });
    auto *escape = view->addAction(tr("Leave full screen")); escape->setShortcut(QKeySequence(Qt::Key_Escape)); connect(escape, &QAction::triggered, this, [this] { if (m_fullScreen) setFullScreenMode(false); });
    view->addSeparator(); view->addAction(m_playlistDock->toggleViewAction());

    auto *help = menuBar()->addMenu(tr("&Help")); auto *about = help->addAction(tr("About RAV Player")); connect(about, &QAction::triggered, this, &MainWindow::showAbout);
    auto *license = help->addAction(tr("License information")); connect(license, &QAction::triggered, this, [this] {
        QMessageBox::information(this, tr("License"), tr("The project license has not yet been selected.\n\nRAV Player is an independent community project and is not an official GNU Project product. The RAV Player and Triasoft names and logos are trademarks and are not granted by a software license."));
    });
}

void MainWindow::openDialog(bool multiple)
{
    const QString filter = tr("Media files (*.mp3 *.flac *.wav *.ogg *.opus *.m4a *.aac *.mp4 *.mkv *.webm *.mov *.avi *.mpeg *.mpg);;All files (*)");
    if (multiple) openFiles(QFileDialog::getOpenFileNames(this, tr("Add media files"), {}, filter));
    else { const QString file = QFileDialog::getOpenFileName(this, tr("Open media file"), {}, filter); if (!file.isEmpty()) openFiles({file}); }
}
void MainWindow::openFiles(const QStringList &files)
{
    const int start = m_playlist->rowCount(); m_playlist->addFiles(files);
    if (m_playlist->rowCount() > start) playRow(start);
}
void MainWindow::playRow(int row)
{
    const QString path = m_playlist->pathAt(row); if (path.isEmpty()) return;
    if (!QFileInfo::exists(path)) { QMessageBox::warning(this, tr("File not found"), tr("The media file no longer exists:\n%1").arg(path)); return; }
    m_playlist->setCurrentRow(row); m_listView->setCurrentIndex(m_playlist->index(row)); m_currentPath = path;
    m_loop->clearAbLoop(); m_loop->setRepeatEnabled(false); m_engine->loadFile(path); m_engine->setPaused(false);
    m_audioView->setMedia(path, QFileInfo(path).completeBaseName(), {}, {}, false);
}
void MainWindow::playNext() { const int row = m_playlist->nextRow(); if (row >= 0) playRow(row); }
void MainWindow::playPrevious() { const int row = m_playlist->previousRow(); if (row >= 0) playRow(row); }
void MainWindow::updateLoopUi()
{
    const double a = m_loop->hasA() ? m_loop->pointA() : -1.0;
    const double b = m_loop->hasB() ? m_loop->pointB() : -1.0;
    m_controls->setLoopState(m_loop->repeatEnabled(), a, b);
}
void MainWindow::applyTheme(SettingsManager::Theme theme)
{
    for (QAction *action : m_themeActions->actions()) action->setChecked(action->data().toInt() == int(theme));
    m_theme->apply();
}
void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About RAV Player"), tr("<h2>RAV Player 0.1.2</h2><p>Repeat Audio Video Player</p><p>Developed by Triasoft</p><p>Pre-Alpha</p><p>An independent community project. Not an official GNU Project product.</p>"));
}
void MainWindow::setFullScreenMode(bool enabled)
{
    if (m_fullScreen == enabled) return;
    if (enabled) {
        m_wasMaximized = isMaximized();
        m_fullScreen = true; m_playlistDock->hide(); menuBar()->hide(); m_headerBar->hide();
        showFullScreen();
    } else {
        m_fullScreen = false; showNormal();
        menuBar()->show(); m_headerBar->show(); m_playlistDock->show();
        if (m_wasMaximized) showMaximized();
    }
}
void MainWindow::updatePlaylistSummary()
{
    if (!m_playlistStack || !m_playlistCount) return;
    const int count = m_playlist->rowCount();
    m_playlistCount->setText(QString::number(count));
    m_playlistStack->setCurrentIndex(count > 0 ? 0 : 1);
}
void MainWindow::dragEnterEvent(QDragEnterEvent *event) { if (event->mimeData()->hasUrls()) event->acceptProposedAction(); }
void MainWindow::dropEvent(QDropEvent *event)
{
    QStringList files; for (const QUrl &url : event->mimeData()->urls()) if (url.isLocalFile()) files << url.toLocalFile();
    if (!files.isEmpty()) openFiles(files);
    event->acceptProposedAction();
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    m_settings.saveGeometry(saveGeometry()); m_settings.saveWindowState(saveState());
    m_settings.savePlaylist(m_playlist->paths(), m_playlist->currentRow());
    QMainWindow::closeEvent(event);
}
