INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${nstllib_SOURCE_DIR}
)

SET(nstltestSrcs
  main.cpp
  args_editor_tests.cpp
  backtrace_tests.cpp
  parse_tests.cpp
  c_timezone_tests.cpp
  exception_tests.cpp
  logging_tests.cpp
  range_print_tests.cpp
  ro_buffer_tests.cpp
  scope_exit_tests.cpp
  unlock_guard_tests.cpp
  safe_basename_tests.cpp
  dns_tools_tests.cpp
  temp_dir_tests.cpp
  string_tests.cpp
  datahash_tests.cpp
  env_var_raii_tests.cpp
  dead_lock_detect_tests.cpp
  macros_tests.cpp
  signal_barrier_tests.cpp
  memory_tests.cpp
  base64_tests.cpp
  vector_tests.cpp
  http_client_tests.cpp
)

ADD_EXECUTABLE(nstltest ${nstltestSrcs})

TARGET_LINK_LIBRARIES(nstltest nstllib GTest::gtest Boost::headers Boost::program_options)

include(GoogleTest)
gtest_discover_tests(nstltest)

add_test(NAME nstltest COMMAND nstltest)
