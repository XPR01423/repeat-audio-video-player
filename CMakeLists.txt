cmake_minimum_required(VERSION 3.25)
project(RAVPlayer VERSION 0.1.2 DESCRIPTION "Repeat Audio Video Player" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

add_executable(rav-player
    src/main.cpp
    src/engine/MpvEngine.cpp src/engine/MpvEngine.h
    src/ui/MainWindow.cpp src/ui/MainWindow.h
    src/ui/VideoWidget.cpp src/ui/VideoWidget.h
    src/ui/AudioArtworkWidget.cpp src/ui/AudioArtworkWidget.h
    src/ui/PlayerControls.cpp src/ui/PlayerControls.h
    src/loop/LoopController.cpp src/loop/LoopController.h
    src/playlist/PlaylistModel.cpp src/playlist/PlaylistModel.h
    src/settings/SettingsManager.cpp src/settings/SettingsManager.h
    src/theme/ThemeManager.cpp src/theme/ThemeManager.h
    resources/resources.qrc
)
target_link_libraries(rav-player PRIVATE Qt6::Widgets Qt6::OpenGLWidgets PkgConfig::MPV)
target_compile_options(rav-player PRIVATE -Wall -Wextra -Wpedantic)

include(GNUInstallDirs)
install(TARGETS rav-player RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/com.triasoft.ravplayer.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES resources/icons/icon.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps RENAME com.triasoft.ravplayer.png)
install(FILES packaging/com.triasoft.ravplayer.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
