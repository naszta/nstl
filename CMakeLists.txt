cmake_minimum_required(VERSION 3.28)
PROJECT(nstlroot)

SET(nstlroot_MAJOR_VERSION 0)
SET(nstlroot_MINOR_VERSION 3)
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

set(NSTL_USING_CURL ON CACHE BOOL "Adding curl module")

include(FetchContent)

if (NSTL_USING_CURL)
  set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
  set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
  if (WIN32)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_CURL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(CURL
	    URL https://curl.se/download/curl-8.21.0.tar.gz
	    DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    FetchContent_MakeAvailable(CURL)
  else()
    find_package(CURL COMPONENTS HTTPS SSL)
  endif()
endif()

if (WIN32)
  set(NSTL_USING_OPENSSL OFF CACHE BOOL "")
else()
  set(NSTL_USING_OPENSSL ON CACHE BOOL "")
endif()

if (APPLE)
  set(NSTL_USING_HH_DATE ON CACHE BOOL "")
else()
  set(NSTL_USING_HH_DATE OFF CACHE BOOL "")
endif()

# openssl
if (NSTL_USING_OPENSSL)
  find_package(OpenSSL REQUIRED)
endif()

# tbb
find_package(TBB REQUIRED)
set(TBB_TEST OFF CACHE BOOL "" FORCE)

# gtest
find_package(GTest REQUIRED)

# hh date
if (NSTL_USING_HH_DATE)
  set(USE_SYSTEM_TZ_DB ON CACHE BOOL "")
  set(BUILD_TZ_LIB ON CACHE BOOL "")
  set(COMPILE_WITH_C_LOCALE ON CACHE BOOL "")

  add_subdirectory(modules/date)
endif()

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
  ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_SUBDIRECTORY(nstl)

enable_testing()
ADD_SUBDIRECTORY(tests)
