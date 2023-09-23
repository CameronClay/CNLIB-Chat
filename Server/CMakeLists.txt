add_executable(server 
    StdAfx.cpp
    Whiteboard.cpp
    Windows.cpp    
    ../Common/Mouse.cpp 
    ../Common/Palette.cpp 
    ../Common/WhiteboardClientData.cpp 
    ../Common/WinFirewall.cpp
    Resource.rc
)

set_target_properties(server PROPERTIES WIN32_EXECUTABLE TRUE)

target_precompile_headers(server PRIVATE
    StdAfx.h
)

target_compile_definitions(server PRIVATE UNICODE)
target_compile_definitions(server PRIVATE _UNICODE)

target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/TCPCS/DLLSRC)
target_link_libraries(server PRIVATE TCPCS)