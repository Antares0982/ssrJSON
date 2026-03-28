cmake_minimum_required(VERSION 3.18)
project(ssrjson LANGUAGES C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

# ------------------------------------------------------------------------------
# Build Options for tests and docs
option(BUILD_FREE_THREADING "Build with free-threading support" OFF)
option(FREE_THREADING_LOCKFREE "Use lock-free free-threading encoding" OFF)
option(ASAN_ENABLED "Build with asan" OFF)
option(BUILD_COVERAGE "Build with coverage" OFF)
option(BUILD_CTESTS "Build tests" ON)
option(BUILD_SHIPPING_SIMD "Build all simd for shipping" ON)
option(BUILD_AVX512 "Force build avx512, non-shipping" OFF)
option(BUILD_AVX2 "Force build avx2, non-shipping" OFF)
option(BUILD_SSE4 "Force build sse4, non-shipping" OFF)
option(BUILD_NATIVE "Force build with -march=native, non-shipping" OFF)
option(SEARCH_PYTHON3_USE_ENV
       "Search python3 using environment variables without cmake way" OFF)
option(USE_LTO "Use LTO" ON)
option(BUILD_FUZZER "Build fuzzer" OFF)

if(BUILD_SHIPPING_SIMD)
  if(BUILD_AVX512
     OR BUILD_AVX2
     OR BUILD_SSE4
     OR BUILD_NATIVE)
    message(
      FATAL_ERROR
        "BUILD_SHIPPING_SIMD cannot be enabled with BUILD_AVX512, BUILD_AVX2, BUILD_SSE4 or BUILD_NATIVE"
    )
  endif()

  set(BUILD_MULTI_LIB 1)
else()
  if((BUILD_AVX512
      OR BUILD_AVX2
      OR BUILD_SSE4)
     AND BUILD_NATIVE)
    message(
      FATAL_ERROR
        "BUILD_AVX512, BUILD_AVX2, BUILD_SSE4 cannot be enabled with BUILD_NATIVE"
    )
  endif()

  set(BUILD_MULTI_LIB 0)
endif()

# ------------------------------------------------------------------------------
# Project Config
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerFeatures)

if(NOT (CMAKE_C_COMPILER_ID MATCHES Clang))
  message(FATAL_ERROR "CMAKE_C_COMPILER_ID must be Clang (got: ${CMAKE_C_COMPILER_ID})")
endif()
if(WIN32)
  if(NOT (CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    message(FATAL_ERROR "On Windows, only clang-cl is supported (frontend variant must be MSVC, got: ${CMAKE_C_COMPILER_FRONTEND_VARIANT})")
  endif()
else()
  if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    message(FATAL_ERROR "On Linux/Apple, only clang is supported (clang-cl/MSVC frontend is not allowed)")
  endif()
endif()
# ------------------------------------------------------------------------------
# Search Python Package
if(SEARCH_PYTHON3_USE_ENV)
  set(Python3_INCLUDE_DIRS $ENV{Python3_INCLUDE_DIR})
  set(Python3_LIBRARIES $ENV{Python3_LIBRARY})
  if(DEFINED ENV{Python3_EXECUTABLE})
    set(Python3_EXECUTABLE $ENV{Python3_EXECUTABLE})
  else()
    find_program(Python3_EXECUTABLE NAMES python3 python REQUIRED)
  endif()
else()
  find_package(
    Python3
    COMPONENTS Interpreter Development
    REQUIRED)
endif()

# check for python3
if((NOT Python3_INCLUDE_DIRS) OR (NOT Python3_LIBRARIES))
  message(FATAL_ERROR "Python3 not found")
endif()

message("Python3_INCLUDE_DIRS = ${Python3_INCLUDE_DIRS}")
message("Python3_LIBRARIES = ${Python3_LIBRARIES}")

# ------------------------------------------------------------------------------
# Build Type
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  if(NOT CMAKE_BUILD_TYPE)
    if(BUILD_COVERAGE)
      message(
        STATUS "No build type selected, default to: Debug (BUILD_COVERAGE)")
      set(CMAKE_BUILD_TYPE Debug)
    else()
      message(STATUS "No build type selected, default to: Release")
      set(CMAKE_BUILD_TYPE Release)
    endif()
  endif()
endif()

# ------------------------------------------------------------------------------
# Global settings
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------------
# Libraries

# common definitions
add_library(commonBuild INTERFACE)
target_link_libraries(commonBuild INTERFACE)
target_include_directories(
  commonBuild INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
target_include_directories(commonBuild SYSTEM INTERFACE ${Python3_INCLUDE_DIRS})
if(BUILD_FREE_THREADING)
  message("GIL: Disabled")
  target_compile_definitions(commonBuild INTERFACE Py_GIL_DISABLED=1)
  if(FREE_THREADING_LOCKFREE)
    message("Lock-free: Enabled")
    target_compile_definitions(commonBuild
                               INTERFACE SSRJSON_FREE_THREADING_LOCKFREE=1)
  endif()
endif()
target_compile_definitions(commonBuild
                           INTERFACE BUILD_MULTI_LIB=${BUILD_MULTI_LIB})
if(WIN32)
  target_compile_options(commonBuild INTERFACE /FAcs)
else()
  target_compile_options(commonBuild INTERFACE -fvisibility=hidden
                                               -fvisibility-inlines-hidden)
endif()

if(USE_LTO)
  target_compile_options(commonBuild INTERFACE -flto)
  target_link_options(commonBuild INTERFACE -flto)
endif()

if(BUILD_COVERAGE)
  target_compile_definitions(commonBuild INTERFACE SSRJSON_COVERAGE=1)
endif()

# custom target: update SCM file
if(DEFINED PREDEFINED_VERSION)
  add_custom_target(
    update_version_header ALL
    COMMAND ${Python3_EXECUTABLE} dev_tools/scm.py src/version.h.in --version
            ${PREDEFINED_VERSION}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Update SCM file")
else()
  add_custom_target(
    update_version_header ALL
    COMMAND ${Python3_EXECUTABLE} dev_tools/scm.py src/version.h.in
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Update SCM file")
endif()

add_dependencies(commonBuild update_version_header)

if(BUILD_AVX512)
  add_avx512_compile_option(commonBuild INTERFACE)
endif()

if(BUILD_AVX2)
  add_avx2_compile_option(commonBuild INTERFACE)
endif()

if(BUILD_SSE4)
  add_sse4_compile_option(commonBuild INTERFACE)
endif()

if(BUILD_NATIVE)
  add_native_compile_option(commonBuild INTERFACE)
  target_compile_definitions(commonBuild INTERFACE SSRJSON_BUILD_NATIVE=1)
endif()

message("CMAKE_C_COMPILER_ID = ${CMAKE_C_COMPILER_ID}")
message("CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message("CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(^aarch64)|(^arm64)|(^ARM64)")
  set(SSRJSON_DETECT_ARCH 1)
  set(TARGET_SIMD_ARCH aarch)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
  set(SSRJSON_DETECT_ARCH 1)
  set(TARGET_SIMD_ARCH aarch)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(x86_64)|(AMD64|amd64)|(^i.86$)")
  set(SSRJSON_DETECT_ARCH 1)
  set(TARGET_SIMD_ARCH x86)
else()
  set(SSRJSON_DETECT_ARCH 0)
endif()

if(SSRJSON_DETECT_ARCH)
  if("${TARGET_SIMD_ARCH}" STREQUAL "x86")
    target_compile_definitions(commonBuild INTERFACE SSRJSON_IS_X64=1)
  elseif("${TARGET_SIMD_ARCH}" STREQUAL "aarch")
    target_compile_definitions(commonBuild INTERFACE SSRJSON_IS_AARCH64=1)
  endif()
else()
  message(FATAL_ERROR "Cannot detect architecture")
endif()

if(ASAN_ENABLED)
  message("ENABLE ASAN")
  target_compile_definitions(commonBuild INTERFACE SSRJSON_ASAN_CHECK=1)

  add_asan_compile_option(commonBuild INTERFACE)
endif()

# ------------------------------------------------------------------------------
# setting up ssrjson

# set ssrjson src files
set(SRC_FILES
    src/ssrjson.c
    src/tls.c
    src/pyutils.c
    src/utils/decode_utils.c
    src/utils/encode_utils.c
    src/utils/float_tables.c
    src/utils/mask_table.c)

set(SRC_WITH_SIMD src/decode/decode.c src/encode/encode.c src/simd/cvt.c)

set(SRC_XJB src/xjb/xjb.cpp)
add_library(xjb OBJECT ${SRC_XJB})
target_link_libraries(xjb PUBLIC commonBuild)

# library ssrjson
if(BUILD_MULTI_LIB)
  set(SRC_FILES ${SRC_FILES} src/multilib.c)

  if("${TARGET_SIMD_ARCH}" STREQUAL "x86")
    add_library(_ssrjson_common OBJECT ${SRC_FILES})
    target_link_libraries(_ssrjson_common PUBLIC commonBuild)
    target_compile_definitions(_ssrjson_common PUBLIC SSRJSON_EXPORTS=1)
    target_compile_definitions(_ssrjson_common
                               PRIVATE DISABLE_INTERNAL_NOINLINE=1)
    add_library(ssrjson_avx512 OBJECT ${SRC_WITH_SIMD})
    target_link_libraries(ssrjson_avx512 PUBLIC _ssrjson_common)
    add_library(ssrjson_avx2 OBJECT ${SRC_WITH_SIMD})
    target_link_libraries(ssrjson_avx2 PUBLIC _ssrjson_common)

    add_library(ssrjson_sse4 OBJECT ${SRC_WITH_SIMD})
    target_link_libraries(ssrjson_sse4 PUBLIC _ssrjson_common)
    add_library(
      ssrjson SHARED
      $<TARGET_OBJECTS:ssrjson_avx512> $<TARGET_OBJECTS:ssrjson_avx2>
      $<TARGET_OBJECTS:ssrjson_sse4> $<TARGET_OBJECTS:_ssrjson_common>
      $<TARGET_OBJECTS:xjb>)
    target_link_libraries(ssrjson PUBLIC commonBuild)

    add_avx512_compile_option(ssrjson_avx512)
    add_avx2_compile_option(ssrjson_avx2)
    add_sse4_compile_option(ssrjson_sse4)

    if(BUILD_COVERAGE)
      add_coverage_flags(_ssrjson_common)
      add_coverage_flags(ssrjson_avx512)
      add_coverage_flags(ssrjson_avx2)
      add_coverage_flags(ssrjson_sse4)
    endif()

  elseif("${TARGET_SIMD_ARCH}" STREQUAL "aarch")
    add_library(_ssrjson_common OBJECT ${SRC_FILES})
    target_link_libraries(_ssrjson_common PUBLIC commonBuild)
    target_compile_definitions(_ssrjson_common PUBLIC SSRJSON_EXPORTS=1)
    target_compile_definitions(_ssrjson_common
                               PRIVATE DISABLE_INTERNAL_NOINLINE=1)
    add_library(ssrjson_neon OBJECT ${SRC_WITH_SIMD})
    target_link_libraries(ssrjson_neon PUBLIC _ssrjson_common)
    add_library(
      ssrjson SHARED
      $<TARGET_OBJECTS:ssrjson_neon> $<TARGET_OBJECTS:_ssrjson_common>
      $<TARGET_OBJECTS:_ssrjson_common> $<TARGET_OBJECTS:xjb>)
    target_link_libraries(ssrjson PUBLIC commonBuild)

    if(BUILD_COVERAGE)
      add_coverage_flags(_ssrjson_common)
      add_coverage_flags(ssrjson_neon)
    endif()

  else()
    message(FATAL_ERROR "TARGET_SIMD_ARCH=${TARGET_SIMD_ARCH} not supported")
  endif()
else()
  set(SRC_FILES ${SRC_FILES} src/singlelib.c)
  add_library(ssrjson SHARED ${SRC_FILES} ${SRC_WITH_SIMD} ${SRC_XJB})
  target_compile_definitions(ssrjson PUBLIC SSRJSON_EXPORTS=1)
  target_link_libraries(ssrjson PUBLIC commonBuild)
endif()

set_target_properties(ssrjson PROPERTIES PREFIX "")

if(BUILD_COVERAGE)
  add_coverage_flags(ssrjson)
endif()

if(ASAN_ENABLED)
  target_link_libraries(ssrjson PRIVATE asan)
endif()

if(ASAN_ENABLED AND WIN32)
  message(FATAL_ERROR "ASAN_ENABLED not supported on Windows")
endif()

# ------------------------------------------------------------------------------
if(APPLE)
  set(SSRJSON_FLAGS)
  list(
    APPEND
    SSRJSON_FLAGS
    "-Wall"
    "-Wextra"
    "-Werror"
    "-pedantic"
    "-pedantic-errors"
    "-Wno-psabi"
    "-Wno-language-extension-token"
    "-fno-stack-protector")

  if(CMAKE_BUILD_TYPE MATCHES "Release")
    list(APPEND SSRJSON_FLAGS "-fomit-frame-pointer")
  endif()

elseif(WIN32)
  set(SSRJSON_FLAGS)
  list(
    APPEND
    SSRJSON_FLAGS
    "/utf-8"
    "/W4"
    "/WX"
    "-Wno-unused-parameter"
    "-Wno-unused-variable"
    "-Wno-unused-label"
    "-Wno-unused-function"
    "-Wno-uninitialized"
    "-Wno-unused-but-set-variable"
    "-Wno-deprecated-declarations"
    "-Wno-visibility"
    "-Wno-language-extension-token"
    "/GS-")

  target_compile_options(commonBuild INTERFACE ${SSRJSON_FLAGS})

else()
  set(SSRJSON_FLAGS)

  list(
    APPEND
    SSRJSON_FLAGS
    "-Wall"
    "-Wextra"
    "-Wno-unknown-warning-option"
    "-Wno-constant-logical-operand" # https://github.com/llvm/llvm-project/issues/63963
    "-Wno-unused-parameter"
    "-Wno-unused-variable"
    "-Wno-unused-label"
    "-Wno-unused-function"
    "-Wno-uninitialized"
    "-Wno-unused-but-set-variable"
    "-Wno-deprecated-declarations"
    "-Wno-visibility"
    "-Wno-language-extension-token"
    "-fno-stack-protector")

  if(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    message(
      "Release mode with debug info, enabling maximal optimization and debug symbols"
    )
    target_compile_options(commonBuild INTERFACE -Werror -Wno-psabi -pedantic
                                                 -pedantic-errors)
  elseif(CMAKE_BUILD_TYPE MATCHES ".*Rel.*")
    message("Release mode, enabling maximal optimization")
    target_compile_options(commonBuild INTERFACE -Werror -Wno-psabi -pedantic
                                                 -pedantic-errors)
    list(APPEND SSRJSON_FLAGS "-fomit-frame-pointer")
  else()
    target_compile_options(commonBuild INTERFACE -O0 -Wno-psabi)

    message("Debug mode, enabling debug symbols")
  endif()

  target_compile_options(commonBuild INTERFACE ${SSRJSON_FLAGS})
endif()

# ------------------------------------------------------------------------------
# Install
if(APPLE)
  set_target_properties(ssrjson PROPERTIES SUFFIX ".so")
  target_link_options(ssrjson PRIVATE "-flat_namespace" "-undefined" "suppress")
elseif(WIN32)
  set_target_properties(ssrjson PROPERTIES SUFFIX ".pyd")
  target_link_libraries(ssrjson PRIVATE ${Python3_LIBRARIES})
endif(APPLE)

install(TARGETS ssrjson LIBRARY DESTINATION .)

if(BUILD_CTESTS)
  include(CTests)
endif()
