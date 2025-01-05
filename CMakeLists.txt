cmake_minimum_required(VERSION 3.14)

set(PACKAGE_PROJECT_ROOT_PATH
    "${CMAKE_CURRENT_LIST_DIR}"
    CACHE INTERNAL "The path to the PackageProject directory"
)

function(_package_project_parse_version VERSION MAJOR_VAR MINOR_VAR PATCH_VAR TWEAK_VAR)
  # clear previous matches
  unset(CMAKE_MATCH_1)
  unset(CMAKE_MATCH_3)
  unset(CMAKE_MATCH_5)
  unset(CMAKE_MATCH_7)

  string(REGEX MATCH "^([0-9]+)(\\.([0-9]+))?(\\.([0-9]+))?(\\.([0-9]+))?$" _ "${VERSION}")

  set(MAJOR ${CMAKE_MATCH_1})
  set(MINOR ${CMAKE_MATCH_3})
  set(PATCH ${CMAKE_MATCH_5})
  set(TWEAK ${CMAKE_MATCH_7})

  if(NOT DEFINED MAJOR)
    set(MAJOR "0")
  endif()
  if(NOT DEFINED MINOR)
    set(MINOR "0")
  endif()
  if(NOT DEFINED PATCH)
    set(PATCH "0")
  endif()
  if(NOT DEFINED TWEAK)
    set(TWEAK "0")
  endif()

  set(${MAJOR_VAR}
      "${MAJOR}"
      PARENT_SCOPE
  )
  set(${MINOR_VAR}
      "${MINOR}"
      PARENT_SCOPE
  )
  set(${PATCH_VAR}
      "${PATCH}"
      PARENT_SCOPE
  )
  set(${TWEAK_VAR}
      "${TWEAK}"
      PARENT_SCOPE
  )
endfunction()

function(_package_project_resolve_target TARGET UNDERLYING_TARGET_VAR ALIAS_VAR)
  if(NOT TARGET ${TARGET})
    message(FATAL_ERROR "Expected '${TARGET}' to be a valid target, but it isn't")
  endif()

  get_target_property(ALIASED_TO "${TARGET}" ALIASED_TARGET)

  if(ALIASED_TO)
    set(${UNDERLYING_TARGET_VAR}
        "${ALIASED_TO}"
        PARENT_SCOPE
    )
    set(${ALIAS_VAR}
        "${TARGET}"
        PARENT_SCOPE
    )
  else()
    set(${UNDERLYING_TARGET_VAR}
        "${TARGET}"
        PARENT_SCOPE
    )
    unset(${ALIAS_VAR} PARENT_SCOPE)
  endif()
endfunction()

function(_package_project_determine_visibility_flag TARGET OUTPUT_VARIABLE)
  get_target_property(TARGET_TYPE "${TARGET}" TYPE)

  if("${TARGET_TYPE}" STREQUAL "INTERFACE_LIBRARY")
    set(${OUTPUT_VARIABLE}
        "INTERFACE"
        PARENT_SCOPE
    )
  else()
    set(${OUTPUT_VARIABLE}
        "PUBLIC"
        PARENT_SCOPE
    )
  endif()
endfunction()

function(_package_project_normalize_alias_name NAME NAMESPACE TARGET_LIST ALIAS_LIST
         OUTPUT_VARIABLE
)
  if(TARGET "${NAME}")
    _package_project_resolve_target("${NAME}" TARGET_NAME ALIAS_NAME)

    if(ALIAS_NAME AND NAMESPACE)
      string(REGEX REPLACE "^${NAMESPACE}" "" ALIAS_NAME "${ALIAS_NAME}")
    endif()

    list(FIND TARGET_LIST "${TARGET_NAME}" TARGET_IDX)

    if(TARGET_IDX LESS 0)
      set(NAME "${ALIAS_NAME}")
    else()
      list(GET ALIAS_LIST ${TARGET_IDX} NAME)

      if(NOT NAME)
        set(NAME "${ALIAS_NAME}")
      endif()
    endif()

    if(ALIAS_NAME AND NOT ("${ALIAS_NAME}" STREQUAL "${NAME}"))
      message(FATAL_ERROR "Inconsistent alias names: '${ALIAS_NAME}' and '${NAME}'")
    endif()
  else()
    # If NAME is not a target, it can't be part of the target list (as all entries in there are, by
    # definition, targets). Looking up in ALIAS_LIST is no good as this would either result in a
    # match, in which case NAME already is the normalized alias name. If there is no match the only
    # thing we can try to do is to strip the namespace, which we do below anyway.
  endif()

  if(NAMESPACE)
    string(REGEX REPLACE "^${NAMESPACE}" "" NAME "${NAME}")
  endif()

  set(${OUTPUT_VARIABLE}
      "${NAME}"
      PARENT_SCOPE
  )
endfunction()

function(_package_project_normalize_target_name NAME NAMESPACE TARGET_LIST ALIAS_LIST
         OUTPUT_VARIABLE
)
  if(TARGET "${NAME}")
    _package_project_resolve_target("${NAME}" TARGET_NAME ALIAS_NAME)
    set(NAME "${TARGET_NAME}")
  else()
    if(NAMESPACE AND NAME MATCHES "^${NAMESPACE}")
      # Remove namespace
      string(REGEX REPLACE "^${NAMESPACE}" "" NAME "${NAME}")
      set(REMOVED_NAMESPACE ON)
    endif()

    list(FIND TARGET_LIST "${NAME}" TARGET_IDX)

    if(TARGET_IDX LESS 0)
      # Check if the given target
      list(FIND ALIAS_LIST "${NAME}" TARGET_IDX)
    endif()

    if(TARGET_IDX LESS 0)
      # return unchanged
      if(NOT REMOVED_NAMESPACE AND NOT TARGET "${NAME}")
        # This assumes that everything that started with the given namespace is an internal target
        # and is therefore not required to be an existing target (in the current form)
        message(
          FATAL_ERROR
            "Assumed all non-internal target names to be real cmake targets but '${NAME}' isn't"
        )
      endif()
    else()
      # internal target -> normalize by using name of underlying target (as targets may not have an
      # alias)
      list(GET TARGET_LIST "${TARGET_IDX}" NAME)
    endif()
  endif()

  set(${OUTPUT_VARIABLE}
      "${NAME}"
      PARENT_SCOPE
  )
endfunction()

function(
  _package_project_parse_components
  COMPONENT_SPECS
  NAMESPACE
  TARGET_LIST
  ALIAS_LIST
  OUT_TARGET_LIST_NAME
  OUT_ALIAS_LIST_NAME
)
  # Component specification has the syntax <targetName> [AS <alias>] that is, each component is
  # given as a target and optionally an alias name (indicated by the "AS" keyword)

  # The input list simply contains all individual words (space-separated) of the component
  # specification in a single list. In order to get a list with a single entry per component, we
  # merge elements separated by an "AS" into a single component specication (that now may include
  # the alias specification).
  string(REPLACE ";AS;" "<->" COMPONENT_LIST "${COMPONENT_SPECS}")

  # Work around cmake not being able to add empty strings to empty lists
  list(PREPEND ALIAS_LIST "ToBeRemoved")

  foreach(CURRENT_COMPONENT IN LISTS COMPONENT_LIST)
    if("${CURRENT_COMPONENT}" MATCHES "([^<]+)<->(.+)")
      # Component with alias
      set(TARGET_NAME "${CMAKE_MATCH_1}")
      set(ALIAS_NAME "${CMAKE_MATCH_2}")
    else()
      # Component without alias specification -> try to see whether we can derive alias from the
      # component name itself
      set(TARGET_NAME "${CURRENT_COMPONENT}")
      set(ALIAS_NAME "${CURRENT_COMPONENT}")
      set(INVALID_ALIAS "${CURRENT_COMPONENT}")
    endif()

    _package_project_normalize_target_name(
      "${TARGET_NAME}" "${NAMESPACE}" "${TARGET_LIST}" "${ALIAS_LIST}" TARGET_NAME
    )
    _package_project_normalize_alias_name(
      "${ALIAS_NAME}" "${NAMESPACE}" "${TARGET_LIST}" "${ALIAS_LIST}" ALIAS_NAME
    )
    if("${ALIAS_NAME}" STREQUAL "${INVALID_ALIAS}")
      # Deriving alias from unmodified component name failed -> there is no alias for this target
      set(ALIAS_NAME "")
    endif()

    list(APPEND TARGET_LIST "${TARGET_NAME}")
    list(APPEND ALIAS_LIST "${ALIAS_NAME}")
  endforeach()

  # Remove dummy element
  list(POP_FRONT ALIAS_LIST)

  set(${OUT_TARGET_LIST_NAME}
      "${TARGET_LIST}"
      PARENT_SCOPE
  )
  set(${OUT_ALIAS_LIST_NAME}
      "${ALIAS_LIST}"
      PARENT_SCOPE
  )
endfunction()

function(
  _package_project_parse_dependencies
  DEPENDENCIES_SPEC
  TARGET_LIST
  ALIAS_LIST
  DEPENDENCY_LIST
  OUT_DEPENDENCY_LIST_NAME
  DEPENDENCY_SEPARATOR
  NAMESPACE
)
  # Dependency specifications follow the syntax <component> ON <dep1> [... <depN>]

  set(REFERENCED_COMPONENTS "")
  set(COMPONENT_DEPENDENCIES "")

  # Parse the specification
  list(LENGTH DEPENDENCIES_SPEC LIST_SIZE)
  set(IDX 0)
  while(IDX LESS LIST_SIZE)
    list(GET DEPENDENCIES_SPEC ${IDX} CURRENT_COMPONENT)
    math(EXPR IDX "${IDX} + 1")
    list(GET DEPENDENCIES_SPEC ${IDX} ON_KEYWORD)
    if(NOT ("${ON_KEYWORD}" STREQUAL "ON"))
      message(
        FATAL_ERROR
          "Invalid dependency specification for component '${CURRENT_COMPONENT}' - expected component name to be followed by 'ON'"
      )
    endif()
    math(EXPR IDX "${IDX} + 1")

    list(SUBLIST DEPENDENCIES_SPEC ${IDX} -1 REMAINING)
    list(FIND REMAINING "ON" ON_IDX)
    if(ON_IDX LESS 0)
      # No more 'ON' keyword found -> there are no additional component dependencies after the
      # current one
      set(CURRENT_DEPENDENCIES "${REMAINING}")
      set(IDX "${LIST_SIZE}")
    elseif(ON_IDX EQUAL 0)
      message(
        FATAL_ERROR
          "Duplicated 'ON' in dependency specification for component '${CURRENT_COMPONENT}'"
      )
    elseif(ON_IDX EQUAL 1)
      message(FATAL_ERROR "Empty dependency specification for component '${CURRENT_COMPONENT}'")
    else()
      math(EXPR N_DEPS "${ON_IDX} - 1")
      list(SUBLIST DEPENDENCIES_SPEC ${IDX} ${N_DEPS} CURRENT_DEPENDENCIES)
      math(EXPR IDX "${IDX} + ${N_DEPS}")
    endif()

    string(REPLACE ";" "${DEPENDENCY_SEPARATOR}" DEPENDENCY_STRING "${CURRENT_DEPENDENCIES}")
    list(APPEND REFERENCED_COMPONENTS "${CURRENT_COMPONENT}")
    list(APPEND COMPONENT_DEPENDENCIES "${DEPENDENCY_STRING}")
  endwhile()

  # Process parsed results

  # Ensure that the dependency list has the same size as the target list
  list(LENGTH DEPENDENCY_LIST N_DEPS)
  list(LENGTH TARGET_LIST N_TARGETS)
  math(EXPR N_MISSING "${N_TARGETS} - ${N_DEPS}")
  foreach(_ RANGE 1 "${N_MISSING}") # start at 1 since stop is inclusive
    # Empty dependency lists have to be represented as a list of two empty elements due to quirks in
    # cmake regarding the representation and handling of empty lists
    list(APPEND DEPENDENCY_LIST "${DEPENDENCY_SEPARATOR}")
  endforeach()

  list(LENGTH COMPONENT_DEPENDENCIES N_DEPS_SPECIFIED)
  if(N_DEPS_SPECIFIED GREATER 0)
    math(EXPR STOP "${N_DEPS_SPECIFIED} - 1")
    foreach(IDX RANGE ${STOP})
      list(GET REFERENCED_COMPONENTS ${IDX} CURRENT_COMPONENT)
      list(GET COMPONENT_DEPENDENCIES ${IDX} CURRENT_DEPENDENCIES)

      _package_project_normalize_target_name(
        "${CURRENT_COMPONENT}" "${NAMESPACE}" "${TARGET_LIST}" "${ALIAS_LIST}" CURRENT_COMPONENT
      )

      list(FIND TARGET_LIST "${CURRENT_COMPONENT}" COMPONENT_IDX)

      if(COMPONENT_IDX LESS 0)
        message(
          FATAL_ERROR
            "Found dependency specification for unknown component '${CURRENT_COMPONENT}' (might have been referenced via an ALIAS)"
        )
      endif()

      # Check we are not overwriting an existing dependency spec (that would be a bug)
      list(GET DEPENDENCY_LIST ${COMPONENT_IDX} PREV_DEPS)
      if(NOT ("${PREV_DEPS}" STREQUAL "${DEPENDENCY_SEPARATOR}"))
        message(
          FATAL_ERROR
            "Internal error in dependency parsing - would overwrite '${PREV_DEPS}' with '${CURRENT_DEPENDENCIES}' for component '${CURRENT_COMPONENT}'"
        )
      endif()

      # Transform into proper list
      string(REPLACE "${DEPENDENCY_SEPARATOR}" ";" CURRENT_DEPENDENCIES "${CURRENT_DEPENDENCIES}")

      set(NORMALIZED_DEPENDENCIES "")
      foreach(DEP_SPEC IN LISTS CURRENT_DEPENDENCIES)
        # Turn dependency specification into a list
        string(REPLACE " " ";" DEP_SPEC "${DEP_SPEC}")
        list(POP_FRONT DEP_SPEC DEP_NAME)

        # Prefer using alias names for dependencies
        _package_project_normalize_alias_name(
          "${DEP_NAME}" "${NAMESPACE}" "${TARGET_LIST}" "${ALIAS_LIST}" NORMALIZED_DEP_NAME
        )
        if(NOT NORMALIZED_DEP_NAME)
          # If there is no alias, use the original name
          set(NORMALIZED_DEP_NAME "${DEP_NAME}")
        endif()

        list(PREPEND DEP_SPEC "${NORMALIZED_DEP_NAME}")
        # Turn dependency specification back into a string
        list(JOIN DEP_SPEC " " DEP_SPEC)

        list(APPEND NORMALIZED_DEPENDENCIES "${DEP_SPEC}")
      endforeach()

      # Turn back into a single string separated by DEPENDENCY_SEPARATOR
      list(JOIN NORMALIZED_DEPENDENCIES "${DEPENDENCY_SEPARATOR}" NORMALIZED_DEPENDENCIES)

      # Update dependency entry belonging to the current component
      list(REMOVE_AT DEPENDENCY_LIST ${COMPONENT_IDX})
      list(INSERT DEPENDENCY_LIST ${COMPONENT_IDX} "${NORMALIZED_DEPENDENCIES}")
    endforeach()
  endif()

  set(${OUT_DEPENDENCY_LIST_NAME}
      "${DEPENDENCY_LIST}"
      PARENT_SCOPE
  )
  message(STATUS "Final dependency list: ${DEPENDENCY_LIST}")
endfunction()

function(packageProject)
  include(CMakePackageConfigHelpers)
  include(GNUInstallDirs)

  cmake_parse_arguments(
    PROJECT
    ""
    "NAME;VERSION;INCLUDE_DIR;INCLUDE_DESTINATION;BINARY_DIR;COMPATIBILITY;EXPORT_HEADER;VERSION_HEADER;NAMESPACE;DISABLE_VERSION_SUFFIX;ARCH_INDEPENDENT;INCLUDE_HEADER_PATTERN;CPACK;RUNTIME_DESTINATION;"
    "DEPENDENCIES;COMPONENTS;COMPONENT_DEPENDENCIES;DEFAULT_COMPONENTS"
    ${ARGN}
  )

  if(DEFINED PROJECT_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unknown arguments '${PROJECT_UNPARSED_ARGUMENTS}' in packageProject")
  endif()

  # optional feature: TRUE or FALSE or UNDEFINED! These variables will then hold the respective
  # value from the argument list or be undefined if the associated one_value_keyword could not be
  # found.
  if(PROJECT_DISABLE_VERSION_SUFFIX)
    unset(PROJECT_VERSION_SUFFIX)
  else()
    set(PROJECT_VERSION_SUFFIX -${PROJECT_VERSION})
  endif()

  if(NOT DEFINED PROJECT_COMPATIBILITY)
    set(PROJECT_COMPATIBILITY AnyNewerVersion)
  endif()

  _package_project_parse_version(
    "${PROJECT_VERSION}" PROJECT_VERSION_MAJOR PROJECT_VERSION_MINOR PROJECT_VERSION_PATCH
    PROJECT_VERSION_TWEAK
  )

  # cmake doesn't support appending empty elements to an empty list. Therefore, we have to add a
  # dummy element to it (https://stackoverflow.com/a/58581283)
  set(UNDERLYING_TARGETS "ToBeRemoved")
  set(TARGET_ALIASES "ToBeRemoved")
  set(PACKAGE_TARGET_NAMES "ToBeRemoved")
  set(PACKAGE_TARGET_DEPENDENCIES "ToBeRemoved")

  set(TARGET_DEPENDENCY_SEPARATOR " & ")

  if(DEFINED PROJECT_NAMESPACE)
    if(PROJECT_CPACK)
      set(CPACK_PACKAGE_NAMESPACE ${PROJECT_NAMESPACE})
    endif()
    set(PROJECT_NAMESPACE ${PROJECT_NAMESPACE}::)
  endif()

  if(TARGET "${PROJECT_NAME}")
    _package_project_normalize_target_name(
      "${PROJECT_NAME}" "${PROJECT_NAMESPACE}" "${UNDERLYING_TARGETS}" "${TARGET_ALIASES}"
      TARGET_NAME
    )
    _package_project_normalize_alias_name(
      "${PROJECT_NAME}" "${PROJECT_NAMESPACE}" "${UNDERLYING_TARGETS}" "${TARGET_ALIASES}"
      ALIAS_NAME
    )

    list(APPEND UNDERLYING_TARGETS "${TARGET_NAME}")
    list(APPEND TARGET_ALIASES "${ALIAS_NAME}")

    if(ALIAS_NAME)
      set(PACKAGE_MAIN_TARGET "${ALIAS_TARGET}")
    else()
      set(PACKAGE_MAIN_TARGET "${TARGET_NAME}")
    endif()

    # Adapt syntax to be applicable to the parsing routine
    if(PROJECT_DEPENDENCIES)
      list(PREPEND PROJECT_DEPENDENCIES "ON")
      list(PREPEND PROJECT_DEPENDENCIES "${TARGET_NAME}")
    endif()
    _package_project_parse_dependencies(
      "${PROJECT_DEPENDENCIES}"
      "${UNDERLYING_TARGETS}"
      "${TARGET_ALIASES}"
      "${PACKAGE_TARGET_DEPENDENCIES}"
      PACKAGE_TARGET_DEPENDENCIES
      "${TARGET_DEPENDENCY_SEPARATOR}"
      "${PROJECT_NAMESPACE}"
    )
  elseif(NOT PROJECT_COMPONENTS)
    message(
      FATAL_ERROR "No components specified and '${PROJECT_NAME}' itself is not a valid target"
    )
  endif()

  # Process specified components (if any)
  if(DEFINED PROJECT_COMPONENTS)
    _package_project_parse_components(
      "${PROJECT_COMPONENTS}" "${PROJECT_NAMESPACE}" "${UNDERLYING_TARGETS}" "${TARGET_ALIASES}"
      UNDERLYING_TARGETS TARGET_ALIASES
    )

    _package_project_parse_dependencies(
      "${PROJECT_COMPONENT_DEPENDENCIES}"
      "${UNDERLYING_TARGETS}"
      "${TARGET_ALIASES}"
      "${PACKAGE_TARGET_DEPENDENCIES}"
      PACKAGE_TARGET_DEPENDENCIES
      "${TARGET_DEPENDENCY_SEPARATOR}"
      "${PROJECT_NAMESPACE}"
    )
  endif()

  if(PROJECT_NAMESPACE)
    # Strip namespace from aliases
    list(TRANSFORM TARGET_ALIASES REPLACE "^${PROJECT_NAMESPACE}" "")
  endif()

  list(LENGTH UNDERLYING_TARGETS STOP)
  math(EXPR STOP "${STOP} - 1")
  # Note: we start at index 1 to skip over the still-present dummy entries
  foreach(IDX RANGE 1 "${STOP}")
    list(GET UNDERLYING_TARGETS ${IDX} CURRENT_TARGET)
    list(GET TARGET_ALIASES ${IDX} CURRENT_ALIAS)

    _package_project_resolve_target(${CURRENT_TARGET} UNDERLYING_TARGET ALIAS_TARGET)
    if(PROJECT_NAMESPACE)
      string(REGEX REPLACE "^${PROJECT_NAMESPACE}" "" ALIAS_TARGET "${ALIAS_TARGET}")
    endif()

    if(ALIAS_TARGET)
      if(CURRENT_ALIAS AND NOT ("${CURRENT_ALIAS}" STREQUAL "${ALIAS_TARGET})"))
        message(
          FATAL_ERROR
            "Provided target '${CURRENT_TARGET}' is already an ALIAS target but the explicitly provided alias is different ('${CURRENT_ALIAS}')"
        )
      else()
        # The given target is actually an alias target -> use it as the alias and infer the aliased
        # target
        set(CURRENT_ALIAS "${ALIAS_TARGET}")
        set(CURRENT_TARGET "${UNDERLYING_TARGET}")
      endif()
    endif()

    if(CURRENT_ALIAS)
      if(NOT TARGET "${PROJECT_NAMESPACE}${CURRENT_ALIAS}")
        # Define the alias target, if it doesn't exist yet
        add_library("${PROJECT_NAMESPACE}${CURRENT_ALIAS}" ALIAS "${CURRENT_TARGET}")
      endif()

      # Ensure that the export will use the ALIAS name
      set_target_properties(${CURRENT_TARGET} PROPERTIES EXPORT_NAME ${CURRENT_ALIAS})
    elseif(PROJECT_NAMESPACE AND NOT TARGET "${PROJECT_NAMESPACE}${CURRENT_TARGET}")
      # Ensure an appropriate namespaced target exists
      add_library(${PROJECT_NAMESPACE}${CURRENT_TARGET} ALIAS ${CURRENT_TARGET})
    endif()

    # Update the entries in our lists in case CURRENT_TARGET/CURRENT_ALIAS has been modified
    list(REMOVE_AT UNDERLYING_TARGETS ${IDX})
    list(INSERT UNDERLYING_TARGETS ${IDX} "${CURRENT_TARGET}")
    list(REMOVE_AT TARGET_ALIASES ${IDX})
    list(INSERT TARGET_ALIASES ${IDX} "${CURRENT_ALIAS}")
  endforeach()

  # Get rid of dummy entries in lists
  list(POP_FRONT UNDERLYING_TARGETS)
  list(POP_FRONT TARGET_ALIASES)
  list(POP_FRONT PACKAGE_TARGET_NAMES)
  list(POP_FRONT PACKAGE_TARGET_DEPENDENCIES)

  if(DEFINED PROJECT_VERSION_HEADER OR DEFINED PROJECT_EXPORT_HEADER)
    if(NOT PACKAGE_MAIN_TARGET)
      message(
        FATAL_ERROR
          "VERSION_HEADER and EXPORT_HEADER can only be used if '${PROJECT_NAME}' is itself a target"
      )
    endif()

    # In case ${PROJECT_NAME} is itself a target, its underlying target will be the first entry in
    # the UNDERLYING_TARGETS list
    list(GET UNDERLYING_TARGETS 0 MAIN_TARGET)

    set(PROJECT_VERSION_INCLUDE_DIR ${PROJECT_BINARY_DIR}/PackageProjectInclude)

    if(DEFINED PROJECT_EXPORT_HEADER)
      include(GenerateExportHeader)
      generate_export_header(
        ${MAIN_TARGET} EXPORT_FILE_NAME ${PROJECT_VERSION_INCLUDE_DIR}/${PROJECT_EXPORT_HEADER}
      )
    endif()

    if(DEFINED PROJECT_VERSION_HEADER)
      string(TOUPPER ${PROJECT_NAME} UPPERCASE_PROJECT_NAME)
      # ensure that the generated macro does not include invalid characters
      string(REGEX REPLACE [^a-zA-Z0-9] _ UPPERCASE_PROJECT_NAME ${UPPERCASE_PROJECT_NAME})
      configure_file(
        ${PACKAGE_PROJECT_ROOT_PATH}/version.h.in
        ${PROJECT_VERSION_INCLUDE_DIR}/${PROJECT_VERSION_HEADER} @ONLY
      )
    endif()

    _package_project_determine_visibility_flag(${MAIN_TARGET} VISIBILITY)
    target_include_directories(
      ${MAIN_TARGET} ${VISIBILITY} "$<BUILD_INTERFACE:${PROJECT_VERSION_INCLUDE_DIR}>"
    )
    install(
      DIRECTORY ${PROJECT_VERSION_INCLUDE_DIR}/
      DESTINATION ${PROJECT_INCLUDE_DESTINATION}
      COMPONENT "${PROJECT_NAME}_Development"
    )
  endif()

  if(NOT DEFINED PROJECT_ARCH_INDEPENDENT)
    set(PROJECT_ARCH_INDEPENDENT YES)

    foreach(CURRENT_TARGET IN LISTS UNDERLYING_TARGETS)
      get_target_property(CURRENT_TYPE ${CURRENT_TARGET} TYPE)

      if(NOT ("${CURRENT_TYPE}" STREQUAL "INTERFACE_LIBRARY"))
        set(PROJECT_ARCH_INDEPENDENT NO)
        break()
      endif()
    endforeach()
  endif()

  set(wbpvf_extra_args "")
  if(PROJECT_ARCH_INDEPENDENT)
    set(wbpvf_extra_args ARCH_INDEPENDENT)
    # install to architecture independent (share) directory
    set(INSTALL_DIR_FOR_CMAKE_CONFIGS ${CMAKE_INSTALL_DATADIR})
  else()
    # if x32 or multilib->x32 , install to (lib) directory. if x64, install to (lib64) directory
    set(INSTALL_DIR_FOR_CMAKE_CONFIGS ${CMAKE_INSTALL_LIBDIR})
  endif()

  string(TOLOWER "${PROJECT_NAME}" PROJECT_NAME_LOWER)
  write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWER}-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ${PROJECT_COMPATIBILITY} ${wbpvf_extra_args}
  )

  # set default runtime install subdirectory (RUNTIME_DESTINATION)
  if(NOT DEFINED PROJECT_RUNTIME_DESTINATION)
    set(PROJECT_RUNTIME_DESTINATION ${PROJECT_NAME}${PROJECT_VERSION_SUFFIX})
  endif()

  foreach(CURRENT_TARGET IN LISTS UNDERLYING_TARGETS)
    set_target_properties(${CURRENT_TARGET} PROPERTIES VERSION ${PROJECT_VERSION})

    if(PROJECT_VERSION_MAJOR OR PROJECT_VERSION_MINOR)
      if("${PROJECT_COMPATIBILITY}" STREQUAL "AnyNewerVersion")
        set(PROJECT_SOVERSION "")
      elseif("${PROJECT_COMPATIBILITY}" STREQUAL "SameMajorVersion")
        set(PROJECT_SOVERSION "${PROJECT_VERSION_MAJOR}")
      elseif("${PROJECT_COMPATIBILITY}" STREQUAL "SameMinorVersion")
        set(PROJECT_SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
      elseif("${PROJECT_COMPATIBILITY}" STREQUAL "ExactVersion")
        # Note: according to cmake doc ExactVersion still ignores tweak version
        set(PROJECT_SOVERSION
            "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
        )
      else()
        message(FATAL_ERROR "Unknown compatibility mode '${PROJECT_COMPATIBILITY}'")
      endif()

      set_target_properties(${CURRENT_TARGET} PROPERTIES SOVERSION "${PROJECT_SOVERSION}")
    endif()

    set("${PROJECT_NAME}_INSTALL_CMAKEDIR"
        "${INSTALL_DIR_FOR_CMAKE_CONFIGS}/cmake/${PROJECT_NAME}${PROJECT_VERSION_SUFFIX}"
        CACHE PATH "CMake package config location relative to the install prefix"
    )

    # Name target files same as the exported targets (without namespace)
    get_target_property(EXP_NAME ${CURRENT_TARGET} EXPORT_NAME)
    if(NOT EXP_NAME)
      set(EXP_NAME "${CURRENT_TARGET}")
    endif()

    list(APPEND PACKAGE_TARGET_NAMES "${EXP_NAME}")
    list(APPEND PACKAGE_TARGET_FILES "${EXP_NAME}Targets.cmake")

    install(
      TARGETS ${CURRENT_TARGET}
      EXPORT ${EXP_NAME}Targets
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_RUNTIME_DESTINATION}
              COMPONENT "${PROJECT_NAME}_Runtime"
              NAMELINK_COMPONENT "${PROJECT_NAME}_Development"
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_RUNTIME_DESTINATION}
              COMPONENT "${PROJECT_NAME}_Development"
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/${PROJECT_RUNTIME_DESTINATION}
              COMPONENT "${PROJECT_NAME}_Runtime"
      BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}/${PROJECT_RUNTIME_DESTINATION}
             COMPONENT "${PROJECT_NAME}_Runtime"
      PUBLIC_HEADER DESTINATION ${PROJECT_INCLUDE_DESTINATION}
                    COMPONENT "${PROJECT_NAME}_Development"
      INCLUDES
      DESTINATION "${PROJECT_INCLUDE_DESTINATION}"
    )

    install(
      EXPORT ${EXP_NAME}Targets
      DESTINATION "${${PROJECT_NAME}_INSTALL_CMAKEDIR}"
      NAMESPACE ${PROJECT_NAMESPACE}
      COMPONENT "${PROJECT_NAME}_Development"
    )
  endforeach()

  mark_as_advanced("${PROJECT_NAME}_INSTALL_CMAKEDIR")

  configure_package_config_file(
    ${PACKAGE_PROJECT_ROOT_PATH}/Config.cmake.in
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWER}-config.cmake"
    INSTALL_DESTINATION "${${PROJECT_NAME}_INSTALL_CMAKEDIR}"
  )

  install(
    FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWER}-config-version.cmake"
          "${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWER}-config.cmake"
    DESTINATION "${${PROJECT_NAME}_INSTALL_CMAKEDIR}"
    COMPONENT "${PROJECT_NAME}_Development"
  )

  if(NOT DEFINED PROJECT_INCLUDE_HEADER_PATTERN)
    set(PROJECT_INCLUDE_HEADER_PATTERN "*")
  endif()

  if(PROJECT_INCLUDE_DESTINATION AND PROJECT_INCLUDE_DIR)
    install(
      DIRECTORY ${PROJECT_INCLUDE_DIR}/
      DESTINATION ${PROJECT_INCLUDE_DESTINATION}
      COMPONENT "${PROJECT_NAME}_Development"
      FILES_MATCHING
      PATTERN "${PROJECT_INCLUDE_HEADER_PATTERN}"
    )
  endif()

  set(${PROJECT_NAME}_VERSION
      ${PROJECT_VERSION}
      CACHE INTERNAL ""
  )

  if(PROJECT_CPACK)
    # TODO: Make use of CPackComponent?
    if(CPACK_PACKAGE_NAMESPACE)
      set(CPACK_PACKAGE_NAME ${CPACK_PACKAGE_NAMESPACE}-${PROJECT_NAME})
    else()
      set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
    endif()
    if(NOT CPACK_PACKAGE_DESCRIPTION_SUMMARY)
      set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    endif()
    if(NOT CPACK_PACKAGE_HOMEPAGE_URL)
      set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
    endif()
    set(CPACK_VERBATIM_VARIABLES YES)
    set(CPACK_THREADS 0)
    set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
      set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    endif()

    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
        OWNER_READ
        OWNER_WRITE
        OWNER_EXECUTE
        GROUP_READ
        GROUP_EXECUTE
        WORLD_READ
        WORLD_EXECUTE
    )

    include(CPack)
  endif()
endfunction()
