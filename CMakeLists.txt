cmake_minimum_required(VERSION 3.28)
PROJECT(nstlroot)

SET(nstlroot_MAJOR_VERSION 0)
SET(nstlroot_MINOR_VERSION 1)
SET(nstlroot_BUILD_VERSION 0)

IF(EXISTS ${nstlroot_SOURCE_DIR}/.git)
  FIND_PACKAGE(Git)
  IF(GIT_FOUND)
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      WORKING_DIRECTORY "${nstlroot_SOURCE_DIR}"
      OUTPUT_VARIABLE "nstlroot_BUILD_VERSION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    MESSAGE( STATUS "Git version: ${nstlroot_BUILD_VERSION}" )
  ENDIF(GIT_FOUND)
ENDIF(EXISTS ${nstlroot_SOURCE_DIR}/.git)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(gtest_version "v1.17.0" CACHE STRING "GTest version")

include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/refs/tags/${gtest_version}.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

if (MSVC)
  add_compile_options(/W4 /WX /Zc:__cplusplus)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1 NOMINMAX _WIN32_WINNT=0x0603 NTDDI_VERSION=NTDDI_WINBLUE)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()
add_compile_definitions(
  $<$<CONFIG:RELEASE>:NDEBUG>
)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_SUBDIRECTORY(nstl)

enable_testing()
ADD_SUBDIRECTORY(tests)
