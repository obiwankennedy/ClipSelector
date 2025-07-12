cmake_minimum_required(VERSION 3.16)

project(ClipSelector VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(PkgConfig)
pkg_check_modules(TAGLIB REQUIRED IMPORTED_TARGET taglib)
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Multimedia)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(clipSelector
    main.cpp
    utils.h utils.cpp
)

add_library(audiorw SHARED IMPORTED) # or STATIC instead of SHARED
set_target_properties(audiorw PROPERTIES
  IMPORTED_LOCATION "/usr/lib/libaudiorw.so"
  INTERFACE_INCLUDE_DIRECTORIES "/usr/include"
)


qt_add_qml_module(clipSelector
    URI Views
    VERSION 1.0
    QML_FILES
        Main.qml
        SOURCES maincontroller.h maincontroller.cpp
        SOURCES clipmodel.h clipmodel.cpp
        QML_FILES VideoPlayer.qml
        SOURCES audiofilemodel.h audiofilemodel.cpp
        QML_FILES SubClipTable.qml
        QML_FILES MusicSelector.qml
        SOURCES filteredmodel.h filteredmodel.cpp
        QML_FILES OffsetSelector.qml
        SOURCES audiospect.h audiospect.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(clipSelector PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appCilpSelector
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(clipSelector
    PRIVATE Qt6::Quick Qt6::Multimedia PkgConfig::TAGLIB audiorw
)

include(GNUInstallDirs)
install(TARGETS clipSelector
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
