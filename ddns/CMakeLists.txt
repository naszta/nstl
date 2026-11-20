INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${nstllib_SOURCE_DIR}
)

SET(ddnsHdrs
  config.hpp
  nft.hpp
  worker.hpp
)

SET(ddnsSrcs
  config.cpp
  nft.cpp
  worker.cpp
)

add_library(nstlddnslib STATIC ${ddnsSrcs} ${ddnsHdrs})
target_compile_features(nstlddnslib PUBLIC cxx_std_20)
target_include_directories(nstlddnslib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(nstlddns main.cpp)

target_compile_features(nstlddns PUBLIC cxx_std_20)

set_target_properties(nstlddns PROPERTIES
  VERSION ${nstlroot_VERSION}
  POSITION_INDEPENDENT_CODE ON
)

set(nstlddns_dependencies
  nstllib
  nlohmann_json::nlohmann_json
  Boost::headers
  Boost::program_options
)

if(LINUX)
  list(APPEND nstlddns_dependencies nftables)
endif()

TARGET_LINK_LIBRARIES(nstlddnslib PUBLIC ${nstlddns_dependencies})
TARGET_LINK_LIBRARIES(nstlddns PRIVATE nstlddnslib)

ADD_SUBDIRECTORY(test)

install(TARGETS nstlddns DESTINATION bin)
