cmake_minimum_required(VERSION 3.16)

project(HttpServer VERSION 1.0.0 LANGUAGES CXX)

find_package(PkgConfig)
pkg_check_modules(FMT REQUIRED fmt)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(qtpromise
  GIT_REPOSITORY https://github.com/simonbrunel/qtpromise.git
  GIT_TAG v0.7.0
  GIT_SHALLOW true
)

# CMake v3.14+
FetchContent_MakeAvailable(qtpromise)

find_package(Qt5 REQUIRED COMPONENTS Core Network)

add_subdirectory(./src)
add_subdirectory(./test)
add_subdirectory(./example)
