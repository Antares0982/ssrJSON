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

function(add_sse4_compile_option TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  check_co_type(${CO_TYPE})
  target_compile_options(
    ${TARGET}
    ${CO_TYPE}
    $<$<C_COMPILER_ID:MSVC>:/arch:SSE4.2>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-msse4.2>
  )
endfunction(add_sse4_compile_option TARGET)

function(add_avx2_compile_option TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  check_co_type(${CO_TYPE})
  target_compile_options(
    ${TARGET}
    ${CO_TYPE}
    $<$<C_COMPILER_ID:MSVC>:/arch:AVX2>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-mavx2>
  )
endfunction(add_avx2_compile_option TARGET)

function(add_avx512_compile_option TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  check_co_type(${CO_TYPE})
  # Modern architecture except Knights Landing, Knights Mill
  target_compile_options(
    ${TARGET}
    ${CO_TYPE}
    $<$<C_COMPILER_ID:MSVC>:/arch:AVX512>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-mavx512f>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-mavx512cd>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-mavx512bw>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-mavx512vl>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-mavx512dq>
  )
endfunction(add_avx512_compile_option TARGET)

function(add_asan_compile_option TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  check_co_type(${CO_TYPE})
  target_compile_options(
    ${TARGET}
    ${CO_TYPE}
    $<$<C_COMPILER_ID:MSVC>:/fsanitize=address>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-fsanitize=address>
  )
  target_link_options(
    ${TARGET}
    ${CO_TYPE}
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-fsanitize=address>
  )
endfunction(add_asan_compile_option TARGET)

function(add_coverage_flags TARGET)
  if(ARGC GREATER 1)
    set(CO_TYPE "${ARGV1}")
  else()
    set(CO_TYPE "PRIVATE")
  endif()

  check_co_type(${CO_TYPE})
  target_compile_options(
    ${TARGET}
    ${CO_TYPE}
    $<$<C_COMPILER_ID:MSVC>:/fprofile-instr-generate>
    $<$<C_COMPILER_ID:MSVC>:/fcoverage-mapping>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-fprofile-instr-generate>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-fcoverage-mapping>
  )
  target_link_options(
    ${TARGET}
    ${CO_TYPE}
    $<$<C_COMPILER_ID:MSVC>:/fprofile-instr-generate>
    $<$<C_COMPILER_ID:MSVC>:/fcoverage-mapping>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-fprofile-instr-generate>
    $<$<OR:$<C_COMPILER_ID:GNU>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:Intel>>:-fcoverage-mapping>
  )
endfunction()
