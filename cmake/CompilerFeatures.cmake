function(check_co_type CO_TYPE)
  if(NOT
     ("${CO_TYPE}" STREQUAL "PRIVATE"
      OR "${CO_TYPE}" STREQUAL "PUBLIC"
      OR "${CO_TYPE}" STREQUAL "INTERFACE"))
    message(
      FATAL_ERROR
        "Invalid compile option type: ${CO_TYPE}. Only PRIVATE, PUBLIC or INTERFACE are allowed."
    )
  endif()
endfunction(check_co_type CO_TYPE)

function(add_native_compile_option TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  if(MSVC)
    message(FATAL_ERROR "native option is not allowed in MSVC.")
  endif()

  check_co_type(${CO_TYPE})
  target_compile_options(${TARGET} ${CO_TYPE} -march=native)
endfunction()

function(add_ssse3_compile_option TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  check_co_type(${CO_TYPE})
  if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(${TARGET} ${CO_TYPE} /clang:-mssse3)
  else()
    target_compile_options(${TARGET} ${CO_TYPE} -mssse3)
  endif()
endfunction(add_ssse3_compile_option TARGET)

function(add_sse4_compile_option TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  check_co_type(${CO_TYPE})
  if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(${TARGET} ${CO_TYPE} /clang:-msse4.2)
  else()
    target_compile_options(${TARGET} ${CO_TYPE} -msse4.2)
  endif()
endfunction(add_sse4_compile_option TARGET)

function(add_avx2_compile_option TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  check_co_type(${CO_TYPE})
  if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(${TARGET} ${CO_TYPE} /arch:AVX2)
  else()
    target_compile_options(${TARGET} ${CO_TYPE} -mavx2)
  endif()
endfunction(add_avx2_compile_option TARGET)

function(add_avx512_compile_option TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  check_co_type(${CO_TYPE})
  # Modern architecture except Knights Landing, Knights Mill
  if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(
      ${TARGET}
      ${CO_TYPE}
      /clang:-mavx512f
      /clang:-mavx512cd
      /clang:-mavx512bw
      /clang:-mavx512vl
      /clang:-mavx512dq)
  else()
    target_compile_options(
      ${TARGET}
      ${CO_TYPE}
      -mavx512f
      -mavx512cd
      -mavx512bw
      -mavx512vl
      -mavx512dq)
  endif()
endfunction(add_avx512_compile_option TARGET)

function(add_asan_compile_option TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  check_co_type(${CO_TYPE})
  if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(${TARGET} ${CO_TYPE} /fsanitize=address)
  else()
    target_compile_options(${TARGET} ${CO_TYPE} -fsanitize=address)
    target_link_options(${TARGET} ${CO_TYPE} -fsanitize=address)
  endif()
endfunction(add_asan_compile_option TARGET)

function(add_fuzzer_coverage_option TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  check_co_type(${CO_TYPE})
  target_compile_options(${TARGET} ${CO_TYPE} -fsanitize=fuzzer-no-link)
  target_link_options(${TARGET} ${CO_TYPE} -fsanitize=fuzzer-no-link)
endfunction(add_fuzzer_coverage_option TARGET)

function(add_coverage_flags TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  check_co_type(${CO_TYPE})
  if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    target_compile_options(${TARGET} ${CO_TYPE} /fprofile-instr-generate
                           /fcoverage-mapping)
    target_link_options(${TARGET} ${CO_TYPE} /fprofile-instr-generate
                        /fcoverage-mapping)
  else()
    target_compile_options(${TARGET} ${CO_TYPE} -fprofile-instr-generate
                           -fcoverage-mapping)
    target_link_options(${TARGET} ${CO_TYPE} -fprofile-instr-generate
                        -fcoverage-mapping)
  endif()
endfunction()
