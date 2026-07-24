add_executable(ClipWordStoreTest
  ClipWordStoreTest.cpp
)

target_link_libraries(ClipWordStoreTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

target_include_directories(ClipWordStoreTest PRIVATE
  ${REPO_ROOT}/src
  ${REPO_ROOT}/lib/EpdFont
)

gtest_discover_tests(ClipWordStoreTest)
