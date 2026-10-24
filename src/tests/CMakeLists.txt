# Unit tests.
#
# tst_model compiles clipmodel + rangemodel directly (as before) instead of
# linking corelib, so it does not require taglib / audiorw to build.

find_package(Qt6 REQUIRED COMPONENTS Gui Test Quick Widgets)

qt_add_executable(tst_model
    tst_model.cpp
)

add_test(NAME tst_model COMMAND tst_model)

set_target_properties(tst_model PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_include_directories(tst_model PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/core/tutorial
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/core/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/core/model
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/core/data
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/core/utils
)

target_link_libraries(tst_model
    PRIVATE Qt6::Test Qt6::Gui Qt6::Quick Qt6::Widgets corelib
)

# tst_tutorial exercises the TUTORIAL mode logic (comments, SRT, TTS command
# building, render command building, project persistence). It links the whole
# corelib so it needs TagLib / audiorw (already located at the root).
qt_add_executable(tst_tutorial
    tst_tutorial.cpp
)

add_test(NAME tst_tutorial COMMAND tst_tutorial)

set_target_properties(tst_tutorial PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

target_include_directories(tst_tutorial PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/core/tutorial
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/core/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/core/model
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/core/data
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/core/utils

)

target_link_libraries(tst_tutorial
    PRIVATE views corelib Qt6::Test Qt6::Quick
)

