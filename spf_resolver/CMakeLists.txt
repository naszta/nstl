INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${nstllib_SOURCE_DIR}
)

add_executable(spf_resolver main.cpp)

target_compile_features(spf_resolver PUBLIC cxx_std_20)

set_target_properties(spf_resolver PROPERTIES
  VERSION ${nstlroot_VERSION}
  POSITION_INDEPENDENT_CODE ON
)

TARGET_LINK_LIBRARIES(spf_resolver nstllib Boost::headers Boost::program_options)

install(TARGETS spf_resolver DESTINATION bin)
