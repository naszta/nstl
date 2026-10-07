cmake_minimum_required(VERSION 3.28)
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
if (POLICY CMP0148)
  cmake_policy(SET CMP0148 NEW)
endif()

PROJECT(nstlroot)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BOOST_ENABLE_CMAKE ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(cmake/FindNstl.cmake)
include(FetchContent)

set(NSTL_BOOST_VERSION boost-1.92.0 CACHE STRING "boost version")
set(NSTL_TBB_VERSION v2023.1.0 CACHE STRING "intel TBB version")
set(NSTL_GTEST_VERSION v1.18.0 CACHE STRING "GoogleTest version")
set(NSTL_CURL_VERSION 8.22.0 CACHE STRING "curl version")

set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
set(TBB_TEST OFF CACHE BOOL "" FORCE)
if(LINUX)
  find_package(Boost COMPONENTS headers serialization program_options tokenizer)
  find_package(CURL COMPONENTS HTTPS SSL)
  find_package(TBB REQUIRED)
  find_package(GTest REQUIRED)
else(LINUX)
  FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG ${NSTL_BOOST_VERSION}
    GIT_SHALLOW TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(Boost)
  message(STATUS "Boost version: ${NSTL_BOOST_VERSION}")

  FetchContent_Declare(
    TBB
    GIT_REPOSITORY https://github.com/uxlfoundation/oneTBB.git
    GIT_TAG ${NSTL_TBB_VERSION}
    GIT_SHALLOW TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(TBB)
  message(STATUS "intel TBB version: ${NSTL_TBB_VERSION}")

  FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG ${NSTL_GTEST_VERSION}
    GIT_SHALLOW TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(GTest)
  message(STATUS "Google Test version: ${NSTL_GTEST_VERSION}")

  if(APPLE)
    find_package(CURL COMPONENTS HTTPS SSL)
  else(APPLE)
    set(_CURL_URL "https://curl.se/download/curl-${NSTL_CURL_VERSION}.tar.gz")
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_STATIC_CURL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(CURL
      URL ${_CURL_URL}
      DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    FetchContent_MakeAvailable(CURL)
    message(STATUS "curl version: ${NSTL_CURL_VERSION}")
  endif(APPLE)
endif(LINUX)

# openssl
if (NSTL_USING_OPENSSL)
  find_package(OpenSSL REQUIRED)
endif()

# hh date
if (NOT NSTL_USING_STD_CHRONO)
  set(USE_SYSTEM_TZ_DB ON CACHE BOOL "")
  set(BUILD_TZ_LIB ON CACHE BOOL "")
  set(COMPILE_WITH_C_LOCALE ON CACHE BOOL "")

  add_subdirectory(modules/date)
endif()

if (MSVC)
  add_compile_options(/W4 /WX /Zc:__cplusplus)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1 NOMINMAX _WIN32_WINNT=0x0603 NTDDI_VERSION=NTDDI_WINBLUE)
else(MSVC)
  add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wmisleading-indentation -Wswitch-default -Wswitch-enum -Wold-style-cast -fstack-protector-strong)
  add_compile_definitions(_FORTIFY_SOURCE=2)
endif(MSVC)
add_compile_definitions(
  $<$<CONFIG:RELEASE>:NDEBUG>
)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_SUBDIRECTORY(nstl)
ADD_SUBDIRECTORY(bin)
ADD_SUBDIRECTORY(spf_resolver)

enable_testing()
ADD_SUBDIRECTORY(tests)
ADD_SUBDIRECTORY(bin/test)
