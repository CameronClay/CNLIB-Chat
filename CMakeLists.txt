cmake_minimum_required(VERSION 3.5)

# if(NOT CMAKE_TOOLCHAIN_FILE)
#   set(CMAKE_TOOLCHAIN_FILE
#       "C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake"
#       CACHE STRING "" FORCE)
# endif()

# attach to vcpkg's cmake configuration for project dependencies
# set(CMAKE_TOOLCHAIN_FILE "C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake")

# This will make issues in the future
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")


project(WNLIB-CHAT VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(TCPCS)
add_subdirectory(Client)
add_subdirectory(Server)
add_subdirectory(MyUtils)
add_subdirectory(Common)
add_subdirectory(CNLIB_Tests)
