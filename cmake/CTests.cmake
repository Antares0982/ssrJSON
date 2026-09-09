include(CTest)

set(SRC_TEST
    src/ctests/tools.c src/ctests/main.c src/utils/float_tables.c
    src/utils/mask_table.c src/utils/utf8_tables.c
    src/ctests/test_numpy_reserve.c)
set(SRC_TEST_WITH_SIMD src/ctests/test.c)
set(SRC_DECODE_FUZZER src/ctests/decode_fuzzer.c)
set(SRC_ENCODE_FUZZER src/ctests/encode_fuzzer.c)

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
    add_executable(ssrjson_test ${SRC_TEST} $<TARGET_OBJECTS:ssrjson_test_neon>)
  else()
    message(FATAL_ERROR "TARGET_SIMD_ARCH=${TARGET_SIMD_ARCH} not supported")
  endif()
else()
  add_executable(ssrjson_test ${SRC_TEST} ${SRC_TEST_WITH_SIMD})
endif()

target_link_libraries(ssrjson_test PUBLIC commonBuild ${Python3_LIBRARIES})
install(TARGETS ssrjson_test RUNTIME DESTINATION .)
add_test(ssrjson_test ${CMAKE_CURRENT_BINARY_DIR}/ssrjson_test)

if((CMAKE_C_COMPILER_ID MATCHES Clang)
   AND NOT WIN32
   AND BUILD_FUZZER)
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/test_data/fuzzer/fuzzer.dict"
       DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
  file(GLOB TEST_DATA_FILES
       "${CMAKE_CURRENT_SOURCE_DIR}/test_data/json/**/*.json")
  file(COPY ${TEST_DATA_FILES}
       DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/decode_corpus")
  add_executable(ssrjson_decode_fuzzer ${SRC_DECODE_FUZZER})
  target_link_libraries(ssrjson_decode_fuzzer PUBLIC commonBuild
                                                     ${Python3_LIBRARIES})

  add_asan_compile_option(ssrjson_decode_fuzzer)
  target_compile_options(ssrjson_decode_fuzzer PRIVATE -fsanitize=fuzzer -g -O1)
  target_link_options(ssrjson_decode_fuzzer PRIVATE -fsanitize=fuzzer)

  if(ASAN_ENABLED)
    add_asan_compile_option(ssrjson_decode_fuzzer)
  endif()

  add_test(ssrjson_decode_fuzzer
           ${CMAKE_CURRENT_BINARY_DIR}/ssrjson_decode_fuzzer -dict=fuzzer.dict
           -max_total_time=1800 ${CMAKE_CURRENT_BINARY_DIR}/decode_corpus)
  set_tests_properties(
    ssrjson_decode_fuzzer
    PROPERTIES
      ENVIRONMENT
      "ASAN_OPTIONS=detect_leaks=0;PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/test_data/fuzzer"
  )

  # Encoding fuzzer
  #
  # No -dict: the encode fuzzer input is an opaque control-byte stream consumed
  # positionally by the C generator, not JSON text, so a token dictionary does
  # not help guide mutation. The corpus carries the useful state.

  # Object generator: compiled hot (-O2) and WITHOUT SanitizerCoverage so that
  # the generator's own branches are not counted as fuzzer coverage (which would
  # dilute the coverage signal for ssrJSON). ASAN is kept to guard the raw
  # unicode buffer fills.
  add_library(ssrjson_encode_fuzz_gen OBJECT src/ctests/encode_fuzz_gen.c)
  target_link_libraries(ssrjson_encode_fuzz_gen PUBLIC commonBuild)
  target_compile_options(ssrjson_encode_fuzz_gen PRIVATE -g -O2)
  if(ASAN_ENABLED)
    add_asan_compile_option(ssrjson_encode_fuzz_gen)
  endif()

  add_executable(ssrjson_encode_fuzzer
                 ${SRC_ENCODE_FUZZER} $<TARGET_OBJECTS:ssrjson_encode_fuzz_gen>)
  target_link_libraries(ssrjson_encode_fuzzer PUBLIC commonBuild
                                                     ${Python3_LIBRARIES})

  add_asan_compile_option(ssrjson_encode_fuzzer)
  target_compile_options(ssrjson_encode_fuzzer PRIVATE -fsanitize=fuzzer -g -O1)
  target_link_options(ssrjson_encode_fuzzer PRIVATE -fsanitize=fuzzer)

  if(ASAN_ENABLED)
    add_asan_compile_option(ssrjson_encode_fuzzer)
  endif()

  add_test(ssrjson_encode_fuzzer
           ${CMAKE_CURRENT_BINARY_DIR}/ssrjson_encode_fuzzer
           -max_total_time=1800)
  set_tests_properties(
    ssrjson_encode_fuzzer
    PROPERTIES
      ENVIRONMENT
      "ASAN_OPTIONS=detect_leaks=0;PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/test_data/fuzzer"
  )
endif()
