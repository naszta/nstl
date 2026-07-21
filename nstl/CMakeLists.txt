PROJECT(nstllib LANGUAGES CXX)

SET(nstllibHdrs
  nstl/c_timezone.hpp
  nstl/range_print.hpp
  nstl/ro_buffer.hpp
  nstl/scope_exit.hpp
  nstl/unlock_guard.hpp
)

SET(nstllibSrcs
  nstl/c_timezone.cpp
)

ADD_LIBRARY(nstllib ${nstllibHdrs} ${nstllibSrcs})

target_compile_features(nstllib PUBLIC cxx_std_20)
target_include_directories(nstllib PUBLIC ${nstllib_SOURCE_DIR})

set_target_properties(nstllib PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
