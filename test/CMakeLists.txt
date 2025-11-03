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

add_test(NAME read_ole COMMAND read_ole)

include(AddOleFixture.cmake)

add_ole_fixture(
        NAME exists
        OLE_FILE exists.ole
        ROOT_DIR exists
        BIND_TEST read_ole
        SPEC
        f:c/c/c.txt
        d:a/a
        d:a/b
        d:b/a
        d:b/b
        )
########################################################################################