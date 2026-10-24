find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Qml Multimedia)

qt_standard_project_setup()

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
        tutorial/ttsengine.cpp
        tutorial/ttsengine.h
        tutorial/subtitlegenerator.cpp
        tutorial/subtitlegenerator.h
        tutorial/videorenderer.cpp
        tutorial/videorenderer.h
        tutorial/tutorialcontroller.cpp
        utils/utils.cpp
        utils/utils.h
)

# The sources use bare #include "clipmodel.h" across subdirectories, so expose
# every subdirectory on the include path.
target_include_directories(Core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    ${CMAKE_CURRENT_SOURCE_DIR}/model
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorial
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
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
