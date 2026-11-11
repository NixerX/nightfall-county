cmake_minimum_required(VERSION 3.16)
project(nightfall_county_3d CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# raylib 5.5, fetched and built statically (or use a local checkout in ./raylib)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
if(EXISTS ${CMAKE_SOURCE_DIR}/raylib/CMakeLists.txt)
  add_subdirectory(raylib)
else()
  include(FetchContent)
  FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(raylib)
endif()

add_executable(nightfall src/main.cpp)
target_link_libraries(nightfall PRIVATE raylib)
if(MSVC)
  target_compile_options(nightfall PRIVATE /O2 /W1)
else()
  target_compile_options(nightfall PRIVATE -O2 -Wall -Wno-misleading-indentation)
endif()
add_custom_command(TARGET nightfall POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:nightfall>/assets)
