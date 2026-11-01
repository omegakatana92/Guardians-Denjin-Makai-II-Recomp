# Steam Deck notes

Target artifact: `Guardians-SteamDeck-x86_64.AppImage`.

**Status: packaging is prepared, not built or tested on a Steam Deck.** The
native Linux target is the same code as the Windows build; it has not been
compiled on Linux in this environment. Do not publish an AppImage until the
Linux build and a runtime smoke test have actually passed.

## Build environment

Use a Linux x86-64 container/runner with:

```bash
sudo apt install build-essential cmake ninja-build python3 libsdl2-dev \
  file wget desktop-file-utils
```

Build the native binary first:

```bash
./scripts/build-local.sh /path/to/grdians.zip
```

Then package:

```bash
./scripts/build-appimage.sh
```

The script expects `appimagetool` or `linuxdeploy` on `PATH` and writes
`dist/Guardians-SteamDeck-x86_64.AppImage`. It never adds a ROM.

## Runtime on the Deck

- Add the AppImage as a non-Steam game.
- Launch once with the user's `grdians.zip`; the default config is
  `guardians.cfg` next to the executable.
- The SDL2 front end uses the Deck's evdev gamepad through SDL's controller API.
- Display: integer scaling and fullscreen are supported (`F11` toggles).
- Audio: not implemented in the current milestone; the X1-010 mixer is the next
  audio task.

## Configuration

`guardians.cfg` is a plain text file:

```
rom=/home/deck/roms/grdians.zip
fullscreen=1
scale=2
```

Command-line options override the file.

## Honest status

The AppImage path and desktop file exist so the packaging can be completed once
the Linux build and video/audio correctness are verified. Shipping a "playable"
AppImage now would be inaccurate.
