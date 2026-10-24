# "Views" QML module.
#
# Owns QML type registration for the QML_ELEMENT core classes (the .cpp
# implementations come from the corelib STATIC library, linked in below). The QML files live in ./qml; Main.qml lives in ../../bin
# and is registered here (by absolute path) so the executable's
# loadFromModule("Views", "Main") keeps working.
#
# NOTE: if you add a C++ class deriving from QQuickItem in ./component, add
# its .h/.cpp to the SOURCES list below so it is moc'd and (optionally)
# registered with QML_ELEMENT here.

# Main.qml lives in an "extra" directory (bin/) relative to this module; the
# generated qmldir registers it explicitly, so acknowledge QTP0004.
qt_policy(SET QTP0004 NEW)

# Main.qml lives outside the module directory; Qt requires a resource alias for
# absolute-path QML files so it is reachable as "Main.qml" in the resource tree.
set_source_files_properties(${CMAKE_SOURCE_DIR}/src/bin/Main.qml
    PROPERTIES QT_RESOURCE_ALIAS "Main.qml")

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
        QML_FILES qml/TimeLine.qml
        QML_FILES qml/EditToolBar.qml
        QML_FILES qml/EditorContent.qml
)

# Pull in the compiled implementations of the core classes (and their moc).
target_link_libraries(views
    PUBLIC Core
)
