INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${nstllib_SOURCE_DIR}
)

SET(nstlddnstestSrcs
  ${CMAKE_SOURCE_DIR}/tests/main.cpp
  config_tests.cpp
  nft_tests.cpp
  worker_tests.cpp
)

ADD_EXECUTABLE(nstlddnstest ${nstlddnstestSrcs})

target_compile_features(nstlddnstest PUBLIC cxx_std_20)

TARGET_LINK_LIBRARIES(nstlddnstest nstlddnslib GTest::gtest Boost::headers Boost::program_options)

include(GoogleTest)
gtest_discover_tests(nstlddnstest)

add_test(NAME nstlddnstest COMMAND nstlddnstest)
