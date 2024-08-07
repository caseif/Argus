function(_argus_set_compile_flags TARGET)
  if(MSVC AND NOT MINGW)
    target_compile_options("${TARGET}" PUBLIC
        "/W4" # -Wall
        "/wd4068" # -Wno-unknown-pragmas
        "/wd4706" # assignment within cond expr
        "/wd4996" # -Wno-deprecated
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
      "-Wno-strict-aliasing"
      "$<$<COMPILE_LANGUAGE:CXX>:-ftemplate-backtrace-limit=0>")

    target_compile_options("${TARGET}" PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>")

    # -Wmismatched-tags is only available in Clang or GCC 10+
    if(CLANG OR (GCC AND CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 10))
      target_compile_options("${TARGET}" PUBLIC
        "$<$<COMPILE_LANGUAGE:CXX>:-Werror=mismatched-tags>")
    endif()

    target_compile_options("${TARGET}" PUBLIC "$<$<CONFIG:Debug>:-gdwarf-4>")
    target_link_options("${TARGET}" PUBLIC "$<$<CONFIG:Debug>:-gdwarf-4>")

    target_compile_options("${TARGET}" PUBLIC "$<$<CONFIG:Release>:-O3>")

    if(CLANG)
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
          "-fsanitize=leak"
          "-fno-omit-frame-pointer"
          "-mllvm"
          "-asan-use-private-alias=1")
        target_link_options("${TARGET}" PUBLIC
          "-fsanitize=address"
          "-fsanitize=leak"
          "-fno-omit-frame-pointer")
      endif()
    elseif(GCC)
      target_compile_options("${TARGET}" PUBLIC "-Wno-variadic-macros" "-Wno-error=unknown-pragmas")
    endif()
  endif()

  target_compile_definitions("${TARGET}" PUBLIC "SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS=1")
endfunction()