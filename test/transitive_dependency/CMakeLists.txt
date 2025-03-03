cmake_minimum_required(VERSION 3.24...3.31)

project(
  transitive_dependency
  VERSION 7.8.9.21948124
  LANGUAGES CXX
  DESCRIPTION "A transitive dependency for testing PackageProject.cmake"
)

include(cmake/CPM.cmake)

CPMAddPackage(
  NAME fmt
  GIT_TAG 11.1.4
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES"
)

CPMAddPackage(
  NAME cxxopts
  GITHUB_REPOSITORY jarro2783/cxxopts
  VERSION 3.2.1
  OPTIONS "CXXOPTS_ENABLE_INSTALL YES"
)

# Set default visibility to hidden for all targets
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

# supported in CMake 3.23 and newer
set(CMAKE_VERIFY_INTERFACE_HEADER_SETS NO)

add_library(${PROJECT_NAME} SHARED source/transitive_dependency.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt-header-only cxxopts::cxxopts)

if(CMAKE_VERIFY_INTERFACE_HEADER_SETS)
  target_sources(
    transitive_dependency
    PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
           include/transitive_dependency/transitive_dependency.h
           # TODO: add missing generated headers too!
  )
else()
  target_include_directories(
    transitive_dependency PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                                 $<INSTALL_INTERFACE:include>
  )
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. PackageProject)

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include
  VERSION_HEADER "transitive_dependency/version.h"
  EXPORT_HEADER "transitive_dependency/export.h"
  DEPENDENCIES "fmt 11.1.4;cxxopts 3.2.1"
  CPACK "${TEST_CPACK}"
  RUNTIME_DESTINATION /
  HEADER_SETS HEADERS
)
