cmake_minimum_required(VERSION 3.22)

project(containerfs
        VERSION 0.0.1
        DESCRIPTION "Serialize and parse OLE storage"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

add_library(containerfs SHARED)
add_library(containerfs::containerfs ALIAS containerfs)

target_sources(containerfs
               PRIVATE
               sources/ole.cpp

               include/containerfs/ole.h
               include/containerfs/ole_error.h
               include/containerfs/device_api.h
               include/containerfs/file_device.h
               include/containerfs/filesystem.h
               include/containerfs/ole_string.h
               include/containerfs/ole_string.cpp
               include/containerfs/ole_path.h
               include/containerfs/ole_path.cpp
               include/containerfs/ole_directory.h)

target_include_directories(containerfs
                           PUBLIC
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           INTERFACE
                           $<INSTALL_INTERFACE:include>)

include(GenerateExportHeader)
generate_export_header(containerfs
                       EXPORT_MACRO_NAME CONTAINERFS_API
                       EXPORT_FILE_NAME include/containerfs/export.h)
include(CPack)
include(CTest)
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()

