cmake_minimum_required(VERSION 3.0.0)

set(VERSION_MAJOR
    1
    CACHE STRING "Project major version number")
set(VERSION_MINOR
    "1"
    CACHE STRING "Project minor version number")
set(VERSION_PATCH
    "0"
    CACHE STRING "Project patch version number")
set(CN_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH CN_VERSION)

project("cn-cbor" VERSION "${CN_VERSION}")

# ##############################################################################
# OPTIONS
# ##############################################################################

option(CN_CBOR_USE_CONTEXT "Use context pointer for CBOR functions" OFF)
option(CN_CBOR_VERBOSE "Produce verbose makefile output" OFF)
option(CN_CBOR_OPTIMIZE "Optimize for size" OFF)
option(CN_CBOR_FATAL_WARNINGS "Treat build warnings as errors" ON)
option(CN_CBOR_COVERALLS "Generate coveralls data" ON)
option(CN_CBOR_COVERALLS_SEND "Send data to coveralls site" OFF)
option(CN_CBOR_BUILD_DOCS "Create docs using Doxygen" OFF)
option(CN_CBOR_BUILD_TESTS "Create tests" ON)
option(CN_CBOR_NO_FLOATS "Build without floating point support" OFF)
option(CN_CBOR_ALIGN_READS "Use memcpy in ntoh*p()" OFF)
option(CN_CBOR_RUN_CLANG_TIDY "Use Clang-Tidy for static analysis" OFF)

if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()

set(dist_dir ${CMAKE_BINARY_DIR}/dist)
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX}/bin)
set(libdir ${CMAKE_INSTALL_PREFIX}/lib)
set(includedir ${CMAKE_INSTALL_PREFIX}/include)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cn-cbor.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/cn-cbor.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cn-cbor.pc DESTINATION lib/pkgconfig)

set(package_prefix "${CMAKE_PACKAGE_NAME}-${CMAKE_SYSTEM_NAME}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${dist_dir}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${dist_dir}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${dist_dir}/lib)

if(NOT CMAKE_BUILD_TYPE)
  if(CN_CBOR_OPTIMIZE)
    set(CMAKE_BUILD_TYPE MinSizeRel)
    set(CN_CBOR_COVERALLS OFF)
    set(CN_CBOR_COVERALLS_SEND OFF)
  else()
    set(CMAKE_BUILD_TYPE Debug)
  endif()
endif()

if(MSVC)
  set(CN_CBOR_COVERALLS OFF)
endif()

message("Build type: ${CMAKE_BUILD_TYPE}")

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(STATUS "adding GCC/Clang options ")
  add_definitions(-std=gnu99 -Wall -Wextra -pedantic)
  if(CN_CBOR_FATAL_WARNINGS)
    add_definitions(-Werror)
  endif()
  if(CN_CBOR_OPTIMIZE)
    add_definitions(-Os)
  endif()
elseif(MSVC)
  add_definitions(/W3)
  if(CN_CBOR_FATAL_WARNINGS)
    add_definitions(/WX)
  endif()
else()
  message(FATAL_ERROR "unhandled compiler id: ${CMAKE_C_COMPILER_ID}")
endif()

if(CN_CBOR_NO_FLOATS)
  add_definitions(-DCBOR_NO_FLOAT)
endif()

if(CN_CBOR_VERBOSE)
  set(CMAKE_VERBOSE_MAKEFILE ON)
endif()

# ##############################################################################
# LIBRARY
# ##############################################################################

set(cbor_srcs src/cn-cbor.c src/cn-create.c src/cn-encoder.c src/cn-error.c
              src/cn-get.c src/cn-print.c)

if(CN_CBOR_ALIGN_READS)
  add_definitions(-DCBOR_ALIGN_READS)
endif()
if(CN_CBOR_USE_CONTEXT)
  add_definitions(-DUSE_CBOR_CONTEXT)
endif()
# default is static, can be shared if -DBUILD_SHARED_LIBS=ON passed to cmake
add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE include/cn-cbor/cn-cbor.h ${cbor_srcs})
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
if(MSVC)
  message("test: ${BUILD_SHARED_LIBS}")
  message("CN_CBOR_VERBOSE: ${CN_CBOR_VERBOSE}")
  target_link_libraries(${PROJECT_NAME} ws2_32)
endif()
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
if(CN_CBOR_COVERALLS)
  include(Coveralls)
  coveralls_turn_on_coverage()

  set(COVERAGE_SRCS "")
  foreach(S ${cbor_srcs})
    get_filename_component(S_ABS ${S} ABSOLUTE)
    list(APPEND COVERAGE_SRCS ${S_ABS})
  endforeach()

  # Create the coveralls target.
  coveralls_setup("${COVERAGE_SRCS}" ${CN_CBOR_COVERALLS_SEND} # If we should upload.
  )

  # add_dependencies(CN_CBOR_COVERALLS, all)
endif()

if(NOT MSVC)
  add_custom_target(
    size
    COMMAND echo "${CMAKE_BINARY_DIR}/src/CMakeFiles/cn-cbor.dir/cn-cbor.c.o"
    COMMAND size "${CMAKE_BINARY_DIR}/src/CMakeFiles/cn-cbor.dir/cn-cbor.c.o"
    COMMAND size -m "${CMAKE_BINARY_DIR}/src/CMakeFiles/cn-cbor.dir/cn-cbor.c.o"
    DEPENDS cn-cbor
    COMMENT "Output the size of the parse routine")
endif(NOT MSVC)

# ##############################################################################
# DOCS
# ##############################################################################

if(CN_CBOR_BUILD_DOCS)
  find_package(Doxygen)

  if(NOT DOXYGEN_FOUND)
    message(WARNING "Doxygen is needed to build the documentation.")
  else()
    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_target(
      doc
      COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)
  endif()
endif()

###############################################################################
# DEPENDENCIES
###############################################################################

if(CN_CBOR_RUN_CLANG_TIDY)
  find_program(
    CLANG_TIDY_EXE
    NAMES "clang-tidy"
    DOC "/usr/bin/clang-tidy")
  if(NOT CLANG_TIDY_EXE)
    message(WARNING "clang-tidy not found.")
  endif()
  set_target_properties(${PROJECT_NAME} PROPERTIES C_CLANG_TIDY "${CLANG_TIDY_EXE}")
endif(CN_CBOR_RUN_CLANG_TIDY)

# ##############################################################################
# TESTS
# ##############################################################################

if(CN_CBOR_BUILD_TESTS)
  include(CTest)
  add_subdirectory(test)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
  include(LCov)
endif()

# ##############################################################################
# PACKAGING
# ##############################################################################

set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/\\\\.git/")
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/.gitignore igs)
foreach(ig IN ITEMS ${igs})
  # remove comments
  string(REGEX REPLACE "^\\s*#.*" "" ig "${ig}")
  # remove any other whitespace
  string(STRIP "${ig}" ig)
  # anything left?
  if(ig)
    # dots are literal
    string(REPLACE "." "\\\\." ig "${ig}")
    # stars are on thars
    string(REPLACE "*" ".*" ig "${ig}")
    list(APPEND CPACK_SOURCE_IGNORE_FILES "/${ig}/")
  endif()
endforeach()

set(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

include(CPack)

# ##############################################################################
# INSTALL
# ##############################################################################

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(FILES LICENSE README.md DESTINATION .)
set(CN_CBOR_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib)

install(
  EXPORT ${PROJECT_NAME}Targets
  DESTINATION ${CN_CBOR_CONFIG_INSTALL_DIR}
  NAMESPACE ${PROJECT_NAME}::
  COMPONENT dev)

configure_package_config_file(cmake/Config.cmake.in ${PROJECT_NAME}Config.cmake
                              INSTALL_DESTINATION ${CN_CBOR_CONFIG_INSTALL_DIR})
write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake
                                 COMPATIBILITY SameMajorVersion)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${CN_CBOR_CONFIG_INSTALL_DIR}
  COMPONENT dev)

install(
  DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  COMPONENT dev
  FILES_MATCHING
  PATTERN "*.h")
install(
  TARGETS cn-cbor
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)

###############################################################################
# PRINT CONFIG
###############################################################################

message(STATUS "CN_CBOR_USE_CONTEXT:..............${CN_CBOR_USE_CONTEXT}")
message(STATUS "CN_CBOR_FATAL_WARNINGS:...........${CN_CBOR_FATAL_WARNINGS}")
message(STATUS "CN_CBOR_BUILD_TESTS:..............${CN_CBOR_BUILD_TESTS}")
message(STATUS "CN_CBOR_BUILD_DOCS:...............${CN_CBOR_BUILD_DOCS}")
message(STATUS "CMAKE_BUILD_TYPE:................${CMAKE_BUILD_TYPE}")
message(STATUS "BUILD_SHARED_LIBS:...............${BUILD_SHARED_LIBS}")
message(STATUS "CN_VERSION:....................${CN_VERSION}")
message(STATUS "CMAKE_VERSION:...................${CMAKE_VERSION}")
message(STATUS "CMAKE_C_COMPILER:................${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER:..............${CMAKE_CXX_COMPILER}")
message(STATUS "CLANG_TIDY_EXE:..................${CLANG_TIDY_EXE}")
