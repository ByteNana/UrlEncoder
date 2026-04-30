cmake_minimum_required(VERSION 3.15)

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

function(add_url_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE Urls GTest::gtest_main GTest::gmock_main)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_url_test(urls_gtest urls_gtest.cpp)
