cmake_minimum_required(VERSION 3.0)

project(TILEGEN
  VERSION 0.1.0
  LANGUAGES CXX
)

if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

if(UNIX)
  option(TILEGEN_DEV_ENABLE_ASAN "Enable Address Sanitizer" OFF)
endif()

include(GNUInstallDirs)

find_package(gf REQUIRED)

if(MSVC)
  message(STATUS "Using MSVC compiler")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DNOMINMAX /W4 /utf-8 /std:c++14 /permissive-")
  link_directories(${Boost_LIBRARY_DIRS})
  # for more information on the last two switches:
  # - https://blogs.msdn.microsoft.com/vcblog/2016/06/07/standards-version-switches-in-the-compiler/
  # - https://blogs.msdn.microsoft.com/vcblog/2016/11/16/permissive-switch/
else()
  message(STATUS "Using GCC or Clang compiler")
  # assume it's a GNU-like compiler (GCC, clang)
  set(CMAKE_CXX_FLAGS                 "-Wall -Wextra -std=c++14 -pedantic")
  set(CMAKE_CXX_FLAGS_DEBUG           "-O0 -g")
  set(CMAKE_CXX_FLAGS_MINSIZEREL      "-Os -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE         "-O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "-O2 -g")

  set(CMAKE_C_FLAGS                   "-Wall -std=c99")
  set(CMAKE_C_FLAGS_DEBUG             "-O0 -g")
  set(CMAKE_C_FLAGS_MINSIZEREL        "-Os -DNDEBUG")
  set(CMAKE_C_FLAGS_RELEASE           "-O3 -DNDEBUG")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO    "-O2 -g")
endif()

if(UNIX)
  if(TILEGEN_DEV_ENABLE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  endif()
endif()

add_subdirectory(code)
