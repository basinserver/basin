cmake_minimum_required(VERSION 3.6)
project(basin)

set(CMAKE_C_STANDARD 11)

option(JAVA_PLUGIN_SUPPORT "Enables Java plugin support, requires JAVA_HOME variable" ON)

set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wno-discarded-qualifiers")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

file(GLOB src
        "src/*.c"
        "src/*.h"
        )

add_executable(basin ${src})

target_link_libraries(basin pthread crypto ssl m z dl avuna-util)
target_include_directories(basin PUBLIC include/)

if (JAVA_PLUGIN_SUPPORT)
    find_package(JNI)
    target_link_directories(basin PRIVATE "${JAVA_JVM_LIBRARY}")
    target_link_libraries(basin jvm)
    target_include_directories(basin PRIVATE "${JAVA_INCLUDE_PATH}")
endif()

