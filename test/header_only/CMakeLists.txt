cmake_minimum_required(VERSION 3.24...3.31)

project(
  header_only
  VERSION 1.0.0.0
  LANGUAGES CXX
  DESCRIPTION "A header only dependency for testing PackageProject.cmake"
)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. PackageProject)

# supported in CMake 3.23 and newer
set(CMAKE_VERIFY_INTERFACE_HEADER_SETS ON)

add_library(header_only INTERFACE)

string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)
string(TOUPPER ${PROJECT_NAME} UPPERCASE_PROJECT_NAME)
set(${UPPERCASE_PROJECT_NAME}_VERSION \"${PROJECT_VERSION}\")
configure_file(${PACKAGE_PROJECT_ROOT_PATH}/version.h.in ${VERSION_HEADER_LOCATION} @ONLY)

if(CMAKE_VERIFY_INTERFACE_HEADER_SETS)
  target_sources(
    header_only
    PUBLIC FILE_SET
           HEADERS
           BASE_DIRS
           include
           ${PROJECT_BINARY_DIR}
           FILES
           include/header_only/adder.h
           ${PROJECT_BINARY_DIR}/${VERSION_HEADER_LOCATION}
  )
else()
  target_include_directories(
    header_only INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                          $<INSTALL_INTERFACE:include>
  )
endif()

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DESTINATION include
  DEPENDENCIES ""
  CPACK "${TEST_CPACK}"
  HEADER_SETS HEADERS
)
