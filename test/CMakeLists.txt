cmake_minimum_required(VERSION 3.22)

project(test_containerfs
        DESCRIPTION "Test ole module"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED 23)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

# First try to find libgsf with pkg-config
find_package(PkgConfig)
pkg_search_module(gsf IMPORTED_TARGET libgsf-1)

# Then try to find libgsf globally, whether or not libgsf have been found with pkg-config
find_program(gsf NAMES gsf REQUIRED PATHS ${gsf_PREFIX}/bin)
add_executable(read_ole test_read_file.cpp)

find_package(GTest REQUIRED)
target_link_libraries(read_ole PRIVATE containerfs GTest::gtest_main)

add_test(NAME read_ole
         COMMAND read_ole)
set_tests_properties(read_ole PROPERTIES FIXTURES_REQUIRED "READ_SETUP1;READ_SETUP2")

########################################################################################
# These fixtures should be used as follows:
# set_tests_properties(read_ole PROPERTIES FIXTURES_REQUIRED "READ_SETUP1;READ_SETUP2")
########################################################################################
# create dirs for using by gsf
add_test(NAME read_setup1
         COMMAND ${CMAKE_COMMAND} -E tar x ${CMAKE_SOURCE_DIR}/test/test.tgz)
set_tests_properties(read_setup1 PROPERTIES FIXTURES_SETUP READ_SETUP1)

# remove dirs
add_test(NAME read_cleanup1
         COMMAND ${CMAKE_COMMAND} -E rm -r ./root)
set_tests_properties(read_cleanup1 PROPERTIES FIXTURES_CLEANUP READ_SETUP1)

# create ole file using gsf
add_test(NAME read_setup2
         COMMAND ${gsf} createole test.ole root)
set_tests_properties(read_setup2 PROPERTIES
                     FIXTURES_REQUIRED READ_SETUP1
                     FIXTURES_SETUP READ_SETUP2)

# remove ole file after using
add_test(NAME read_cleanup2
         COMMAND ${CMAKE_COMMAND} -E rm test.ole)
set_tests_properties(read_cleanup2 PROPERTIES
                     FIXTURES_REQUIRED READ_SETUP1
                     FIXTURES_CLEANUP READ_SETUP2)
########################################################################################