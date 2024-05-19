add_library(TCPCS SHARED
    Stdafx.cpp
    ${CMAKE_SOURCE_DIR}/Common/CNLIB/BuffAllocator.h DLLSRC/BuffAllocator.cpp
    ${CMAKE_SOURCE_DIR}/Common/CNLIB/BufferOptions.h DLLSRC/BufferOptions.cpp
    ${CMAKE_SOURCE_DIR}/Common/CNLIB/BuffSendInfo.h  DLLSRC/BuffSendInfo.cpp
    DLLSRC/BufSendAlloc.h                            DLLSRC/BufSendAlloc.cpp
    ${CMAKE_SOURCE_DIR}/Common/CNLIB/CritLock.h      DLLSRC/CritLock.cpp
    ${CMAKE_SOURCE_DIR}/Common/CNLIB/File.h          DLLSRC/File.cpp
    ${CMAKE_SOURCE_DIR}/Common/CNLIB/HeapAlloc.h     DLLSRC/HeapAlloc.cpp
    ${CMAKE_SOURCE_DIR}/Common/CNLIB/IOCP.h          DLLSRC/IOCP.cpp
    DLLSRC/KeepAlive.h                               DLLSRC/KeepAlive.cpp
    DLLSRC/RecvHandler.h                             DLLSRC/RecvHandler.cpp
    ${CMAKE_SOURCE_DIR}/Common/CNLIB/Socket.h        DLLSRC/Socket.cpp
    ${CMAKE_SOURCE_DIR}/Common/CNLIB/SocketInfo.h    DLLSRC/SocketInfo.cpp
    ${CMAKE_SOURCE_DIR}/Common/CNLIB/SocketOptions.h DLLSRC/SocketOptions.cpp
    DLLSRC/TCPClient.h                               DLLSRC/TCPClient.cpp
    DLLSRC/TCPServ.h                                 DLLSRC/TCPServ.cpp
    ${CMAKE_SOURCE_DIR}/Common/CNLIB/Typedefs.h      DLLSRC/Typedefs.cpp
    ${CMAKE_SOURCE_DIR}/Common/CNLIB/UPNP.h          DLLSRC/UPNP.cpp
)

target_compile_definitions(TCPCS PRIVATE TCPCS_EXPORTS)
target_compile_definitions(TCPCS PUBLIC UNICODE)
target_compile_definitions(TCPCS PUBLIC _UNICODE)


target_link_libraries(TCPCS PRIVATE comctl32)
target_link_libraries(TCPCS PRIVATE shlwapi)

target_include_directories(TCPCS PUBLIC ${CMAKE_SOURCE_DIR}/Common)
target_link_directories(TCPCS PRIVATE ${CMAKE_SOURCE_DIR}/Common ${CMAKE_SOURCE_DIR}/Common/zlib/lib/x64)
target_link_libraries(TCPCS PRIVATE zdll)

target_precompile_headers(TCPCS PRIVATE
    "Stdafx.h"
)
