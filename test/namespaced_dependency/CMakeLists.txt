cmake_minimum_required(VERSION 3.24...3.31)

project(
  namespaced_dependency
  VERSION 4.5.6
  LANGUAGES CXX
  DESCRIPTION "A namespaced dependency for testing PackageProject.cmake"
)

set(CMAKE_VERIFY_INTERFACE_HEADER_SETS ON)

set(PROJECT_NAMESPACE "ns")
add_library(namespaced_dependency STATIC source/namespaced_dependency.cpp)

if(CMAKE_VERIFY_INTERFACE_HEADER_SETS)
  target_sources(
    namespaced_dependency PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
                                 include/namespaced_dependency/namespaced_dependency.h
  )
else()
  target_include_directories(
    namespaced_dependency PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                                 $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
  )
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. PackageProject)

# the alias ${PROJECT_NAMESPACE}::${PROJECT_NAME} is automatically provided by PackageProject.cmake
# if we use the `NAMESPACE` parameter

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAMESPACE}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  # INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include TODO: INCLUDE_DESTINATION
  # include/${PROJECT_NAME}-${PROJECT_VERSION}
  INCLUDE_DESTINATION include
  VERSION_HEADER "namespaced_dependency/version.h"
  INCLUDE_HEADER_PATTERN "*.h"
  DEPENDENCIES ""
  CPACK "${TEST_CPACK}"
  HEADER_SETS HEADERS
)
