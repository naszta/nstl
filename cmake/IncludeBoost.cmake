set(BOOST_ENABLE_CMAKE ON)
find_package(Boost COMPONENTS headers serialization program_options tokenizer)
if (Boost_FOUND)
  message(STATUS "Boost found locally")
else()
  set(NSTL_BOOST_VERSION boost-1.92.0 CACHE STRING "boost version")
  message(STATUS "Downloading Boost version: ${NSTL_BOOST_VERSION}")
  FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG ${NSTL_BOOST_VERSION}
    GIT_SHALLOW TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(Boost)
endif()
