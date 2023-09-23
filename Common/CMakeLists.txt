#add_library(Common STATIC
#    Stdafx.cpp
#    Mouse.cpp 
#    Palette.cpp 
#    WhiteboardClientData.cpp 
#    WinFirewall.cpp
#)
#
#target_compile_definitions(Common PRIVATE TCPCS_EXPORTS)
#target_include_directories(Common PUBLIC CNLIB)
#
#target_precompile_headers(Common PRIVATE
#    "Stdafx.h"
#)