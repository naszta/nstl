set(ENABLE_CURL_MANUAL OFF CACHE BOOL "" FORCE)
set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)

find_package(CURL COMPONENTS HTTPS SSL)
if (CURL_FOUND)
  message(STATUS "curl found locally")
else()
  set(NSTL_CURL_VERSION 8.22.0 CACHE STRING "curl version")
  set(_CURL_URL "https://curl.se/download/curl-${NSTL_CURL_VERSION}.tar.gz")
  set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
  set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
  set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_STATIC_CURL ON CACHE BOOL "" FORCE)
  message(STATUS "Downloading curl version: ${NSTL_CURL_VERSION}")
  FetchContent_Declare(CURL
    URL ${_CURL_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP true
  )
  FetchContent_MakeAvailable(CURL)
endif()
