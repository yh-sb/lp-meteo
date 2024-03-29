# Lviv Polytechnic meteostation

[![build](https://github.com/yhsb2k/lp-meteo/workflows/build/badge.svg)](https://github.com/yhsb2k/lp-meteo/actions?workflow=build)
[![license](https://img.shields.io/github/license/yhsb2k/lp-meteo?color=blue)](https://github.com/yhsb2k/lp-meteo/blob/master/LICENSE)

![](https://github.com/yhsb2k/lp-meteo/blob/master/image.jpg)

## How to build
```
git clone --recursive https://github.com/yhsb2k/lp-meteo.git
cd lp-meteo
make
```
**Other targets:**
```
make flash - Upload firmware to the target
make erase - Erase all memory on the target
make reset - Reset the target
make debug - Upload firmware to the target and start the debug session
```

## Debug
Just start `ARM` debug configuration in [vscode](https://code.visualstudio.com)

## Requirements
* [GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
* [CMake](https://cmake.org/download)
* [Ninja](https://ninja-build.org)
* [Make](https://winlibs.com)
* [JLink](https://www.segger.com/downloads/jlink) + [ST-LINK On-Board as JLink probe](https://www.segger.com/products/debug-probes/j-link/models/other-j-links/st-link-on-board/#getting-started-with-st-link-on-board)
* For Linux (apt): `apt install cmake ninja-build gcc-arm-none-eabi`
