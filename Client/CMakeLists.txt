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
    connectdialog.h connectdialog.cpp connectdialog.ui
    manageserversdialog.h manageserversdialog.cpp manageserversdialog.ui
    optionsdialog.h optionsdialog.cpp optionsdialog.ui
    viewlogdialog.h viewlogdialog.cpp viewlogdialog.ui
    readlogdialog.h readlogdialog.cpp readlogdialog.ui
    authenticatedialog.h authenticatedialog.cpp authenticatedialog.ui
    whiteboard.h whiteboard.cpp whiteboard.ui
    startwhiteboarddialog.h startwhiteboarddialog.cpp startwhiteboarddialog.ui
    invitewhiteboarddialog.h invitewhiteboarddialog.cpp invitewhiteboarddialog.ui

    imginfo.h imginfo.cpp
    whiteboardinviteparams.h whiteboardinviteparams.cpp
    clientinfo.h clientinfo.cpp
    Options.h Options.cpp
    ../Common/WinFirewall.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(client
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET client APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(client SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(client
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(client PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.client)
endif()
set_target_properties(client PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS client
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set_target_properties(client PROPERTIES WIN32_EXECUTABLE TRUE)

target_precompile_headers(client PRIVATE
    StdAfx.h
)

target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/MyUtils/)
target_include_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/TCPCS/DLLSRC)
target_link_libraries(client PRIVATE TCPCS)
target_link_libraries(client PRIVATE MyUtils)
target_link_libraries(client PRIVATE comctl32)
target_link_libraries(client PRIVATE shlwapi)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(client)
endif()
