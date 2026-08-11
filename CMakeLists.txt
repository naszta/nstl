cmake_minimum_required(VERSION 3.28)
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()
if (POLICY CMP0148)
  cmake_policy(SET CMP0148 NEW)
endif()

PROJECT(nstlroot)

SET(nstlroot_MAJOR_VERSION 0)
SET(nstlroot_MINOR_VERSION 4)
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

SET(nstlroot_VERSION "${nstlroot_MAJOR_VERSION}.${nstlroot_MINOR_VERSION}.${nstlroot_BUILD_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BOOST_ENABLE_CMAKE ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)

if (WIN32)
  set(NSTL_USING_OPENSSL OFF CACHE BOOL "")
else()
  set(NSTL_USING_OPENSSL ON CACHE BOOL "")
endif()

try_compile(NSTL_USING_STD_CHRONO
  ${CMAKE_CURRENT_BINARY_DIR}/temp
  ${CMAKE_CURRENT_SOURCE_DIR}/modules/try_compile/chrono_has_tzone.cpp
  OUTPUT_VARIABLE NSTL_USING_STD_CHRONO_OUT)

MESSAGE(STATUS "Using C++20 std::chrono: ${NSTL_USING_STD_CHRONO}")

set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
set(TBB_TEST OFF CACHE BOOL "" FORCE)
if(LINUX)
  find_package(Boost COMPONENTS headers serialization program_options)
  find_package(CURL COMPONENTS HTTPS SSL)
  find_package(TBB REQUIRED)
  find_package(GTest REQUIRED)
else(LINUX)
  FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG boost-1.91.0-1
    GIT_SHALLOW TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(Boost)

  FetchContent_Declare(
    TBB
    GIT_REPOSITORY https://github.com/uxlfoundation/oneTBB.git
    GIT_TAG v2023.1.0
    GIT_SHALLOW TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(TBB)

  FetchContent_Declare(
    GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    GIT_SHALLOW TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(GTest)

  if(APPLE)
    find_package(CURL COMPONENTS HTTPS SSL)
  else(APPLE)
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

  add_compile_definitions(NSTL_USING_HH_DATE)
  add_subdirectory(modules/date)
endif()

if (MSVC)
  add_compile_options(/W4 /WX /Zc:__cplusplus)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1 NOMINMAX _WIN32_WINNT=0x0603 NTDDI_VERSION=NTDDI_WINBLUE)
else(MSVC)
  add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wmisleading-indentation -Wswitch-default -Wswitch-enum -Wold-style-cast)
endif(MSVC)
add_compile_definitions(
  $<$<CONFIG:RELEASE>:NDEBUG>
)

if(LINUX)
  try_compile(HAVE_BACKTRACE ${CMAKE_CURRENT_BINARY_DIR}/temp ${CMAKE_CURRENT_SOURCE_DIR}/modules/try_compile/backtrace_is_around.cpp LINK_LIBRARIES backtrace OUTPUT_VARIABLE HAVE_BACKTRACE_OUT)
else(LINUX)
  set(HAVE_BACKTRACE OFF)
  set(HAVE_BACKTRACE_OUT "Feature is Linux only")
endif(LINUX)
if (HAVE_BACKTRACE)
  message(STATUS "HAVE_BACKTRACE: ${HAVE_BACKTRACE}")
else(HAVE_BACKTRACE)
  message(STATUS "HAVE_BACKTRACE: ${HAVE_BACKTRACE} - ${HAVE_BACKTRACE_OUT}")
endif(HAVE_BACKTRACE)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_SUBDIRECTORY(nstl)
ADD_SUBDIRECTORY(bin)

enable_testing()
ADD_SUBDIRECTORY(tests)
ADD_SUBDIRECTORY(bin/test)
