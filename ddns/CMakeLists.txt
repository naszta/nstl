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
  main.cpp
  nft.cpp
  worker.cpp
)

add_executable(nstlddns ${ddnsSrcs} ${ddnsHdrs})

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

TARGET_LINK_LIBRARIES(nstlddns PRIVATE ${nstlddns_dependencies})

install(TARGETS nstlddns DESTINATION bin)
