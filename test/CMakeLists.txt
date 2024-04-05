cmake_minimum_required(VERSION 3.22)

project(test_ole
        DESCRIPTION "Test ole module"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED 23)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

find_package(PkgConfig)
pkg_search_module(gsf REQUIRED IMPORTED_TARGET libgsf-1)

add_executable(test_ole2 test_read_file.cpp)

find_package(GTest REQUIRED)
target_link_libraries(test_ole2 PRIVATE ole GTest::gtest_main)
########################################################################################
# These fixtures should be used as follows:
# set_tests_properties(read_ole PROPERTIES FIXTURES_REQUIRED "READ_SETUP1;READ_SETUP2")
########################################################################################
# create dirs for using by gsf
add_test(NAME read_setup1
         COMMAND ${CMAKE_COMMAND} -E tar x ${CMAKE_SOURCE_DIR}/test/test.tgz)
set_tests_properties(read_setup1 PROPERTIES FIXTURES_SETUP READ_SETUP1)

# create ole file using gsf
add_test(NAME read_setup2
         COMMAND ${gsf_PREFIX}/bin/gsf createole test.ole root)
set_tests_properties(read_setup2 PROPERTIES
                     FIXTURES_REQUIRED READ_SETUP1
                     FIXTURES_SETUP READ_SETUP2)

# remove ole file after using
add_test(NAME read_cleanup2
         COMMAND ${CMAKE_COMMAND} -E rm test.ole)
set_tests_properties(read_cleanup2 PROPERTIES
                     FIXTURES_REQUIRED READ_SETUP1
                     FIXTURES_CLEANUP READ_SETUP2)

# remove dirs
add_test(NAME read_cleanup1
         COMMAND ${CMAKE_COMMAND} -E rm -r ./root)
set_tests_properties(read_cleanup1 PROPERTIES FIXTURES_CLEANUP READ_SETUP1)

########################################################################################