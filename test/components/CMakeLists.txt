cmake_minimum_required(VERSION 3.14...3.31)

project(
  components
  VERSION 1.0
  LANGUAGES CXX
  DESCRIPTION "A multi-component library setup for testing PackageProject.cmake"
)

add_library(components_core INTERFACE)
target_include_directories(
  components_core INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                            $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

add_library(components_add STATIC source/add.cpp)
target_link_libraries(components_add PUBLIC components_core)

add_library(components_mult STATIC source/mult.cpp)
target_link_libraries(components_mult PUBLIC components_core)

add_library(components_pow STATIC source/pow.cpp)
# Note: we are explicitly sloppy about Pow's dependency to Core in that we don't specify the
# dependency explicitly. Rather we rely on transitive dependency handling on cmake's side
target_link_libraries(components_pow PUBLIC components_mult)

# Only add a subset of aliases here to test packageProject's capability to define alias targets
add_library(Comps::Core ALIAS components_core)

foreach(CURRENT IN ITEMS components_add components_mult components_pow)
  target_include_directories(
    ${CURRENT} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                      $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
  )
endforeach()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../.. PackageProject)

packageProject(
  NAME ${PROJECT_NAME}
  COMPONENTS # These are the different possibilities of specifying components (with aliases)
             Comps::Core components_add components_mult
  AS Comps::Mult components_pow
  AS Pow
  VERSION ${PROJECT_VERSION}
  NAMESPACE Comps
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  DEFAULT_COMPONENTS Core Add
  COMPONENT_DEPENDENCIES
    components_add
    ON
    Core
    Comps::Mult
    ON
    Comps::Core
    # Again, we are implicit about Pow's dependence on Core
    Pow
    ON
    components_mult
  CPACK "${TEST_CPACK}"
)

foreach(ALIAS_NAME IN ITEMS Core Mult Pow)
  if(NOT TARGET Comps::${ALIAS_NAME})
    message(FATAL_ERROR "Comps::${ALIAS_NAME} was expected to be defined, but wasn't")
  endif()
endforeach()
