INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${nstllib_SOURCE_DIR}
)

add_executable(multi_test main_test.cpp)

target_compile_features(multi_test PUBLIC cxx_std_20)
TARGET_LINK_LIBRARIES(multi_test nstllib)
