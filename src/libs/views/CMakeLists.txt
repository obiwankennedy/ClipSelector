find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Qml Multimedia)

qt_policy(SET QTP0004 NEW)

set(ASSET_ROOT "../../../assets")

qt_standard_project_setup(REQUIRES 6.11)

qt_policy(SET QTP0001 NEW)

qt_add_qml_module(views
    URI Views
    VERSION 1.0
    IMPORTS
        Core
    QML_FILES
        qml/VideoPlayer.qml
        qml/SubClipTable.qml
        qml/MusicSelector.qml
        qml/OffsetSelector.qml
        qml/SettingsContent.qml
        qml/TutorialView.qml
        qml/TimeLinePanel.qml
        qml/EffectSidePanel.qml
        qml/EditToolBar.qml
        qml/TopToolBar.qml
        qml/EditorContent.qml
    RESOURCES
        ${ASSET_ROOT}/icons/add.svg
        ${ASSET_ROOT}/icons/clock.svg
        ${ASSET_ROOT}/icons/cutter.svg
        ${ASSET_ROOT}/icons/fast-forward.svg
        ${ASSET_ROOT}/icons/home.svg
        ${ASSET_ROOT}/icons/music.svg
        ${ASSET_ROOT}/icons/next.svg
        ${ASSET_ROOT}/icons/padlock.svg
        ${ASSET_ROOT}/icons/pause.svg
        ${ASSET_ROOT}/icons/play.svg
        ${ASSET_ROOT}/icons/record.svg
        ${ASSET_ROOT}/icons/refresh.svg
        ${ASSET_ROOT}/icons/remove.svg
        ${ASSET_ROOT}/icons/render.svg
        ${ASSET_ROOT}/icons/settings.svg
        ${ASSET_ROOT}/icons/shutdown.svg
        ${ASSET_ROOT}/icons/speaker.svg
        ${ASSET_ROOT}/icons/stop.svg
        ${ASSET_ROOT}/icons/highlighter.svg
        ${ASSET_ROOT}/icons/highlighter2.svg
        SOURCES
        src/timelinetokenbase.h
        src/timelinetokenbase.cpp
        src/timelinetoken.h
        src/timelinetoken.cpp
        src/timelineeffecttoken.h
        src/timelineeffecttoken.cpp
            node/timelinenode.h
            node/timelinenode.cpp
            QML_FILES qml/TimeLineBackGround.qml
)

target_link_libraries(views PRIVATE Core Qt6::Quick Qt6::QuickControls2 Qt6::Qml Qt6::Multimedia)

target_include_directories(views PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/node
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)