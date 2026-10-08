set(TBB_TEST OFF CACHE BOOL "" FORCE)

find_package(TBB QUIET)
if (TBB_FOUND)
  message(STATUS "TBB found locally")
else()
  set(NSTL_TBB_VERSION v2023.1.0 CACHE STRING "intel TBB version")
  message(STATUS "Downloading oneTBB version: ${NSTL_TBB_VERSION}")
  FetchContent_Declare(
    TBB
    GIT_REPOSITORY https://github.com/uxlfoundation/oneTBB.git
    GIT_TAG ${NSTL_TBB_VERSION}
    GIT_SHALLOW TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(TBB)
endif()
