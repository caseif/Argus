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
      #"-Weverything"
      #"-Wno-c++98-compat"
      #"-Wno-c++98-compat-pedantic"
      #"-Wno-reserved-macro-identifier"
      #"-Wno-global-constructors"
      #"-Wno-exit-time-destructors"
      #"-Wno-format-nonliteral"
      #"-Wno-missing-variable-declarations"
      #"-Wno-shadow-field-in-constructor"
      #"-Wno-double-promotion"
      #"-Wno-extra-semi"
      #"-Wno-padded"
      #"-Wno-reserved-identifier"
      #"-Wno-unused-macros"
      #"-Wno-missing-noreturn"
      #"-Wno-documentation"
      #"-Wno-weak-vtables"
      #"-Wno-switch-enum"
      #"-Wno-newline-eof"
      #"-Wno-shadow-uncaptured-local"
      #"-Wno-suggest-override"
      #"-Wno-suggest-destructor-override"
      #"-Wno-inconsistent-missing-destructor-override"
      #"-Wno-covered-switch-default"
      #"-Wno-undef"
      #"-Wconditional-uninitialized"
      #"-Wno-shadow"
      #"-Wno-shadow-field"
      #"-Wno-documentation-unknown-command"
      #"-Wno-missing-prototypes"
      #"-Wno-zero-as-null-pointer-constant"
      #"-Wno-float-equal"
      #"-Wno-signed-enum-bitfield"
      #"-Wno-unreachable-code"
      #"-Wno-unreachable-code-return"
      #"-Wno-declaration-after-statement"
      #"-Wno-unused-template"
      "-Wextra"
      "$<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>"
      "$<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>"
      "-Wuninitialized"
      "-Wimplicit-fallthrough"
      "-Wreturn-type"
      "-Wdeprecated"
      "-Wconversion")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
       OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION GREATER_EQUAL 10))
      target_compile_options("${TARGET}" PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:-Werror=mismatched-tags>")
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
      target_compile_options("${TARGET}" PUBLIC "-Wno-variadic-macros")
    endif()
  endif()
endfunction()