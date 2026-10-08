find_package(GTest QUIET)
if (GTest_FOUND)
  message(STATUS "Googletest found locally")
else()
  set(NSTL_GTEST_VERSION v1.18.0 CACHE STRING "GoogleTest version")
  message(STATUS "Downloading Googletest version: ${NSTL_GTEST_VERSION}")
  FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG ${NSTL_GTEST_VERSION}
    GIT_SHALLOW TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(GTest)
endif()
