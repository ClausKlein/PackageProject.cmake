cmake_minimum_required(VERSION 3.24...3.31)

project(
  header_only
  VERSION 1.0
  LANGUAGES CXX
  DESCRIPTION "A header only dependency for testing PackageProject.cmake"
)

# supported in CMake 3.23 and newer
set(CMAKE_VERIFY_INTERFACE_HEADER_SETS ON)

add_library(header_only INTERFACE)

if(CMAKE_VERIFY_INTERFACE_HEADER_SETS)
  target_sources(
    header_only PUBLIC FILE_SET HEADERS BASE_DIRS include FILES include/header_only/adder.h
  )
else()
  target_include_directories(
    header_only INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                          $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
  )
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. PackageProject)

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  # INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include TODO: INCLUDE_DESTINATION
  # include/${PROJECT_NAME}-${PROJECT_VERSION}
  INCLUDE_DESTINATION include
  VERSION_HEADER "${PROJECT_NAME}/version.h"
  DEPENDENCIES ""
  CPACK "${TEST_CPACK}"
  HEADER_SETS HEADERS
)
