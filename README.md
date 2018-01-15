# Basin
A high performance Minecraft server implementation written in C.

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
* C Plugin System
* Block & Sky Lighting

## Planned Features
* Natural Mob Spawning & AI
* Lua Plugin Systems
* Comprehensive permissions & commands system
* Ultra-low memory mode - Sacrifice CPU Cycles for further memory use reduction
* Entities-by-chunk logic - Hold millions of entities in a world
* World generation
* Comprehensive opt-in Anticheat
* GPU accelerated world generation, AI, and more(?).

# Compiling
To compile Basin, you can use Make directly or use CMake if you choose. Output is always put in basin/Debug regardless of build system.

### Make:
To compile: `cd basin/Debug; make`
To clean: `make clean` (in Debug)

### CMake:
To compile:
```
mkdir cmake-build-debug
cd cmake-build-debug
cmake ..
make -j 4
```
To clean: `make clean` (in cmake-build-debug)

# Running Basin
To run Basin, simply invoke the binary `./basin`. Ensure that the binary is located with the relevant data files (the JSONs).
