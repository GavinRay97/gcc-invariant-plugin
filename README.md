# GCC Design-by-Contract [[invariant]] plugin

A work-in-progress GCC plugin to support Design-by-Contract (DbC) in C++

Inspired wholly by the D Programming Language's [invariant](https://tour.dlang.org/tour/en/gems/contract-programming) feature.

## Building and Testing the Plugin

```sh-session
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build --target run-gcc
```

This will build the plugin located in `src/plugin.cpp` and test it against the contents of `test/test.cpp`.

It does this by running the following command:

```cmake
 ${GCC_COMPILER} -fplugin=./libgcc-invariant-plugin.so -c \
    -o ${CMAKE_CURRENT_SOURCE_DIR}/test-binary \
       ${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp
```
