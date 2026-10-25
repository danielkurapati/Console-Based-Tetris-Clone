cmake_minimum_required(VERSION 3.12)
project(tetris LANGUAGES CXX)

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
# Needs the MySQL C client library (libmysqlclient or MariaDB Connector/C) and its header.
# If they are not installed, only the game is built.
find_library(MYSQL_CLIENT_LIB NAMES mysqlclient mariadb libmariadb libmysql)
find_path(MYSQL_INCLUDE_DIR NAMES mysql.h PATH_SUFFIXES mysql mariadb)

if(MYSQL_CLIENT_LIB AND MYSQL_INCLUDE_DIR)
    file(GLOB SERVER_SOURCES CONFIGURE_DEPENDS server/*.cpp)
    add_executable(tetris_server ${SERVER_SOURCES})
    target_include_directories(tetris_server PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(tetris_server PRIVATE ${MYSQL_CLIENT_LIB} Threads::Threads)
    if(WIN32)
        target_link_libraries(tetris_server PRIVATE ws2_32)
    endif()
else()
    message(STATUS "MySQL client library not found: building the game only (no tetris_server)")
endif()

# ---- Warnings -------------------------------------------------------------------------
foreach(target tetris tetris_server)
    if(TARGET ${target})
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra)
        endif()
    endif()
endforeach()
