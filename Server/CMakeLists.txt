find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

#AUTOUIC is a boolean specifying whether CMake will handle the Qt uic code generator automatically, i.e. without having to use commands like QT4_WRAP_UI(), QT5_WRAP_UI(), etc. Currently, Qt versions 4 to 6 are supported.
set(CMAKE_AUTOUIC ON)
#AUTOMOC is a boolean specifying whether CMake will handle the Qt moc preprocessor automatically, i.e. without having to use commands like QT4_WRAP_CPP(), QT5_WRAP_CPP(), etc. Currently, Qt versions 4 to 6 are supported.
set(CMAKE_AUTOMOC ON)
#AUTORCC is a boolean specifying whether CMake will handle the Qt rcc code generator automatically, i.e. without having to use commands like QT4_ADD_RESOURCES(), QT5_ADD_RESOURCES(), etc. Currently, Qt versions 4 to 6 are supported.
set(CMAKE_AUTORCC ON)

set(PROJECT_SOURCES
    StdAfx.cpp
    main.cpp
    mainwindow.h mainwindow.cpp mainwindow.ui
    optionsdialog.h optionsdialog.cpp optionsdialog.ui
    manageadminsdialog.h manageadminsdialog.cpp manageadminsdialog.ui
    serverinfo.h serverinfo.cpp
    #Whiteboard.h
    #Whiteboard.cpp
    ../Common/Mouse.cpp
    ../Common/Palette.cpp
    ../Common/WhiteboardClientData.cpp
    ../Common/WinFirewall.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(server
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
  )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET server APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(server SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(server
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(server PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.server)
endif()
set_target_properties(server PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS server
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set_target_properties(server PROPERTIES WIN32_EXECUTABLE TRUE)

target_precompile_headers(server PRIVATE
    StdAfx.h
)

target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/MyUtils/)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/TCPCS/DLLSRC)
target_link_libraries(server PRIVATE TCPCS)
target_link_libraries(server PRIVATE MyUtils)
target_link_libraries(server PRIVATE comctl32)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(server)
endif()

