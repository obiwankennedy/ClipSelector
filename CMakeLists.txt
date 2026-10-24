cmake_minimum_required(VERSION 3.16)

project(ClipSelector VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig)
pkg_check_modules(TAGLIB REQUIRED IMPORTED_TARGET taglib)
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Multimedia Test)

qt_standard_project_setup(REQUIRES 6.5)

# QML bytecode caching (qmlcachegen) is a performance optimization. Disable it
# by default in this environment because the installed qmlcachegen has an ABI
# mismatch ("undefined symbol: ... Qt_6_PRIVATE_API"). Override with
# -DQT_QML_NO_CACHEGEN=OFF once the Qt installation is repaired.
set(QT_QML_NO_CACHEGEN ON CACHE BOOL "Disable QML bytecode caching (qmlcachegen)")

# audiorw is a system library (no CMake config), so expose it as an imported target.
add_library(audiorw SHARED IMPORTED) # or STATIC instead of SHARED
set_target_properties(audiorw PROPERTIES
  IMPORTED_LOCATION "/usr/lib/libaudiorw.so"
  INTERFACE_INCLUDE_DIRECTORIES "/usr/include"
)

enable_testing()

add_subdirectory(src)

