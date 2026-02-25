file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "*.cpp")

add_library(redis STATIC ${SRC_FILES})
target_include_directories(redis PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(redis PRIVATE -Wall -Wextra -Wpedantic -O2)
enable_sanitizers(redis)
enable_compiler_warnings(redis)