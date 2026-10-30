# RAV Player 0.1.2 Pre-Alpha

RAV Player (Repeat Audio Video Player) is an independent GNU/Linux desktop media player developed by Triasoft. It is not an official GNU Project product. The application is built with C++20, Qt 6 Widgets, and libmpv.

## Features

- Audio and video playback through libmpv, with an embedded OpenGL video view.
- Add multiple files with the file picker, drag and drop, or command-line paths.
- Playlist selection, next/previous navigation, removal, and persistent playlist state.
- Play/pause, stop, seeking, volume, mute, and full-screen window mode.
- Repeat-one playback and MPV A–B looping. Set A and B from the current play position; B must follow A.
- System, light, and dark appearance choices. Light/dark changes apply immediately.
- Persistent window geometry, volume, mute, theme, and playlist settings.
- The project supplied icon at `resources/icons/icon.png`, installed as the desktop application icon and bundled as the in-window icon fallback.

## Build on GNU/Linux

Install a C++20 compiler, CMake 3.25 or newer, Ninja, Qt 6 Widgets and OpenGLWidgets development packages, and libmpv development packages. Then run:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/rav-player
```

Open files from **File → Open file**, drop files onto the window, or launch with paths:

```sh
./build/rav-player ~/Videos/example.mkv ~/Music/example.flac
```

## Controls

| Key | Action |
| --- | --- |
| Space | Play/pause |
| Left / Right | Seek back/forward 5 seconds |
| Up / Down | Raise/lower volume |
| M | Mute/unmute |
| F | Full screen |
| O | Open a file |
| Esc | Leave full screen |
| [ / ] | Set A / B loop points |
| R | Repeat current file |

The A button stores the current position as A and clears B. The B button applies the loop once B is later than A. “A–B ×” clears the range.

## Flatpak

The manifest is in `packaging/flatpak/com.triasoft.ravplayer.yml`. It targets the KDE 6.10 runtime and requests Wayland/X11 display, audio, IPC, and GPU access. File access is intended to go through desktop file chooser and drag-and-drop portals; it does not request broad home-directory access.

libmpv is not part of the standard Flathub shared-modules catalog. A Flatpak build must bundle libmpv and its codec/rendering dependencies using a maintained libmpv module before the manifest can build. The manifest currently records that packaging target but the Flatpak build has not been validated. Native CMake builds use the system `mpv` pkg-config package.

## License and marks

The project's `LICENSE` file is currently empty, and no software license has been selected. GPL-3.0-or-later is one option for the maintainers to evaluate; it has not been applied here. Software licensing and the Triasoft/RAV Player name and logo rights are separate matters. Do not treat a software license as permission to use those marks.

## Current scope

This is pre-alpha software. On the current Ubuntu development environment, a clean Release build with Ninja completed using Qt 6.10.2 and libmpv 2.5.0. The supplied WAV path containing a space was opened by the application; MPV reported 2.69546 seconds, initialized a 44.1 kHz stereo PipeWire output, and started audio playback. `LC_NUMERIC` was `C` before MPV initialization and no locale warning appeared.

Video rendering, audible output on physical speakers, pause/resume and seeking through the controls, repeat and A–B boundaries, visual theme changes, resize behavior, and Flatpak packaging have not yet been independently verified. An offscreen Qt plugin cannot validate the OpenGL video widget. Flatpak packaging remains incomplete until a maintained libmpv module and its runtime dependencies are added.
