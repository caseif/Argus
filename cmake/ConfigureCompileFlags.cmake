function(_argus_set_compile_flags TARGET)
  if(MSVC)
    target_compile_options("${TARGET}" PUBLIC "/W4" "/wd4996"
        "$<$<CONFIG:Debug>:/Od>")
    if(MSVC_VERSION LESS 1911)
      target_compile_options("${TARGET}" PUBLIC "$<$<CONFIG:Debug>:/DEBUG:FULL>")
    else()
      target_compile_options("${TARGET}" PUBLIC "$<$<CONFIG:Debug>:/DEBUG>")
    endif()

    target_compile_options("${TARGET}" PUBLIC "$<$<CONFIG:Release>:/O2>")
  else()
    target_compile_options("${TARGET}" PUBLIC
      "-Werror"
      "-Wall"
      "-Wmissing-noreturn"
      "-Wunreachable-code"
      "$<$<CXX_COMPILER_ID:Clang>:-Wnewline-eof>"
      "$<$<CXX_COMPILER_ID:Clang>:-Wsuggest-destructor-override>"
      "$<$<CXX_COMPILER_ID:Clang>:-Winconsistent-missing-destructor-override>"
      "$<$<CXX_COMPILER_ID:Clang>:-Wdocumentation>"
      #"-Wshadow"
      #"-Wshadow-field"
      #"-Wshadow-field-in-constructor"
      #"-Wshadow-uncaptured-local"
      # -Wmissing-prototypes is only available for C code in GCC
      "$<$<OR:$<COMPILE_LANGUAGE:C>,$<CXX_COMPILER_ID:Clang>>:-Wmissing-prototypes>"
      # -Wzero-as-nullpointer-constant is only available for C++ code in GCC
      "$<$<OR:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:Clang>>:-Wzero-as-null-pointer-constant>"
      # -Wextra-semi is only available for C++ code in GCC
      "$<$<OR:$<COMPILE_LANGUAGE:CXX>,$<C_COMPILER_ID:Clang>>:-Wextra-semi>"
      # -Wsuggest-override is only available for C++ code in GCC
      "$<$<OR:$<COMPILE_LANGUAGE:CXX>,$<C_COMPILER_ID:Clang>>:-Wsuggest-override>"
      "$<$<CXX_COMPILER_ID:Clang>:-Wunreachable-code-return>"
      "$<$<CXX_COMPILER_ID:Clang>:-Wconditional-uninitialized>"
      "-Wextra"
      "$<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>"
      "$<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>"
      "-Wuninitialized"
      "-Wimplicit-fallthrough"
      "-Wreturn-type"
      "-Wdeprecated"
      "-Wconversion"
      "-ftemplate-backtrace-limit=0")

    # -Wmismatched-tags is only available in Clang or GCC 10+
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
       OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 10))
      target_compile_options("${TARGET}" PUBLIC
        "$<$<COMPILE_LANGUAGE:CXX>:-Werror=mismatched-tags>")
    endif()

    target_compile_options("${TARGET}" PUBLIC "$<$<CONFIG:Debug>:-gdwarf-4>")
    target_link_options("${TARGET}" PUBLIC "$<$<CONFIG:Debug>:-gdwarf-4>")

    target_compile_options("${TARGET}" PUBLIC "$<$<CONFIG:Release>:-O3>")

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      # enable pedantic errors, but with a few exceptions:
      #   - pasting , and __va_args__ so we can have a nice assert macro
      #   - anonymous structs, since there's no good standard alternative
      #   - zero-length arrays, since again there's no good standard alternative
      # we also only enable pedantic errors for Clang because GCC apparently lacks the ability to configure exceptions
      target_compile_options("${TARGET}" PUBLIC
        "-Wpedantic"
        "-Wno-gnu-zero-variadic-macro-arguments"
        "-Wno-gnu-anonymous-struct"
        "-Wno-zero-length-array")
      # enable -Weffc++ for clang only
      target_compile_options("${TARGET}" PUBLIC
        "-Weffc++")
      if("${USE_ASAN}")
        target_compile_options("${TARGET}" PUBLIC
          "-fsanitize=address"
          "-fno-omit-frame-pointer"
          "-mllvm"
          "-asan-use-private-alias=1")
        target_link_options("${TARGET}" PUBLIC
          "-fsanitize=address"
          "-fno-omit-frame-pointer")
      endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_compile_options("${TARGET}" PUBLIC "-Wno-variadic-macros" "-Wno-error=unknown-pragmas")
    endif()
  endif()
endfunction()