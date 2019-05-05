# Basin
A high performance Minecraft server implementation written in C.

## Discord Server

Join the Discord server [here](https://discord.gg/3uW2QQV).

# Goals
* Pure performance: minimum resource usage for a full Minecraft server implementation
* Drop-in replacement: compatible with Spigot API plugins via JNI, standard Minecraft world format (Anvil)
* Extensibility: ease of modification, large plugin API
* Stability: use good C programming standards to prevent server-breaking bugs

# Status

Currently, version Minecraft 1.11.2 and 1.10.2 is supported. As of April 2019, this project is being refactored to meet higher standards of quality and stability. Feature development will resume afterwards, with Minecraft 1.14 support coming concurrently.

# Features
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

## Work In Progress
* Mob AI
* Spigot-mirrored Java plugin system
* Command system
* World generation
* Anticheat

## Planned
* Protocol generator/export Minecraft protocol as a shared library for various versions
* Natural mob spawning
* Lua plugin system
* Permission system
* World saving

## Maybe One Day
* Forge mod analog API to allow server-side reimplementation of Java forge mods in C
* Protocol compatibility layer to allow several versions of the game without a third party tool
* Reverse proxy dimension/world sourcing

# Development
Basin uses `cmake` to build, install, and package.

## Dependencies
* [avuna-util](https://github.com/Protryon/avuna-util)
* OpenSSL
* JNI (if Java plugin support is enabled)

## Compilation:
```
mkdir build
cd build
cmake ..
make -j 16
```

## Clean
Within your build directory:
```
make clean
```

## Install
Within your build directory:
```
sudo make install
```

## Package
Within your build directory:
```
cpack --config ./CPackConfig.cmake
```

# Running Basin
To run Basin, simply invoke the binary `./basin`. Ensure that the binary is located with the relevant data files (the JSONs).
