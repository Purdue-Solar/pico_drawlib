# Thank You
Thank you to [ST7789 Library for Pico](https://github.com/ArmDeveloperEcosystem/st7789-library-for-pico)

# Building
Use `CMakeLists.txt` to compile for the Raspberry Pi Pico 2.

# Simulation (make, linux only)
Run `make` to compile a simulation that you can run on your computer.
The built simulation will be located at `simbuild/pico_drawlib`.

## Display simulation (cmake, windows or linux)

`components/pico_drawlib/Sim` contains a desktop simulation of the ILI9341
display that renders to a [raylib](https://www.raylib.com/) window. It is a
**completely separate build** from the embedded firmware — it uses your native
host `gcc`/`g++`, not the ARM cross-compiler, and has its own out-of-source
build directory.

### Simulation prerequisites

**Windows (Chocolatey)**

The VS Code Pico extension's ARM toolchain is a cross-compiler and cannot build
native Windows code. Install MinGW-w64 (provides native `gcc`/`g++`):

```powershell
choco install mingw
```

Confirm it is on your PATH before running CMake:

```powershell
gcc --version
```

**Linux / WSL2**

```bash
sudo apt install gcc g++ cmake ninja-build
# raylib is auto-downloaded by CMake if not found; to install system-wide:
sudo apt install libraylib-dev   # Ubuntu 22.04+ / Debian 12+
```

### Build the simulation

**Windows** — run from the repo root in a plain PowerShell (not the Pico
extension terminal, which only has the ARM toolchain on its PATH):

```powershell
cmake -S Sim -B Sim/build `
      -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build Sim/build
.\Sim\build\pico_drawlib_sim.exe
```

**Linux / WSL2:**

```bash
cmake -S Sim -B Sim/build -G Ninja
cmake --build Sim/build
./Sim/build/pico_drawlib_sim
```

If raylib is not installed system-wide, CMake fetches and builds it from GitHub
on the first configure (requires internet; takes about a minute). Subsequent
builds reuse the cached download.

### Clean simulation build

```bash
rm -rf Sim/build   # Linux
rmdir /S /Q Sim\build  # Windows PowerShell
```

---

