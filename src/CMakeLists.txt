# Aggregates the application targets:
#   libs/core  -> STATIC library with all C++ logic
#   libs/views -> QML module "Views"
#   bin        -> the executable
#   tests      -> unit tests
add_subdirectory(libs/core)
add_subdirectory(libs/views)
add_subdirectory(bin)
#add_subdirectory(tests)
