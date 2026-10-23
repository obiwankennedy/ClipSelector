cmake_minimum_required(VERSION 3.21)

project(test VERSION 0.1 LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Gui Test Quick Widgets)

qt_standard_project_setup()

qt_add_executable(tst_model
    tst_model.cpp
    ../clipmodel.cpp
    ../clipmodel.h
    ../rangemodel.cpp
    ../rangemodel.h
)
add_test(NAME tst_model COMMAND tst_model)


set_target_properties(tst_model PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)
target_include_directories(tst_model PRIVATE ../)

target_link_libraries(tst_model
    PRIVATE Qt6::Test Qt6::Gui Qt6::Quick Qt6::Widgets
)
