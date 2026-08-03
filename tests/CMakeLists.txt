INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${nstllib_SOURCE_DIR}
)

SET(nstltestSrcs
  main.cpp
  c_timezone_tests.cpp
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
)

if (NSTL_USING_CURL)
  list(APPEND nstltestSrcs http_client_tests.cpp)
  add_compile_definitions(NSTL_USING_CURL)
endif()

if (NSTL_USING_HH_DATE)
  add_compile_definitions(NSTL_USING_HH_DATE)
endif()

ADD_EXECUTABLE(nstltest ${nstltestSrcs})

TARGET_LINK_LIBRARIES(nstltest nstllib GTest::gtest)

include(GoogleTest)
gtest_discover_tests(nstltest)

add_test(NAME nstltest COMMAND nstltest)
