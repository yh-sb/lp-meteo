# Lviv Polytechnic meteostation

[![build](https://github.com/yhsb2k/lp-meteo/workflows/build/badge.svg)](https://github.com/yhsb2k/lp-meteo/actions?workflow=build)
[![license](https://img.shields.io/github/license/yhsb2k/lp-meteo?color=blue)](https://github.com/yhsb2k/lp-meteo/blob/master/LICENSE)

![](https://github.com/r44083/lp-meteo/blob/master/image.jpg)

https://github.com/r44083/lp-meteo

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

## Requirements
* [GNU Arm Embedded Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
* [CMake](https://cmake.org/download)
* [Ninja](https://ninja-build.org)
* [Make](https://winlibs.com)
* For Linux (apt): `apt install cmake ninja-build`
