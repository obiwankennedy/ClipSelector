cmake_minimum_required(VERSION 3.16)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Qml Multimedia Test)


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(QT_QML_NO_CACHEGEN ON CACHE BOOL "Disable QML bytecode caching (qmlcachegen)")


find_package(PkgConfig)
pkg_check_modules(TAGLIB REQUIRED IMPORTED_TARGET taglib)

add_library(audiorw SHARED IMPORTED) # or STATIC instead of SHARED
set_target_properties(audiorw PROPERTIES
  IMPORTED_LOCATION "/usr/lib/libaudiorw.so"
  INTERFACE_INCLUDE_DIRECTORIES "/usr/include"
)

qt_standard_project_setup(REQUIRES 6.11)

qt_policy(SET QTP0001 NEW)

qt_add_qml_module(Core
    URI Core
    VERSION 1.0
    SOURCES
        controllers/maincontroller.cpp
        controllers/maincontroller.h
        data/audiospect.cpp
        data/audiospect.h
        model/clipmodel.cpp
        model/clipmodel.h
        model/audiofilemodel.cpp
        model/audiofilemodel.h
        model/filteredmodel.cpp
        model/filteredmodel.h
        model/rangemodel.cpp
        model/rangemodel.h


        tutorial/tutorialcomment.cpp
        tutorial/videoeffect.cpp
        tutorial/videoeffect.h
        tutorial/effectplayback.cpp
        tutorial/effectplayback.h
        tutorial/ttsengine.cpp
        tutorial/ttsengine.h
        tutorial/subtitlegenerator.cpp
        tutorial/subtitlegenerator.h
        tutorial/videorenderer.cpp
        tutorial/videorenderer.h
        controllers/tutorialcontroller.cpp
        controllers/tutorialcontroller.h
        utils/utils.cpp
        utils/utils.h
        controllers/videocontroller.h controllers/videocontroller.cpp
        controllers/musiccontroller.h controllers/musiccontroller.cpp
        controllers/rendercontroller.h controllers/rendercontroller.cpp
        controllers/projectcontroller.h controllers/projectcontroller.cpp
        SOURCES model/recentfilesmodel.h model/recentfilesmodel.cpp
        SOURCES data/timedevent.h data/timedevent.cpp
)

# The sources use bare #include "clipmodel.h" across subdirectories, so expose
# every subdirectory on the include path.
target_include_directories(Core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    ${CMAKE_CURRENT_SOURCE_DIR}/model
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorial
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/controllers>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/data>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/model>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/tutorial>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)



target_link_libraries(Core PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Test
    PkgConfig::TAGLIB
    audiorw
)
