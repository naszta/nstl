PROJECT(nstllib LANGUAGES CXX)

SET(nstllibHdrs
  nstl/range_print.hpp
  nstl/ro_buffer.hpp
  nstl/scope_exit.hpp
  nstl/unlock_guard.hpp
)

ADD_LIBRARY(nstllib INTERFACE)

# ref: https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#interface-libraries
target_sources(nstllib PUBLIC
  FILE_SET HEADERS # useless line
  BASE_DIRS nstl # same as target_include_directories
  FILES ${nstllibHdrs} # useless line
)
