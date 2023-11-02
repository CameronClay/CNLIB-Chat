cmake_minimum_required(VERSION 3.5)

project(WNLIB-CHAT VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(TCPCS)
add_subdirectory(Client)
add_subdirectory(Server)
add_subdirectory(MyUtils)
