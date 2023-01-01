project(gcc-invariant-plugin)
cmake_minimum_required(VERSION 3.20)

set(G++_COMPILER /usr/local/gcc-dev/bin/g++)
set(GCC_PLUGIN_DIR /usr/local/gcc-dev/lib/gcc/x86_64-linux-gnu/13.0.0/plugin)

# Add source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_library(gcc-invariant-plugin SHARED ${SOURCES})
target_compile_options(gcc-invariant-plugin
    PRIVATE -fno-rtti
)
target_include_directories(gcc-invariant-plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GCC_PLUGIN_DIR}/include
)

# Copy compile_commands.json to root directory if changed
add_custom_command(TARGET gcc-invariant-plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
        ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
)

# Command to run GCC with the build plugin using "-fplugin=./libgcc-invariant-plugin.so"
add_custom_target(run-gcc ALL
    COMMAND ${G++_COMPILER} -fplugin=./libgcc-invariant-plugin.so -std=c++2b -O0 -g -ggdb3 -fcontracts
            -o ${CMAKE_CURRENT_SOURCE_DIR}/test-binary ${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp
    DEPENDS gcc-invariant-plugin
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Command to run the compiled test binary using the plugin
add_custom_target(run-test ALL
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test-binary
    DEPENDS run-gcc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)