cmake_minimum_required(VERSION 3.22)

project(ole
        VERSION 0.0.1
        DESCRIPTION "Serialize and parse OLE storage"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

add_library(ole SHARED)
add_library(ole::ole ALIAS ole)

target_sources(ole
               PRIVATE
               sources/filesystem.cpp

               include/ole/filesystem.h
               include/ole/error.h
               include/ole/device_api.h
               include/ole/file_device.h
               include/ole/string.h
               include/ole/string.cpp
               include/ole/path.h
               include/ole/path.cpp
               include/ole/directory.h)

target_include_directories(ole
                           PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           INTERFACE
                           $<INSTALL_INTERFACE:include>)

include(GenerateExportHeader)
generate_export_header(ole
                       EXPORT_MACRO_NAME OLE_API
                       EXPORT_FILE_NAME include/ole/export.h)
include(CPack)
include(CTest)
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()

