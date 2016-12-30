cmake_minimum_required(VERSION 3.6)
project(Basin)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/basin/Debug)

set(SOURCE_FILES
        basin/src/accept.c
        basin/src/accept.h
        basin/src/block.c
        basin/src/block.h
        basin/src/collection.c
        basin/src/collection.h
        basin/src/command.c
        basin/src/command.h
        basin/src/config.c
        basin/src/config.h
        basin/src/crafting.c
        basin/src/crafting.h
        basin/src/entity.c
        basin/src/entity.h
        basin/src/game.c
        basin/src/game.h
        basin/src/globals.h
        basin/src/inventory.c
        basin/src/inventory.h
        basin/src/item.c
        basin/src/item.h
        basin/src/json.c
        basin/src/json.h
        basin/src/log.c
        basin/src/log.h
        basin/src/main.c
        basin/src/nbt.c
        basin/src/nbt.h
        basin/src/network.c
        basin/src/network.h
        basin/src/packet.c
        basin/src/packet.h
        basin/src/player.c
        basin/src/player.h
        basin/src/queue.c
        basin/src/queue.h
        basin/src/server.h
        basin/src/smelting.c
        basin/src/smelting.h
        basin/src/streams.c
        basin/src/streams.h
        basin/src/tileentity.c
        basin/src/tileentity.h
        basin/src/tools.c
        basin/src/tools.h
        basin/src/util.c
        basin/src/util.h
        basin/src/version.h
        basin/src/work.c
        basin/src/work.h
        basin/src/world.c
        basin/src/world.h
        basin/src/worldmanager.c
        basin/src/worldmanager.h
        basin/src/xstring.c
        basin/src/xstring.h)

add_executable(Basin ${SOURCE_FILES})

target_link_libraries(Basin pthread crypto ssl m z)