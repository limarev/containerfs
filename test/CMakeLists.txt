cmake_minimum_required(VERSION 3.22)

project(ole_test
        DESCRIPTION "Test ole module"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED 23)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

# First try to find libgsf with pkg-config
find_package(PkgConfig)
pkg_search_module(gsf IMPORTED_TARGET libgsf-1)

# Then try to find libgsf globally, whether or not libgsf have been found with pkg-config
find_program(gsf NAMES gsf REQUIRED PATHS ${gsf_PREFIX}/bin)
add_executable(filesystem_test filesystem_test.cpp)

find_package(GTest REQUIRED)
target_link_libraries(filesystem_test PRIVATE ole::ole GTest::gtest_main)

add_test(NAME filesystem_test COMMAND filesystem_test)

include(AddOleFixture.cmake)

add_ole_fixture(
        NAME exists
        OLE_FILE exists.ole
        ROOT_DIR exists
        BIND_TEST filesystem_test
        SPEC
        f:c/c/c.txt
        d:a/a
        d:a/b
        d:b/a
        d:b/b
        )
########################################################################################