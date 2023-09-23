cmake_minimum_required(VERSION 3.13)

project(WNLIB-CHAT)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(TCPCS)
add_subdirectory(Client)
add_subdirectory(Server)