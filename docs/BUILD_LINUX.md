# Building on Linux (x86-64)

These instructions are written for a normal Linux x86-64 build. They were **not
executed in this Windows development environment**; the source and CMake logic
are shared with the verified Windows build.

## 1. Prerequisites

Debian/Ubuntu:

```bash
sudo apt install build-essential cmake ninja-build python3 libsdl2-dev
```

Fedora:

```bash
sudo dnf install gcc gcc-c++ cmake ninja-build python3 SDL2-devel
```

## 2. Verify and build

```bash
python3 tools/verify_rom.py /path/to/grdians.zip
./scripts/build-local.sh /path/to/grdians.zip
```

or manually:

```bash
cmake -S . -B build/cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DGRDIANS_ROM=/path/to/grdians.zip
cmake --build build/cmake
```

Outputs:

- `build/cmake/bin/guardians_headless`
- `build/cmake/bin/Guardians`

## 3. Run

```bash
./build/cmake/bin/guardians_headless --rom /path/to/grdians.zip --frames 600 --hashes build/boot.hashes
./build/cmake/bin/Guardians --rom /path/to/grdians.zip
```

## 4. Notes

- The generated C under `build/generated/` is ROM-derived and must not be
  committed, packaged or redistributed.
- Video and audio status is the same as Windows; see `docs/HARDWARE_PLAN.md`.
- The Linux binary expects the user's `grdians.zip`; no ROM is embedded.
