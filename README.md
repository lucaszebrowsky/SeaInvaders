# About

**SeaInvaders** (C-Invaders) is a _Taito Space Invaders_ Emulator written in C using SDL2.

# Dependencies

You will need the SDL2 libraries, cmake and a C11 compatible compiler. The emulator was written and tested using `clang` but `gcc` should also work.

## Arch

```shell
sudo pacman -Syu clang sdl2 cmake git
```

## Debian/Ubuntu

```shell
sudo apt install clang libsdl2-dev cmake git
```

# Compilation

First clone this repository via git:

```shell
git clone https://github.com/Lu-Die-Milchkuh/SeaInvaders.git
```

and then `cd` into the `SeaInvaders` directory:

```shell
cd SeaInvaders
```

## Building

By default SeaInvaders is build in debug mode if the build type is not specified. To specify that you want to build a release build you need to set the **-DCMAKE_BUILD_TYPE** flag:

```shell
cmake -B build -DCMAKE_C_COMPILER=/bin/clang -DCMAKE_BUILD_TYPE=Release
```

then you can simply compile the poject via:

```shell
cmake --build build
```

This will generate an executable called **SeaInvaders** in the build directory.

# Loading the ROM

> [!Note]
> In the **rom** directory you will find the original **Taito Space Invaders** rom.

You can run the emulator via:

```shell
./build/SeaInvaders rom/SpaceInvaders.bin
```

# Control Sheme

| Key         |        Action        |
| ----------- | :------------------: |
| c           |     Insert Coin      |
| 1           | Select 1 Player Mode |
| 2           | Select 2 Player Mode |
| Arrow_UP    |   Player 1: Shoot    |
| Arrow_LEFT  | Player 1: move Left  |
| Arrow_RIGHT | Player 1: move Right |
| w           |   Player 2: Shoot    |
| a           | Player 2: move Left  |
| d           | Player 2: move Right |
