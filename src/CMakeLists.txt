add_subdirectory(libs/core)
add_subdirectory(libs/views)
add_subdirectory(bin)

IF(BUILD_TEST)
  enable_testing()
  add_subdirectory(tests)
endif()

