enable_testing()

find_package(GTest CONFIG REQUIRED)
add_executable(CNLIB_Tests main.cpp test_file.cpp)
target_link_libraries(CNLIB_Tests PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

target_include_directories(CNLIB_Tests PRIVATE ${CMAKE_SOURCE_DIR}/TCPCS/DLLSRC)
target_link_libraries(CNLIB_Tests PRIVATE TCPCS)

add_test(AllTestsInMain CNLIB_Tests)
