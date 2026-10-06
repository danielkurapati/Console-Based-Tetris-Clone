cmake_minimum_required(VERSION 3.20)
project(tetris LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(tetris
	src/main.cpp
)

target_compile_features(tetris PRIVATE cxx_std_17)


