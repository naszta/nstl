INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${nstllib_SOURCE_DIR}
)

ADD_EXECUTABLE(nstltest c_timezone_tests.cpp range_print_tests.cpp ro_buffer_tests.cpp scope_exit_tests.cpp unlock_guard_tests.cpp safe_basename_tests.cpp dns_tools_tests.cpp)

TARGET_LINK_LIBRARIES(nstltest nstllib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(nstltest)

add_test(NAME nstltest COMMAND nstltest)
