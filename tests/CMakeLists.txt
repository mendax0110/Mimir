include(GoogleTest)

add_executable(test_dag test_dag.cpp)
target_link_libraries(test_dag PRIVATE libmimir GTest::gtest_main)
gtest_discover_tests(test_dag)

add_executable(test_signature test_signature.cpp)
target_link_libraries(test_signature PRIVATE libmimir GTest::gtest_main)
gtest_discover_tests(test_signature)

add_executable(test_cache test_cache.cpp)
target_link_libraries(test_cache PRIVATE libmimir GTest::gtest_main)
gtest_discover_tests(test_cache)

add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE libmimir GTest::gtest_main)
gtest_discover_tests(test_parser)

add_executable(test_executor test_executor.cpp)
target_link_libraries(test_executor PRIVATE libmimir GTest::gtest_main GTest::gmock)
gtest_discover_tests(test_executor)

add_executable(test_command_runner test_command_runner.cpp)
target_link_libraries(test_command_runner PRIVATE libmimir GTest::gtest_main GTest::gmock)
gtest_discover_tests(test_command_runner)

add_executable(test_target test_target.cpp)
target_link_libraries(test_target PRIVATE libmimir GTest::gtest_main)
gtest_discover_tests(test_target)
