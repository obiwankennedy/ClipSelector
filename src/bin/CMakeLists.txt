find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Qml Multimedia)

qt_standard_project_setup()

qt_policy(SET QTP0001 NEW)

qt_add_executable(clipSelector
    main.cpp
)

target_link_libraries(clipSelector
    PRIVATE     Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Multimedia Core views
)

qt_add_qml_module(clipSelector
    URI ClipApp
    VERSION 1.0
    IMPORTS
        Core
        Views
    QML_FILES
        Main.qml
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

qt_import_qml_plugins(clipSelector)

include(GNUInstallDirs)
install(TARGETS clipSelector
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
