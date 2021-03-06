cmake_minimum_required(VERSION 2.6)
# project name
project(ProbReach)

# checking for packages
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# setting compiler flags
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wno-sign-compare -Wno-unused-variable -Wno-return-type -Wno-unused-function --std=c++11 -fopenmp -frounding-math")
else ()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wno-sign-compare -Wno-unused-variable -Wno-return-type -Wno-unused-function --std=c++11 -fopenmp -frounding-math -static")
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif()

# setting some environment variables
set(PROBREACH_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(PROBREACH_BINARY_DIR ${PROJECT_SOURCE_DIR}/../build/release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROBREACH_BINARY_DIR})

# checking if dReal binary is set
if(NOT DEFINED DREAL_DIR)
    message(WARNING "The variable DREAL_DIR is not defined. The default value \"/usr/local/src/dreal\" will be set")
    set(DREAL_DIR /usr/local/src/dreal)
endif(NOT DEFINED DREAL_DIR)

if(NOT DEFINED DREAL_REL_DIR)
    if(EXISTS ${DREAL_DIR}/build/release)
        set(DREAL_REL_DIR ${DREAL_DIR}/build/release)
    elseif(EXISTS ${DREAL_DIR}/build/minsizerel)
        set(DREAL_REL_DIR ${DREAL_DIR}/build/minsizerel)
    else(EXISTS ${DREAL_DIR}/build/release)
        message(FATAL_ERROR "dReal build not was not found at ${DREAL_DIR}/build/release or ${DREAL_DIR}/build/minsizerel. Please set the variable DREAL_REL_DIR with the full path to dReal build")
    endif(EXISTS ${DREAL_DIR}/build/release)
endif(NOT DEFINED DREAL_REL_DIR)

# setting ProbReach version
set(PROBREACH_VERSION_MAJOR 1)
set(PROBREACH_VERSION_MINOR 3)
set(PROBREACH_VERSION_PATCH 0)
set(PROBREACH_VERSION ${PROBREACH_VERSION_MAJOR}.${PROBREACH_VERSION_MINOR}.${PROBREACH_VERSION_PATCH})
configure_file(
  "${PROBREACH_SOURCE_DIR}/version.h.in"
  "${PROBREACH_SOURCE_DIR}/version.h"
  )

# setting solver binary
set(DREAL_BIN ${DREAL_DIR}/bin/dReal)
configure_file(
  "${PROBREACH_SOURCE_DIR}/config.h.in"
  "${PROBREACH_SOURCE_DIR}/config.h"
  )

# creating a directory for the lexer and parser files
add_custom_target(pdrhparser
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROBREACH_BINARY_DIR}/parser/pdrh
    COMMENT "Creating directory ${PROBREACH_BINARY_DIR}/parser/pdrh")

# create custom command for flex/lex (note the outputs)
add_custom_command(
  COMMAND ${FLEX_EXECUTABLE}
  ARGS -o ${PROBREACH_BINARY_DIR}/parser/pdrh/pdrhlexer.c
  ${PROBREACH_SOURCE_DIR}/parser/pdrh/pdrhlexer.l
  DEPENDS pdrhparser
  DEPENDS ${PROBREACH_SOURCE_DIR}/parser/pdrh/pdrhlexer.l
  OUTPUT  ${PROBREACH_BINARY_DIR}/parser/pdrh/pdrhlexer.c
  COMMENT "Generating ${PROBREACH_BINARY_DIR}/parser/pdrh/pdrhlexer.c"
  )

# create custom command for bison/yacc (note the DEPENDS)
add_custom_command(
  COMMAND ${BISON_EXECUTABLE}
  ARGS -d -y ${PROBREACH_SOURCE_DIR}/parser/pdrh/pdrhparser.y
  -o ${PROBREACH_BINARY_DIR}/parser/pdrh/pdrhparser.c
  DEPENDS ${PROBREACH_BINARY_DIR}/parser/pdrh/pdrhlexer.c
  DEPENDS ${PROBREACH_SOURCE_DIR}/parser/pdrh/pdrhparser.y
  OUTPUT  ${PROBREACH_BINARY_DIR}/parser/pdrh/pdrhparser.c
  COMMENT "Generating ${PROBREACH_BINARY_DIR}/parser/pdrh/pdrhparser.c"
  )

# setting propeties for the generated lexer and parser files
set_source_files_properties(${PROBREACH_BINARY_DIR}/parser/pdrh/pdrhparser.c GENERATED PROPERTIES LANGUAGE CXX)
set_source_files_properties(${PROBREACH_BINARY_DIR}/parser/pdrh/pdrhlexer.c GENERATED PROPERTIES LANGUAGE CXX)

# header files
include_directories("${DREAL_REL_DIR}/include/ibex")
include_directories("${DREAL_REL_DIR}/include")
include_directories("${PROBREACH_SOURCE_DIR}")
include_directories("${PROBREACH_SOURCE_DIR}/util")
include_directories("${PROBREACH_SOURCE_DIR}/solver")
include_directories("${PROBREACH_SOURCE_DIR}/parser/csv")
include_directories("${PROBREACH_SOURCE_DIR}/parser/pdrh")
include_directories("${PROBREACH_SOURCE_DIR}/logging")
include_directories("${PROBREACH_SOURCE_DIR}/xml")
include_directories("${PROBREACH_BINARY_DIR}/parser/pdrh")

# putting all source files together
file(GLOB SOURCES   ${PROBREACH_SOURCE_DIR}/util/*.cpp
                    ${PROBREACH_SOURCE_DIR}/xml/*.cpp
                    ${PROBREACH_SOURCE_DIR}/parser/csv/*.cpp
                    ${PROBREACH_SOURCE_DIR}/solver/*.cpp
                    ${PROBREACH_SOURCE_DIR}/main.cpp)

set(SOURCES ${SOURCES} ${PROBREACH_BINARY_DIR}/parser/pdrh/pdrhparser.c ${PROBREACH_BINARY_DIR}/parser/pdrh/pdrhlexer.c)

# adding executable
add_executable(ProbReach-bin ${SOURCES})
SET_TARGET_PROPERTIES(ProbReach-bin  PROPERTIES OUTPUT_NAME ProbReach)

# adding library
add_library(ProbReach ${SOURCES})

# creating a bin folder and copying ProbReach binary there
add_custom_command(TARGET ProbReach-bin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROBREACH_SOURCE_DIR}/../bin
    COMMAND ${CMAKE_COMMAND} -E copy ${PROBREACH_BINARY_DIR}/ProbReach ${PROBREACH_SOURCE_DIR}/../bin
    COMMENT "Copying ${PROBREACH_BINARY_DIR}/ProbReach to ${PROBREACH_SOURCE_DIR}/../bin")

# creating a lib folder and copying ProbReach library there
add_custom_command(TARGET ProbReach POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROBREACH_SOURCE_DIR}/../lib
    COMMAND ${CMAKE_COMMAND} -E copy ${PROBREACH_SOURCE_DIR}/../libProbReach.a ${PROBREACH_SOURCE_DIR}/../lib
    COMMAND ${CMAKE_COMMAND} -E remove ${PROBREACH_SOURCE_DIR}/../libProbReach.a
    COMMENT "Copying ${PROBREACH_SOURCE_DIR}/../libProbReach.a to ${PROBREACH_SOURCE_DIR}/../lib")

# linking libraries
target_link_libraries(ProbReach-bin ${DREAL_REL_DIR}/lib/libcapd.a)
target_link_libraries(ProbReach-bin ${DREAL_REL_DIR}/lib/libibex.a)
target_link_libraries(ProbReach-bin ${DREAL_REL_DIR}/lib/libprim.a)
# flag for FLEX and GSL
target_link_libraries(ProbReach-bin -lfl -lgsl)

