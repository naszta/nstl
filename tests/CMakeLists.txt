enable_testing()

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${nstllib_SOURCE_DIR}
)

ADD_EXECUTABLE(nstltest range_print_tests.cpp ro_buffer_tests.cpp unlock_guard_tests.cpp)

if(MSVC)
  target_compile_options(nstltest PRIVATE /W4 /WX)
  target_compile_definitions(nstltest PUBLIC _CRT_SECURE_NO_WARNINGS=1)
else()
  target_compile_options(nstltest PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

TARGET_LINK_LIBRARIES(nstltest nstllib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(nstltest)

add_test(NAME nstltest COMMAND nstltest)
