add_executable(client
    StdAfx.cpp
    Options.cpp 
    Whiteboard.cpp
    Windows.cpp
    ../Common/Mouse.cpp 
    ../Common/Palette.cpp 
    ../Common/WhiteboardClientData.cpp 
    ../Common/WinFirewall.cpp
    Resource.rc
)

set_target_properties(client PROPERTIES WIN32_EXECUTABLE TRUE)

target_precompile_headers(client PRIVATE
    StdAfx.h
)

#target_compile_definitions(client PRIVATE UNICODE)
#target_compile_definitions(client PRIVATE _UNICODE)

#target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/Common)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/TCPCS/DLLSRC)
target_link_libraries(client PRIVATE TCPCS)