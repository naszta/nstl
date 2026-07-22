PROJECT(nstllib LANGUAGES CXX)

SET(nstllibHdrs
  nstl/c_timezone.hpp
  nstl/dns_tools.hpp
  nstl/macros.hpp
  nstl/range_print.hpp
  nstl/ro_buffer.hpp
  nstl/safe_basename.hpp
  nstl/scope_exit.hpp
  nstl/unlock_guard.hpp
)

SET(nstllibSrcs
  nstl/c_timezone.cpp
  nstl/dns_tools.cpp
)

if(WIN32)
  list(APPEND nstllibSrcs nstl/dns_tools_w32.cpp)
else()
  list(APPEND nstllibSrcs nstl/dns_tools_posix.cpp)
endif()

ADD_LIBRARY(nstllib ${nstllibHdrs} ${nstllibSrcs})

target_compile_features(nstllib PUBLIC cxx_std_20)
target_include_directories(nstllib PUBLIC ${nstllib_SOURCE_DIR})

if (WIN32)
  target_link_libraries(nstllib Ws2_32.lib Dnsapi.lib)
else()
  target_link_libraries(nstllib resolv)
endif()

set_target_properties(nstllib PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
