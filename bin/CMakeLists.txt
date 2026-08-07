INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${nstllib_SOURCE_DIR}
)

if (APPLE)
  add_executable(multi_runner main_macos.cpp)
elseif (WIN32)
  add_executable(multi_runner main_win32.cpp)
else()
  add_executable(multi_runner main_linux.cpp)
endif()

target_compile_features(multi_runner PUBLIC cxx_std_20)

set_target_properties(multi_runner PROPERTIES
  VERSION ${nstlroot_VERSION}
  POSITION_INDEPENDENT_CODE ON
)

TARGET_LINK_LIBRARIES(multi_runner nstllib)

install(TARGETS multi_runner DESTINATION bin)
