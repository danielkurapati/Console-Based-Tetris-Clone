cmake_minimum_required(VERSION 3.12)
project(tetris LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

# ---- The game (client) ----------------------------------------------------------------
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(tetris ${GAME_SOURCES})
target_include_directories(tetris PRIVATE src)
target_link_libraries(tetris PRIVATE Threads::Threads)
if(WIN32)
    target_link_libraries(tetris PRIVATE ws2_32)
endif()

# ---- The leaderboard REST API server --------------------------------------------------
# The database is SQLite. Its source (one big C file) lives in server/third_party/, so there
# is nothing to install: the server is always built.
file(GLOB SERVER_SOURCES CONFIGURE_DEPENDS server/*.cpp)

add_executable(tetris_server ${SERVER_SOURCES} server/third_party/sqlite3.c)
target_link_libraries(tetris_server PRIVATE Threads::Threads)
set_source_files_properties(server/third_party/sqlite3.c PROPERTIES
    COMPILE_DEFINITIONS "SQLITE_THREADSAFE=1;SQLITE_OMIT_LOAD_EXTENSION")
if(WIN32)
    target_link_libraries(tetris_server PRIVATE ws2_32)
endif()

# ---- Warnings -------------------------------------------------------------------------
foreach(target tetris tetris_server)
    if(TARGET ${target})
        # Warnings only for our own C++ code (not for the third-party sqlite3.c).
        if(MSVC)
            target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4>)
        else()
            target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra>)
        endif()
    endif()
endforeach()
