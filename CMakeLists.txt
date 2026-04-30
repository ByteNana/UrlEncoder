cmake_minimum_required(VERSION 3.15)
project(UrlEncoder VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

option(BUILD_TESTING "Build tests" OFF)
set(_build_testing_saved ${BUILD_TESTING})

include(FetchContent)

FetchContent_Declare(
  ArduinoMock
  GIT_REPOSITORY https://github.com/ByteNana/ArduinoMock.git
  GIT_TAG master
)
# Prevent ArduinoMock from building its own test suite
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(ArduinoMock)
set(BUILD_TESTING ${_build_testing_saved} CACHE BOOL "" FORCE)

add_library(Urls STATIC src/Urls.cpp)
target_include_directories(Urls PUBLIC src)
target_link_libraries(Urls PUBLIC ArduinoNativeMocks)

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()
