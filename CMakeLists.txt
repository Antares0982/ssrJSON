cmake_minimum_required(VERSION 3.18)
project(ssrjson LANGUAGES C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

# ------------------------------------------------------------------------------
# Build Options for tests and docs
option(ASAN_ENABLED "Build with asan" OFF)
option(BUILD_COVERAGE "Build with coverage" OFF)
option(BUILD_TEST "Build tests" ON)
option(BUILD_SHIPPING_SIMD "Build all simd for shipping" ON)
option(BUILD_AVX512 "Force build avx512, non-shipping" OFF)
option(BUILD_AVX2 "Force build avx2, non-shipping" OFF)
option(BUILD_SSE4 "Force build sse4, non-shipping" OFF)
option(BUILD_NATIVE "Force build with -march=native, non-shipping" OFF)
option(SEARCH_PYTHON3_USE_ENV
       "Search python3 using environment variables without cmake way" OFF)
option(USE_LTO "Use LTO" OFF)

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
include(XcodeProperty)

if(NOT (CMAKE_C_COMPILER_ID MATCHES Clang))
  message(FATAL_ERROR "CMAKE_C_COMPILER_ID must be Clang")
endif()
# ------------------------------------------------------------------------------
# Search Python Package
if(SEARCH_PYTHON3_USE_ENV)
  set(Python3_INCLUDE_DIRS $ENV{Python3_INCLUDE_DIR})
  set(Python3_LIBRARIES $ENV{Python3_LIBRARY})
else()
  find_package(
    Python3
    COMPONENTS Development
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
target_compile_definitions(commonBuild
                           INTERFACE BUILD_MULTI_LIB=${BUILD_MULTI_LIB})
if(MSVC)
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
    COMMAND python dev_tools/scm.py src/version.h.in --version
            ${PREDEFINED_VERSION}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Update SCM file")
else()
  add_custom_target(
    update_version_header ALL
    COMMAND python dev_tools/scm.py src/version.h.in
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
    target_compile_definitions(commonBuild INTERFACE SSRJSON_X86=1)
  elseif("${TARGET_SIMD_ARCH}" STREQUAL "aarch")
    target_compile_definitions(commonBuild INTERFACE SSRJSON_AARCH=1)
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

set(SRC_DRAGON_BOX src/dragonbox/dragonbox_to_chars.cpp)

set(SRC_TEST src/ctests/tools.c src/ctests/main.c src/utils/float_tables.c)

set(SRC_TEST_WITH_SIMD src/ctests/test.c)

set(SRC_FUZZER src/ctests/fuzzer.c)

add_library(dragon_box OBJECT ${SRC_DRAGON_BOX})
target_link_libraries(dragon_box PUBLIC commonBuild)

if(BUILD_COVERAGE)
  add_coverage_flags(dragon_box)
endif()

# add library ssrjson
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
    # add_library(ssrjson_sse2 OBJECT ${SRC_WITH_SIMD})
    # target_link_libraries(ssrjson_sse2 PUBLIC _ssrjson_common)
    add_library(
      ssrjson SHARED
      $<TARGET_OBJECTS:ssrjson_avx512> $<TARGET_OBJECTS:ssrjson_avx2>
      $<TARGET_OBJECTS:ssrjson_sse4> $<TARGET_OBJECTS:_ssrjson_common>
      $<TARGET_OBJECTS:dragon_box>)
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
      $<TARGET_OBJECTS:_ssrjson_common> $<TARGET_OBJECTS:dragon_box>)
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
  add_library(ssrjson SHARED ${SRC_FILES} ${SRC_WITH_SIMD} ${SRC_DRAGON_BOX})
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

if(ASAN_ENABLED AND MSVC)
  get_filename_component(MSVC_COMPILER_DIR "${CMAKE_C_COMPILER}" PATH)
  set(ASAN_DLL_PATH "${MSVC_COMPILER_DIR}/clang_rt.asan_dynamic-x86_64.dll")

  if(EXISTS "${ASAN_DLL_PATH}")
    message(STATUS "Found AddressSanitizer runtime DLL at: ${ASAN_DLL_PATH}")

    add_custom_command(
      TARGET ssrjson
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASAN_DLL_PATH}"
              $<TARGET_FILE_DIR:ssrjson>
      COMMENT "Copying AddressSanitizer runtime DLL to output directory")
  else()
    message(
      WARNING "AddressSanitizer runtime DLL not found at: ${ASAN_DLL_PATH}")
  endif()
endif()

# ------------------------------------------------------------------------------
if(XCODE)
  set(SSRJSON_FLAGS)
  list(
    APPEND
    SSRJSON_FLAGS
    "-Wall"
    "-Wextra"
    "-Werror"
    "-pedantic"
    "-pedantic-errors"
    "-Wno-psabi")

  set_default_xcode_property(commonBuild)
  set_xcode_deployment_version(commonBuild "10.13" "12.0" "12.0" "4.0")

  set_xcode_property(commonBuild GCC_C_LANGUAGE_STANDARD "c17")
  set_xcode_property(commonBuild CLANG_C_LANGUAGE_STANDARD "c17")

  # set_xcode_property(commonBuild CLANG_CXX_LANGUAGE_STANDARD "c++98")
  set_xcode_property(commonBuild OTHER_CFLAGS[variant=Debug] ${SSRJSON_FLAGS})
  set_xcode_property(commonBuild OTHER_CFLAGS[variant=MinSizeRel]
                     ${SSRJSON_FLAGS})
  set_xcode_property(commonBuild OTHER_CFLAGS[variant=RelWithDebInfo]
                     ${SSRJSON_FLAGS})
  set_xcode_property(commonBuild OTHER_CFLAGS[variant=Release] ${SSRJSON_FLAGS})

elseif(MSVC)
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
    "-Wno-visibility")

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
    "-Wno-visibility")

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

if(BUILD_TEST)
  include(CTest)

  if(BUILD_MULTI_LIB)
    if("${TARGET_SIMD_ARCH}" STREQUAL "x86")
      add_library(ssrjson_test_sse4 OBJECT ${SRC_TEST_WITH_SIMD})
      target_link_libraries(ssrjson_test_sse4 PUBLIC commonBuild)
      add_library(ssrjson_test_avx2 OBJECT ${SRC_TEST_WITH_SIMD})
      target_link_libraries(ssrjson_test_avx2 PUBLIC commonBuild)
      add_library(ssrjson_test_avx512 OBJECT ${SRC_TEST_WITH_SIMD})
      target_link_libraries(ssrjson_test_avx512 PUBLIC commonBuild)

      add_avx512_compile_option(ssrjson_test_avx512)
      add_avx2_compile_option(ssrjson_test_avx2)
      add_sse4_compile_option(ssrjson_test_sse4)

      add_executable(
        ssrjson_test
        ${SRC_TEST} $<TARGET_OBJECTS:ssrjson_test_avx512>
        $<TARGET_OBJECTS:ssrjson_test_avx2> $<TARGET_OBJECTS:ssrjson_test_sse4>)
    elseif("${TARGET_SIMD_ARCH}" STREQUAL "aarch")
      add_library(ssrjson_test_neon OBJECT ${SRC_TEST_WITH_SIMD})
      target_link_libraries(ssrjson_test_neon PUBLIC commonBuild)
      add_executable(ssrjson_test ${SRC_TEST}
                                  $<TARGET_OBJECTS:ssrjson_test_neon>)
    else()
      message(FATAL_ERROR "TARGET_SIMD_ARCH=${TARGET_SIMD_ARCH} not supported")
    endif()
  else()
    add_executable(ssrjson_test ${SRC_TEST} ${SRC_TEST_WITH_SIMD})
  endif()

  target_link_libraries(ssrjson_test PUBLIC commonBuild ${Python3_LIBRARIES})
  install(TARGETS ssrjson_test RUNTIME DESTINATION .)
  add_test(ssrjson_test ${CMAKE_CURRENT_BINARY_DIR}/ssrjson_test)

  if((CMAKE_C_COMPILER_ID MATCHES Clang) AND NOT WIN32)
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/test_data/fuzzer/fuzzer.dict"
         DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/test_data/fuzzer/ssrjson_fuzz.py"
         DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(GLOB TEST_DATA_FILES
         "${CMAKE_CURRENT_SOURCE_DIR}/test_data/json/**/*.json")
    file(COPY ${TEST_DATA_FILES}
         DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/corpus")
    add_executable(ssrjson_fuzzer ${SRC_FUZZER})
    target_link_libraries(ssrjson_fuzzer PUBLIC commonBuild
                                                ${Python3_LIBRARIES})

    add_asan_compile_option(ssrjson_fuzzer)
    target_compile_options(ssrjson_fuzzer PRIVATE -fsanitize=fuzzer -g -O1)
    target_link_options(ssrjson_fuzzer PRIVATE -fsanitize=fuzzer)

    if(ASAN_ENABLED)
      add_asan_compile_option(ssrjson_fuzzer)
    endif()

    add_test(ssrjson_fuzzer ${CMAKE_CURRENT_BINARY_DIR}/ssrjson_fuzzer
             -dict=fuzzer.dict -max_total_time=1800
             ${CMAKE_CURRENT_BINARY_DIR}/corpus)
    set_tests_properties(ssrjson_fuzzer
                         PROPERTIES ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0")
  endif()
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/test_data/templates/info.json"
               "${CMAKE_CURRENT_BINARY_DIR}/info.json" @ONLY)
