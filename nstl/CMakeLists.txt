PROJECT(nstllib LANGUAGES CXX)

if (NSTL_USING_OPENSSL)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif()

SET(nstllibHdrs
  nstl/base64.hpp
  nstl/c_timezone.hpp
  nstl/datahash_fwd.hpp
  nstl/datahash.hpp
  nstl/dead_lock_detect.hpp
  nstl/dns_tools.hpp
  nstl/env_var_raii.hpp
  nstl/exception.hpp
  nstl/global_init.hpp
  nstl/logging.hpp
  nstl/macros.hpp
  nstl/memory.hpp
  nstl/range_print.hpp
  nstl/ro_buffer.hpp
  nstl/safe_basename.hpp
  nstl/scope_exit.hpp
  nstl/signal_barrier.hpp
  nstl/string.hpp
  nstl/temp_dir.hpp
  nstl/unlock_guard.hpp
)

SET(nstllibSrcs
  nstl/c_timezone.cpp
  nstl/datahash.cpp
  nstl/dead_lock_detect.cpp
  nstl/dns_tools.cpp
  nstl/env_var_raii.cpp
  nstl/exception.cpp
  nstl/global_init.cpp
  nstl/logging.cpp
  nstl/signal_barrier.cpp
  nstl/temp_dir.cpp
)

if(WIN32)
  list(APPEND nstllibSrcs nstl/dns_tools_w32.cpp)
  add_compile_definitions(_CRT_NONSTDC_NO_DEPRECATE=1)
else()
  list(APPEND nstllibSrcs nstl/dns_tools_posix.cpp)
endif()

set(nstllib_private_dependencies
  TBB::tbb
)

if (NSTL_USING_CURL)
  list(APPEND nstllibHdrs nstl/http_client.hpp)
  list(APPEND nstllibSrcs nstl/http_client.cpp)
  add_compile_definitions(NSTL_USING_CURL)
  list(APPEND nstllib_private_dependencies CURL::libcurl)
endif()

set(nstllib_public_dependencies
  Boost::headers Boost::serialization
)

if (NSTL_USING_HH_DATE)
  list(APPEND nstllib_public_dependencies date::date date::date-tz)
  add_compile_definitions(NSTL_USING_HH_DATE)
endif()

if (NSTL_USING_OPENSSL)
  list(APPEND nstllibSrcs nstl/datahashopenssl.cpp)
  list(APPEND nstllib_private_dependencies OpenSSL::Crypto)
elseif(WIN32)
  list(APPEND nstllibSrcs nstl/datahashwin32.cpp)
  list(APPEND nstllib_private_dependencies Advapi32.lib)
else()
  message(FATAL_ERROR "Win32 API or OpenSSL required")
endif()

ADD_LIBRARY(nstllib ${nstllibHdrs} ${nstllibSrcs})

target_compile_features(nstllib PUBLIC cxx_std_20)
target_include_directories(nstllib PUBLIC ${nstllib_SOURCE_DIR} ${Boost_INCLUDE_DIRS})

if (WIN32)
  list(APPEND nstllib_private_dependencies Ws2_32.lib Dnsapi.lib)
else()
  list(APPEND nstllib_private_dependencies resolv)
endif()

target_link_libraries(nstllib PRIVATE ${nstllib_private_dependencies} PUBLIC ${nstllib_public_dependencies})

set_target_properties(nstllib PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
