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
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(cmake/FindNstl.cmake)
include(FetchContent)
include(cmake/IncludeBoost.cmake)
include(cmake/IncludeTbb.cmake)
include(cmake/IncludeCurl.cmake)
include(cmake/IncludeGTest.cmake)

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
