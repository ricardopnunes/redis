# ConfigWarnings.cmake
# Adiciona flags de warning e trata warnings como erros em modo debug

function(enable_compiler_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Woverloaded-virtual
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
        )
        if (CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()
