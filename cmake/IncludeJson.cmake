# nlohmann json
set(NSTL_JSON_VERSION v3.12.0 CACHE STRING "nlohmann json version")

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG ${NSTL_JSON_VERSION}
  GIT_SHALLOW TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)
set(JSON_Install OFF CACHE INTERNAL "")
message(STATUS "Downloading nlohmann json version: ${NSTL_JSON_VERSION}")
FetchContent_MakeAvailable(nlohmann_json)
