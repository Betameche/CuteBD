# CuteBD - Minimal Qt5 C++ app

## Detected environment (this machine)

- `g++`: `13.3.0`
- `cmake`: `3.28.3`
- C++ standard check with `g++ -std=c++23`: OK (`__cplusplus = 202100`)
- Qt5: not currently installed (`qmake` not found, `pkg-config Qt5Core/Qt5Widgets` not found)

## Install Qt5 on Ubuntu

```bash
sudo apt update
sudo apt install -y qtbase5-dev qtchooser
```

Optional tools:

```bash
sudo apt install -y qt5-qmake
```

## Build and run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/cutebd_qt5_minimal
```

## Notes

- Project is configured with `CMAKE_CXX_STANDARD 23`.
- If your Qt5 toolchain has constraints on your system, you can lower to C++20 by changing one line in `CMakeLists.txt`.
