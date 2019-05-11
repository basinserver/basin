# Basin

> A high performance Minecraft server implementation written in C.

### Status

Currently, version Minecraft 1.11.2 and 1.10.2 is supported. As of April 2019, this project is being refactored to meet higher standards of quality and stability. Feature development will resume afterwards, with Minecraft 1.14 support coming concurrently.

## Features
* Low memory footprint (13 MB for a 400-chunk spawn and one player on a 64-bit system)
* Block placing/breaking
* Survival mode
* Creative mode
* Inventories
* Players, Chat, and Tab menu
* Item entities
* Chests, Furnaces, Crafting
* PvP
* Fall damage
* Block physics & updates
* C plugin system
* Block & sky lighting
* Anvil world loading

### Work In Progress
* Mob AI
* Spigot-mirrored Java plugin system
* Command system
* World generation
* Anticheat

### Planned
* Protocol generator/export Minecraft protocol as a shared library for various versions
* Natural mob spawning
* Lua plugin system
* Permission system
* World saving

## Getting Started

### Prerequisites

Basin uses `cmake` to build, install, and package.

### Dependencies
* [avuna-util](https://github.com/Protryon/avuna-util)
* OpenSSL
* JNI (if Java plugin support is enabled)

### Compilation

* Clone this project.

```bash
git clone https://github.com/basinserver/basin.git
```
* Make a folder called "build" in the Basin directory.

```bash
mkdir build
```

* Open a terminal inside of the build directory.

```bash
cd build
```

* Run CMake and target the root directory.

```bash
cmake ..
```

* Use make.

```
make -j 16
```

### Cleaning

In order to get rid of the compiled executables, you will have to clean your directory with the following command:

```bash
make clean
```

### Installation

In order to install Basin, you will have to execute the following command after compiling the project:

```bash
sudo make install
```

### Package

In order to package Basin, you will have to execute the following command:

```
cpack --config ./CPackConfig.cmake
```

### Running Basin

To run Basin, simply invoke the binary `./basin`. Ensure that the binary is located with the relevant data files (the JSONs).

## Goals

* Pure performance: minimum resource usage for a full Minecraft server implementation
* Drop-in replacement: compatible with Spigot API plugins via JNI, standard Minecraft world format (Anvil)
* Extensibility: ease of modification, large plugin API
* Stability: use good C programming standards to prevent server-breaking bugs

### Perhaps, one day.

* Allow server-side reimplementation of Java forge mods in C via an analog Forge API
* Allow several versions of the game without a third party tool via a protocol compatibility layer
* Reverse proxy dimension/world sourcing

## Discord Server

Are you a developer who's keen on Basin, or perhaps just a simple onlooker?

Either way, feel free to join the official Basin Discord server [here](https://discord.gg/3uW2QQV)!
