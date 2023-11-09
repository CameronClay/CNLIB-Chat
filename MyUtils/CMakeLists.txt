find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_library(MyUtils STATIC
    DragStringListModel.cpp DragStringListModel.h
    FuncUtils.h
    whiteboardargs.h
)

target_link_libraries(MyUtils PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

