cmake_minimum_required(VERSION 2.8.7)
if(POLICY CMP0046)
  cmake_policy(SET CMP0046 NEW)
endif()
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()

# ---[ Peloton project
project(Peloton CXX C)

# ---[ CTest
include(CTest)

# ---[ Peloton version
set(PELOTON_TARGET_VERSION "0.0.5" CACHE STRING "Peloton logical version")
set(PELOTON_TARGET_SOVERSION "0.0.5" CACHE STRING "Peloton soname version")
add_definitions(-DPELOTON_VERSION=${PELOTON_TARGET_VERSION})

# --[ Output directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---[ Using cmake scripts and modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

include(ExternalProject)

include(cmake/Utils.cmake)
include(cmake/Targets.cmake)
include(cmake/Misc.cmake)
include(cmake/CompilerInfo.cmake)
include(cmake/Summary.cmake)
include(cmake/ConfigGen.cmake)

# ---[ Options
peloton_option(BUILD_docs   "Build documentation" ON IF UNIX OR APPLE)

# ---[ Dependencies
include(cmake/Dependencies.cmake)

# ---[ C++11 Flags
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)

if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-c++14-compat")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -faligned-new")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# Create a new pre-processor macro __PELOTONFILE__ that has a truncated
# path to reduce the size of the debug log messages.
# Source: http://stackoverflow.com/a/16658858
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__PELOTONFILE__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

# ---[ Color diagnostics

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND
  (CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.9 OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9))
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=auto")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
endif()

# -- [ Debug Flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -fno-omit-frame-pointer -fno-optimize-sibling-calls")

# ---[ Flags
if(UNIX OR APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wextra -Werror -mcx16 -Wno-invalid-offsetof -march=native")
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-tautological-undefined-compare -Wno-unused-private-field -Wno-braced-scalar-init -Wno-constant-conversion -Wno-potentially-evaluated-expression -Wno-infinite-recursion -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -I/usr/local/opt/openssl/include")
    endif()
# ---[ close$NOCANCEL for Mac OS
    if(APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-dollar-in-identifier-extension")
    endif()
endif()

# There is a problem with building on g++5.4 on Ubuntu 17.10 where
# the compiler will not support a bunch of stuff in std for some reason.
# For example, it would say that 'to_string' is not part of 'std'.
# These flags fix these problems. I don't know whether g++5.5 has the same
# problem, so I am only making this fix target all variants of 5.4.
#
# More Info: https://stackoverflow.com/a/38034394
#            https://github.com/opencv/opencv/issues/10032#issuecomment-342347482
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND
  (CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 5.4 OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.4) AND
  (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.5))
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GLIBCXX_USE_C99=1 -D_GLIBCXX_USE_C99_MATH=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_C99=1 -D_GLIBCXX_USE_C99_MATH=1")
endif()

# ---[ Warnings
peloton_warnings_disable(CMAKE_CXX_FLAGS -Wno-strict-aliasing -Wno-implicit-fallthrough)

# ---[ Check if we should use the GNU Gold linker
set(USE_GOLD true CACHE BOOL "Use the GNU Gold linker if available")
if (USE_GOLD)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)
    if ("${LD_VERSION}" MATCHES "GNU gold")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold -Wl,--disable-new-dtags")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold -Wl,--disable-new-dtags")
    else ()
        message(WARNING "GNU gold linker isn't available, using the default system linker.")
        set(USE_LD_GOLD OFF)
    endif ()
endif()

# ---[ Turn on sanitizers if necessary.
if(USE_SANITIZER)
    if (USE_SANITIZER STREQUAL "Address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    elseif (USE_SANITIZER STREQUAL "Undefined")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-sanitize=vptr,function -fno-sanitize-recover=all")
    elseif (USE_SANITIZER STREQUAL "Thread")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    elseif (USE_SANITIZER STREQUAL "Address;Undefined" OR
            USE_SANITIZER STREQUAL "Undefined;Address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -fno-sanitize=vptr,function -fno-sanitize-recover=all")
    else()
        message(FATAL_ERROR "Unsupported value of USE_SANITIZER: ${USE_SANITIZER}")
    endif()
    if (USE_SANITIZER MATCHES "(Undefined;)?Address(;Undefined)?")
        check_cxx_compiler_flag(-fsanitize-address-use-after-scope FSANITIZE_USE_AFTER_SCOPE_FLAG)
        if (${FSANITIZE_USE_AFTER_SCOPE_FLAG})
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-address-use-after-scope")
        endif()
    endif()
endif()

# -- [ Coverage
option(COVERALLS "Generate coveralls data" OFF)

if (COVERALLS)
    include(Coveralls)
    coveralls_turn_on_coverage()

    # Create the coveralls target.
    file(GLOB_RECURSE srcs ${PROJECT_SOURCE_DIR}/src/*/*.cpp)

    peloton_convert_absolute_paths(srcs)
    peloton_convert_absolute_paths(parser_srcs)

    #message(STATUS "Coverage srcs : ${srcs}" )
    set(COVERAGE_SRCS ${srcs})

    coveralls_setup("${COVERAGE_SRCS}" ON)
endif()

# data directory for SSL
set(DATA_DIR "${PROJECT_SOURCE_DIR}/data")

# ---[ Config generation
configure_file(cmake/Templates/peloton_config.h.in "${PROJECT_BINARY_DIR}/peloton_config.h")

# ---[ Includes
set(Peloton_SRC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/include)
set(Peloton_TEST_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/test/include)
set(Peloton_THIRD_PARTY_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/third_party)
include_directories(${Peloton_SRC_INCLUDE_DIR} ${Peloton_TEST_INCLUDE_DIR} ${Peloton_THIRD_PARTY_INCLUDE_DIR} ${PROJECT_BINARY_DIR})
include_directories(BEFORE src) # This is needed for gtest.

# ---[ Subdirectories
add_subdirectory(src)
add_subdirectory(test)

# -- [ Doxygen

if (DOXYGEN)
	include(doxygen)
	doxygen_configure(src/ test/)
endif()

# ---[ Linter target
add_custom_target(lint COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/cmake/lint.cmake)

# ---[ Configuration summary
peloton_print_configuration_summary()

# ---[ Export configs generation
peloton_generate_export_configs()
