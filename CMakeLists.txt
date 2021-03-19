# Copyright - 2015-2019 - Jan Christoph Uhde <Jan@UhdeJC.com>

cmake_minimum_required(VERSION 3.14)
project(ext-basics VERSION 0.0.1 LANGUAGES CXX)
message(STATUS "extINFO -- entering ext-basics")

## OPTIONS
option(LIBEXT_EXAMPLES "build examples" OFF)
option(LIBEXT_WARNINGS "enable warnings" ON)
option(LIBEXT_CHECKED  "user assert" ON)
option(EXT_DEFER_NON_NOTROW "allow throwing functions in defer" OFF)
option(EXT_DEFER_MOVE_ASSIGN "allow move assignment of defer object" OFF)

option(LIBEXT_TESTS "build tests" OFF)
option(LIBEXT_TESTS_NO_TIME_CRITICAL "no time critical tests" OFF)

# enable extcpp cmake
include(${CMAKE_CURRENT_LIST_DIR}/ext_cmake_enable.cmake)
include(ext_cmake_setup)

# verbose windows linking
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /VERBOSE:LIB")

if(LINUX)
    execute_process(COMMAND getconf LEVEL1_DCACHE_LINESIZE OUTPUT_VARIABLE ext_l1_cache_line_size)
    string(STRIP ${ext_l1_cache_line_size} ext_l1_cache_line_size)
    ext_log("cache line size: '${ext_l1_cache_line_size}'")
    set(EXT_OUTDIR "")
elseif(MSVC)
    set(ext_l1_cache_line_size FALSE)
    ext_log("cache line size: provided by os")
    set(EXT_OUTDIR "${PROJECT_BINARY_DIR}/build")
    ext_log("binary ouput dir: ${EXT_OUTDIR}")

    #TODO - move settings below into corresponding targets
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${EXT_OUTDIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${EXT_OUTDIR}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${EXT_OUTDIR}")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
elseif(APPLE)
    set(ext_l1_cache_line_size "64")
    ext_log("cache line size: '${ext_l1_cache_line_size}'")
else()
    set(ext_l1_cache_line_size FALSE)
endif()

# required by folder structure for XCode and VisualStudio (includes)
# sources are always required
include(src_files.cmake)
include(include_files.cmake)

### library setup
set(ext_common_compile_definitions
    $<IF:$<BOOL:${ext_is_big_endian}>,EXT_BIG_ENDIAN,EXT_LITTLE_ENDIAN>
    $<$<BOOL:${EXT_CXX_COMPILER_IS_GCC}>:EXT_GCC>
    $<$<BOOL:${EXT_CXX_COMPILER_IS_CLANG}>:EXT_CLANG>
    $<$<BOOL:${LIBEXT_CHECKED}>:EXT_CHECKED=1>
    $<$<BOOL:${LIBEXT_TESTS_NO_TIME_CRITICAL}>:EXT_TESTS_NO_TIME_CRITICAL=1>
    $<$<BOOL:${EXT_DEFER_NON_NOTROW}>:EXT_DEFER_ALLOW_NON_NOTHROW_INVOCABLE=1>
    $<$<BOOL:${EXT_DEFER_MOVE_ASSIGN}>:EXT_DEFER_ENABLE_MOVE_ASSIGN=1>
    $<$<BOOL:${ext_l1_cache_line_size}>:EXT_KNOWN_L1_CACHE_LINE_SIZE=${ext_l1_cache_line_size}>
)

find_package(Threads REQUIRED)

### define header only lib
add_library(ext-basics INTERFACE)
target_include_directories(ext-basics INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
)
target_compile_features(ext-basics INTERFACE cxx_std_17)
target_compile_options(ext-basics INTERFACE
    $<$<AND:$<CONFIG:Debug>,$<BOOL:LIBEXT_WARNINGS>>:${ext_stone-warnings}>
)
target_compile_definitions(ext-basics INTERFACE
    ${ext_common_compile_definitions}
    EXT_BASICS_HEADER_ONLY_LIB
)

# set up folder structure for XCode and VisualStudio
#set_target_properties (ext-basics PROPERTIES FOLDER libext) #not for interface libraries
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ext-basics-header} ${ext-basics-source})

add_library(ext::basics ALIAS ext-basics)

## testing
if(LIBEXT_TESTS)
    ext_log("ext-basics tests enabled")
    include(CTest)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    ext_add_test_subdirectory("google" tests)
else()
    ext_log("ext-basics tests disabled")
endif()

## add projects using this lib
if(LIBEXT_EXAMPLES)
    ext_log("ext-basics examples enabled")
    add_subdirectory(examples)
else()
    ext_log("ext-basics examples disabled")
endif()

## installation
if(COMMAND ext_install)
    set_target_properties(ext-basics PROPERTIES EXPORT_NAME basics)

    ext_install(ext-basics include/ext)

    install(TARGETS ext-basics
        EXPORT ext-basics-targets
        DESTINATION ${CMAKE_INSTALL_PREFIX}
    )
	install(
        EXPORT ext-basics-targets
		FILE ext-basics-config.cmake
        NAMESPACE ext::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake
    )
endif()

add_custom_target(
    update_version ALL
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMAND ${CMAKE_COMMAND}
                -D "EXT_GIT_VERSION_OUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/ext_version.hpp"
                -P "${ext_cmake_dir}/ext_script_git_version.cmake"
)
