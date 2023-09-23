add_library(TCPCS SHARED
    Stdafx.cpp
    DLLSRC/BuffAllocator.cpp 
    DLLSRC/BufferOptions.cpp 
    DLLSRC/BuffSendInfo.cpp 
    DLLSRC/BufSendAlloc.cpp 
    DLLSRC/CritLock.cpp 
    DLLSRC/File.cpp 
    DLLSRC/HeapAlloc.cpp 
    DLLSRC/IOCP.cpp 
    DLLSRC/KeepAlive.cpp 
    DLLSRC/RecvHandler.cpp 
    DLLSRC/Socket.cpp 
    DLLSRC/SocketInfo.cpp 
    DLLSRC/SocketOptions.cpp 
    DLLSRC/TCPClient.cpp 
    DLLSRC/TCPServ.cpp 
    DLLSRC/Typedefs.cpp 
    DLLSRC/UPNP.cpp
)

target_compile_definitions(TCPCS PRIVATE TCPCS_EXPORTS)
target_compile_definitions(TCPCS PRIVATE UNICODE)
target_compile_definitions(TCPCS PRIVATE _UNICODE)

target_include_directories(TCPCS PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_link_directories(TCPCS PRIVATE ${CMAKE_SOURCE_DIR}/Common ${CMAKE_SOURCE_DIR}/Common/zlib/lib/x64)
#target_link_libraries(TCPCS PUBLIC Common)
target_link_libraries(TCPCS PRIVATE zdll)

target_precompile_headers(TCPCS PRIVATE
    "Stdafx.h"
)