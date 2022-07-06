function(_argus_set_compile_flags TARGET)
  if(MSVC)
    target_compile_options("/W4" "/wd4996")
  else()
    target_compile_options("${TARGET}" PUBLIC
      "-Werror"
      "-Wall"
      "-Wextra"
      "$<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>"
      "$<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>"
      "-Wuninitialized"
      "-Wconversion"
      "-Wno-error=conversion"
      "-Wno-error=sign-conversion")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
       OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 10))
      target_compile_options("${TARGET}" PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:-Werror=mismatched-tags>")
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      # enable pedantic errors, but with a few exceptions:
      #   - pasting , and __va_args__ so we can have a nice assert macro
      #   - anonymous structs, since there's no good standard alternative
      #   - zero-length arrays, since again there's no good standard alternative
      # we also only enable pedantic errors for Clang because GCC apparently lacks the ability to configure exceptions
      target_compile_features("${TARGET}" PUBLIC
        "-Wpedantic"
        "-Wno-gnu-zero-variadic-macro-arguments"
        "-Wno-gnu-anonymous-struct"
        "-Wno-zero-length-array")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_compile_options("${TARGET}" PUBLIC "-Wno-variadic-macros")
    endif()
  endif()
endfunction()