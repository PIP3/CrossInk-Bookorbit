add_executable(catalog_models_test
  CatalogModelsTest.cpp
)

target_link_libraries(catalog_models_test
  crosspoint_test_common
  gtest_main
)

include(GoogleTest)
gtest_discover_tests(catalog_models_test)
